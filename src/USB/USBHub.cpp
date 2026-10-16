// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
//
// aasdk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// aasdk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/USB/USBHub.hpp>
#include <aasdk/USB/DeviceFilter.hpp>
#include <aasdk/USB/AccessoryModeQueryChain.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>
#include <cstdio>


namespace aasdk {
  namespace usb {

    namespace {

      std::string deviceId(const libusb_device_descriptor &descriptor) {
        char id[10];
        std::snprintf(id, sizeof(id), "%04x:%04x", descriptor.idVendor, descriptor.idProduct);
        return id;
      }

    }

    USBHub::USBHub(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService,
                   IAccessoryModeQueryChainFactory &queryChainFactory)
        : usbWrapper_(usbWrapper), strand_(ioService), queryChainFactory_(queryChainFactory) {
    }

    void USBHub::start(Promise::Pointer promise) {
      strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() {
        if (hotplugPromise_ != nullptr) {
          hotplugPromise_->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
          hotplugPromise_.reset();
        }

        hotplugPromise_ = std::move(promise);

        if (self_ == nullptr) {
          AASDK_LOG(info) << "[USBHub] registering the hotplug callback, connected devices follow";
          // LIBUSB_HOTPLUG_ENUMERATE calls hotplugEventsHandler for every connected device before this returns,
          // on this strand, so handleDevice runs inline for them.
          hotplugHandle_ = usbWrapper_.hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                                               LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
                                                               LIBUSB_HOTPLUG_MATCH_ANY,
                                                               LIBUSB_HOTPLUG_MATCH_ANY,
                                                               reinterpret_cast<libusb_hotplug_callback_fn>(&USBHub::hotplugEventsHandler),
                                                               reinterpret_cast<void *>(this));
          if (hotplugHandle_ == nullptr) {
            // nobody would ever resolve the promise: tell the caller, the next start() tries again
            AASDK_LOG(error) << "[USBHub] hotplug callback not registered, no USB device will be found";
            if (hotplugPromise_ != nullptr) {
              hotplugPromise_->reject(error::Error(error::ErrorCode::USB_LIST_DEVICES));
              hotplugPromise_.reset();
            }
            return;
          }
          self_ = this->shared_from_this();
        } else {
          AASDK_LOG(debug) << "[USBHub] waiting for devices (hotplug callback already registered)";
        }
      });
    }

    void USBHub::cancel() {
      strand_.dispatch([this, self = this->shared_from_this()]() mutable {
        if (hotplugPromise_ != nullptr) {
          hotplugPromise_->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
          hotplugPromise_.reset();
        }

        std::for_each(queryChainQueue_.begin(), queryChainQueue_.end(),
                      std::bind(&IAccessoryModeQueryChain::cancel, std::placeholders::_1));

        if (self_ != nullptr) {
          AASDK_LOG(debug) << "[USBHub] cancelled, hotplug callback deregistered";
          hotplugHandle_.reset();
          self_.reset();
        }
      });
    }

    int USBHub::hotplugEventsHandler(libusb_context *usbContext, libusb_device *device, libusb_hotplug_event event,
                                     void *userData) {
      if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        auto self = reinterpret_cast<USBHub *>(userData)->shared_from_this();
        self->strand_.dispatch(std::bind(&USBHub::handleDevice, self, device));
      }

      return 0;
    }

    bool USBHub::isAOAPDevice(const libusb_device_descriptor &deviceDescriptor) const {
      return deviceDescriptor.idVendor == cGoogleVendorId &&
             (deviceDescriptor.idProduct == cAOAPId || deviceDescriptor.idProduct == cAOAPWithAdbId);
    }

    void USBHub::handleDevice(libusb_device *device) {
      libusb_device_descriptor deviceDescriptor;
      if (usbWrapper_.getDeviceDescriptor(device, deviceDescriptor) != 0) {
        AASDK_LOG(warning) << "[USBHub] device arrived, reading its descriptor failed";
        return;
      }
      const auto id = deviceId(deviceDescriptor);

      if (hotplugPromise_ == nullptr) {
        // nobody waits (a session runs): the caller registers again (start after cancel) when it waits again
        AASDK_LOG(info) << "[USBHub] " << id << " arrived, ignored: nobody is waiting for a device";
        return;
      }
      if (!this->isAOAPDevice(deviceDescriptor) && isCertainlyNotPhone(usbWrapper_, device, deviceDescriptor)) {
        AASDK_LOG(debug) << "[USBHub] " << id << " arrived, skipped: not a phone";
        return;
      }

      DeviceHandle handle;
      auto openResult = usbWrapper_.open(device, handle);

      if (openResult != 0) {
        AASDK_LOG(warning) << "[USBHub] " << id << " arrived, opening it failed: "
                           << libusb_error_name(openResult);
        return;
      }

      if (this->isAOAPDevice(deviceDescriptor)) {
        AASDK_LOG(info) << "[USBHub] " << id << " is in accessory mode, handing it over";
        hotplugPromise_->resolve(std::move(handle));
        hotplugPromise_.reset();
      } else {
        AASDK_LOG(info) << "[USBHub] " << id << " arrived, sending the accessory mode requests";
        queryChainQueue_.emplace_back(queryChainFactory_.create());

        auto queueElementIter = std::prev(queryChainQueue_.end());
        auto queryChainPromise = IAccessoryModeQueryChain::Promise::defer(strand_);
        // The device comes back with another id (accessory mode) and arrives as a new device; this hub has to be
        // waiting by then, which the promise above still is (only an accessory resolves it).
        queryChainPromise->then([this, self = this->shared_from_this(), queueElementIter, id](DeviceHandle) mutable {
                                  AASDK_LOG(info) << "[USBHub] " << id
                                                  << " accepted the accessory mode requests, waiting for it to come back as accessory";
                                  queryChainQueue_.erase(queueElementIter);
                                },
                                [this, self = this->shared_from_this(), queueElementIter, id](
                                    const error::Error &e) mutable {
                                  AASDK_LOG(info) << "[USBHub] accessory mode requests to " << id << " failed: "
                                                  << e.what();
                                  queryChainQueue_.erase(queueElementIter);
                                });

        queryChainQueue_.back()->start(std::move(handle), std::move(queryChainPromise));
      }
    }

  }
}
