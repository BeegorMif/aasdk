/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#include <aasdk/USB/UT/USBWrapper.mock.hpp>
#include <aasdk/USB/AOAPDevice.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

TEST(AOAPDevice_UnitTest, AOAPDevice_OutEndpointFirst)
{
    USBWrapperMock usbWrapperMock;
    boost::asio::io_service ioService;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle;
    DeviceHandle deviceHandle(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle), [](auto*) {});

    libusb_endpoint_descriptor endpointDescriptor[2];
    endpointDescriptor[0].bEndpointAddress = LIBUSB_ENDPOINT_OUT + 4;
    endpointDescriptor[1].bEndpointAddress = LIBUSB_ENDPOINT_IN + 6;

    libusb_interface_descriptor interfaceDescriptor;
    interfaceDescriptor.bInterfaceNumber = 55;
    interfaceDescriptor.bNumEndpoints = 2;
    interfaceDescriptor.endpoint = endpointDescriptor;

    EXPECT_CALL(usbWrapperMock, releaseInterface(deviceHandle, interfaceDescriptor.bInterfaceNumber));
    AOAPDevice aoapDevice(usbWrapperMock, ioService, deviceHandle, &interfaceDescriptor);
    EXPECT_TRUE(endpointDescriptor[0].bEndpointAddress == aoapDevice.getOutEndpoint().getAddress());
    EXPECT_TRUE(endpointDescriptor[1].bEndpointAddress == aoapDevice.getInEndpoint().getAddress());
}

TEST(AOAPDevice_UnitTest, AOAPDevice_InEndpointFirst)
{
    USBWrapperMock usbWrapperMock;
    boost::asio::io_service ioService;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle;
    DeviceHandle deviceHandle(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle), [](auto*) {});

    libusb_endpoint_descriptor endpointDescriptor[2];
    endpointDescriptor[0].bEndpointAddress = LIBUSB_ENDPOINT_IN + 4;
    endpointDescriptor[1].bEndpointAddress = LIBUSB_ENDPOINT_OUT + 6;

    libusb_interface_descriptor interfaceDescriptor;
    interfaceDescriptor.bInterfaceNumber = 55;
    interfaceDescriptor.bNumEndpoints = 2;
    interfaceDescriptor.endpoint = endpointDescriptor;

    EXPECT_CALL(usbWrapperMock, releaseInterface(deviceHandle, interfaceDescriptor.bInterfaceNumber));
    AOAPDevice aoapDevice(usbWrapperMock, ioService, deviceHandle, &interfaceDescriptor);
    EXPECT_TRUE(endpointDescriptor[0].bEndpointAddress == aoapDevice.getInEndpoint().getAddress());
    EXPECT_TRUE(endpointDescriptor[1].bEndpointAddress == aoapDevice.getOutEndpoint().getAddress());
}

TEST(AOAPDevice_UnitTest, AOAPDevice_CreateKeepsNoPointerIntoConfigDescriptor)
{
    USBWrapperMock usbWrapperMock;
    boost::asio::io_service ioService;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle;
    DeviceHandle deviceHandle(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle), [](auto*) {});

    // a config descriptor on the heap that is freed when its handle goes, like libusb_free_config_descriptor
    struct Descriptor
    {
        libusb_endpoint_descriptor endpoints[2]{};
        libusb_interface_descriptor altsetting{};
        libusb_interface interface{};
        libusb_config_descriptor config{};
    };
    auto descriptor = new Descriptor;
    descriptor->endpoints[0].bEndpointAddress = LIBUSB_ENDPOINT_IN + 4;
    descriptor->endpoints[1].bEndpointAddress = LIBUSB_ENDPOINT_OUT + 6;
    descriptor->altsetting.bInterfaceNumber = 55;
    descriptor->altsetting.bNumEndpoints = 2;
    descriptor->altsetting.endpoint = descriptor->endpoints;
    descriptor->interface.altsetting = &descriptor->altsetting;
    descriptor->interface.num_altsetting = 1;
    descriptor->config.bNumInterfaces = 1;
    descriptor->config.interface = &descriptor->interface;
    ConfigDescriptorHandle configDescriptorHandle(&descriptor->config, [descriptor](auto*) { delete descriptor; });

    EXPECT_CALL(usbWrapperMock, getDevice(deviceHandle)).WillOnce(testing::Return(nullptr));
    // hand over the only reference: create() frees the descriptor when it returns
    EXPECT_CALL(usbWrapperMock, getConfigDescriptor(nullptr, 0, testing::_))
        .WillOnce(testing::Invoke([&configDescriptorHandle](libusb_device*, uint8_t, ConfigDescriptorHandle& handle) {
            handle = std::move(configDescriptorHandle);
            return 0;
        }));
    EXPECT_CALL(usbWrapperMock, claimInterface(deviceHandle, 55)).WillOnce(testing::Return(0));

    auto aoapDevice = AOAPDevice::create(usbWrapperMock, ioService, deviceHandle);
    EXPECT_EQ(aoapDevice->getInEndpoint().getAddress(), LIBUSB_ENDPOINT_IN + 4);
    EXPECT_EQ(aoapDevice->getOutEndpoint().getAddress(), LIBUSB_ENDPOINT_OUT + 6);

    // the descriptor is freed now; the destructor still has to release interface 55
    EXPECT_CALL(usbWrapperMock, releaseInterface(deviceHandle, 55));
    aoapDevice.reset();
}

}
}
}
