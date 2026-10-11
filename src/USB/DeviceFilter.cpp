// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
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

#include <aasdk/USB/DeviceFilter.hpp>

namespace aasdk::usb {

  namespace {

    bool isNonPhoneClass(uint8_t interfaceClass) {
      switch (interfaceClass) {
        case LIBUSB_CLASS_AUDIO:
        case LIBUSB_CLASS_HID:
        case LIBUSB_CLASS_PRINTER:
        case LIBUSB_CLASS_HUB:
        case LIBUSB_CLASS_SMART_CARD:
        case LIBUSB_CLASS_VIDEO:
        case 0x10: // audio/video devices
        case 0x11: // billboard
          return true;
        default:
          return false;
      }
    }

  }

  bool isCertainlyNotPhone(IUSBWrapper &usbWrapper, libusb_device *device,
                           const libusb_device_descriptor &deviceDescriptor) {
    if (deviceDescriptor.bDeviceClass == LIBUSB_CLASS_HUB) {
      return true;
    }

    ConfigDescriptorHandle config;
    if (usbWrapper.getConfigDescriptor(device, 0, config) != 0 || config == nullptr) {
      return false;
    }

    bool onlyNonPhone = config->bNumInterfaces > 0;
    for (uint8_t i = 0; i < config->bNumInterfaces && onlyNonPhone; ++i) {
      const auto &interface = config->interface[i];
      for (int a = 0; a < interface.num_altsetting && onlyNonPhone; ++a) {
        onlyNonPhone = isNonPhoneClass(interface.altsetting[a].bInterfaceClass);
      }
    }
    return onlyNonPhone;
  }

}
