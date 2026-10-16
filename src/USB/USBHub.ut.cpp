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

#include <gtest/gtest.h>
#include <aasdk/USB/UT/USBWrapper.mock.hpp>
#include <aasdk/USB/UT/AccessoryModeQueryChainFactory.mock.hpp>
#include <aasdk/USB/UT/AccessoryModeQueryChain.mock.hpp>
#include <aasdk/USB/UT/USBHubPromiseHandler.mock.hpp>
#include <aasdk/USB/USBHub.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgReferee;

class USBHubUnitTest : public testing::Test
{
protected:
    USBHubUnitTest()
        : queryChain_(&queryChainMock_, [](auto*) {})
        , device_(reinterpret_cast<libusb_device*>(-1))
        , deviceHandle_(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle_), [](auto*) {})
        , promise_(IUSBHub::Promise::defer(ioService_))
        , rawHotplugCallbacHandle_(-1)
        , hotplugCallbackHandle_(&rawHotplugCallbacHandle_, [](auto*) {})
    {
        promise_->then(std::bind(&USBHubPromiseHandlerMock::onResolve, &promiseHandlerMock_, std::placeholders::_1),
                      std::bind(&USBHubPromiseHandlerMock::onReject, &promiseHandlerMock_, std::placeholders::_1));
    }

    boost::asio::io_service ioService_;
    USBWrapperMock usbWrapperMock_;
    AccessoryModeQueryChainFactoryMock queryChainFactoryMock_;
    AccessoryModeQueryChainMock queryChainMock_;
    IAccessoryModeQueryChain::Pointer queryChain_;
    libusb_device* device_;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle_;
    DeviceHandle deviceHandle_;
    USBHubPromiseHandlerMock promiseHandlerMock_;
    IUSBHub::Promise::Pointer promise_;
    libusb_hotplug_callback_handle rawHotplugCallbacHandle_;
    HotplugCallbackHandle hotplugCallbackHandle_;
    libusb_hotplug_callback_fn hotplugCallback_;

    static constexpr uint16_t cGoogleVendorId = 0x18D1;
    static constexpr uint16_t cAOAPId = 0x2D00;
    static constexpr uint16_t cAOAPWithAdbId = 0x2D01;
};

TEST_F(USBHubUnitTest, USBHub_QueryDevice)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));

    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor connectedDeviceDescriptor = {0};
    connectedDeviceDescriptor.idVendor = 123;
    connectedDeviceDescriptor.idProduct = 456;

    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(connectedDeviceDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));
    EXPECT_CALL(queryChainFactoryMock_, create()).WillOnce(Return(queryChain_));

    IAccessoryModeQueryChain::Promise::Pointer queryChainPromise;
    EXPECT_CALL(queryChainMock_, start(deviceHandle_, _)).WillOnce(SaveArg<1>(&queryChainPromise));

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    queryChainPromise->resolve(deviceHandle_);
    usbHub->cancel();
    ioService_.run();
}

TEST_F(USBHubUnitTest, USBHub_AOAPDeviceConnected)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));

    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor connectedDeviceDescriptor = {0};
    connectedDeviceDescriptor.idVendor = cGoogleVendorId;
    connectedDeviceDescriptor.idProduct = cAOAPWithAdbId;

    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(connectedDeviceDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));
    EXPECT_CALL(promiseHandlerMock_, onResolve(deviceHandle_));
    EXPECT_CALL(promiseHandlerMock_, onReject(_)).Times(0);

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    usbHub->cancel();
    ioService_.run();
}

TEST_F(USBHubUnitTest, USBHub_GetDeviceDescriptorFailed)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));

    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor connectedDeviceDescriptor = {0};
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(connectedDeviceDescriptor), Return(1)));

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    usbHub->cancel();
    ioService_.run();
}

TEST_F(USBHubUnitTest, USBHub_OpenDeviceFailed)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));

    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor connectedDeviceDescriptor = {0};
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(connectedDeviceDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(1)));

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    usbHub->cancel();
    ioService_.run();
}

TEST_F(USBHubUnitTest, USBHub_CancelAllQueryChains)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));

    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor connectedDeviceDescriptor = {0};
    connectedDeviceDescriptor.idVendor = 123;
    connectedDeviceDescriptor.idProduct = 456;

    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).Times(2).WillRepeatedly(DoAll(SetArgReferee<1>(connectedDeviceDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).Times(2).WillRepeatedly(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));
    EXPECT_CALL(queryChainFactoryMock_, create()).Times(2).WillRepeatedly(Return(queryChain_));

    IAccessoryModeQueryChain::Promise::Pointer queryChainPromise[2];
    EXPECT_CALL(queryChainMock_, start(deviceHandle_, _)).WillOnce(SaveArg<1>(&queryChainPromise[0])).WillOnce(SaveArg<1>(&queryChainPromise[1]));

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    EXPECT_CALL(queryChainMock_, cancel()).Times(2);
    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    usbHub->cancel();
    ioService_.run();
    ioService_.reset();

    queryChainPromise[0]->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
    queryChainPromise[1]->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
    ioService_.run();
}

// Issue openhu#80: the phone (MTP) gets the accessory mode requests and comes back as accessory while the query
// chain of its old device has not finished yet. The accessory has to resolve the waiting promise, whatever the
// old chain reports afterwards.
TEST_F(USBHubUnitTest, USBHub_AccessoryArrivesWhileQueryChainRuns)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                         LIBUSB_HOTPLUG_MATCH_ANY, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    // the phone in MTP mode
    libusb_device_descriptor phoneDescriptor = {0};
    phoneDescriptor.idVendor = cGoogleVendorId;
    phoneDescriptor.idProduct = 0x4EE1;
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(phoneDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));
    EXPECT_CALL(queryChainFactoryMock_, create()).WillOnce(Return(queryChain_));
    IAccessoryModeQueryChain::Promise::Pointer queryChainPromise;
    EXPECT_CALL(queryChainMock_, start(deviceHandle_, _)).WillOnce(SaveArg<1>(&queryChainPromise));

    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();
    ASSERT_NE(queryChainPromise, nullptr);

    // the accessory arrives before the chain reported anything
    auto* accessory = reinterpret_cast<libusb_device*>(-2);
    USBWrapperMock::DummyDeviceHandle dummyAccessoryHandle;
    DeviceHandle accessoryHandle(reinterpret_cast<libusb_device_handle*>(&dummyAccessoryHandle), [](auto*) {});
    libusb_device_descriptor accessoryDescriptor = {0};
    accessoryDescriptor.idVendor = cGoogleVendorId;
    accessoryDescriptor.idProduct = cAOAPId;
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(accessory, _)).WillOnce(DoAll(SetArgReferee<1>(accessoryDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(accessory, _)).WillOnce(DoAll(SetArgReferee<1>(accessoryHandle), Return(0)));
    EXPECT_CALL(promiseHandlerMock_, onResolve(accessoryHandle));
    EXPECT_CALL(promiseHandlerMock_, onReject(_)).Times(0);

    hotplugCallback_(nullptr, accessory, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    // the old device is gone: its chain fails afterwards, which changes nothing
    queryChainPromise->reject(error::Error(error::ErrorCode::USB_TRANSFER, LIBUSB_TRANSFER_NO_DEVICE));
    ioService_.run();
    ioService_.reset();

    usbHub->cancel();
    ioService_.run();
}

// A failed open (e.g. the accessory node is not accessible yet) must leave the hub waiting: the next arrival of
// the accessory resolves the promise.
TEST_F(USBHubUnitTest, USBHub_OpenAccessoryFailedKeepsWaiting)
{
    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(_, _, _, _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), Return(hotplugCallbackHandle_)));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    usbHub->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor accessoryDescriptor = {0};
    accessoryDescriptor.idVendor = cGoogleVendorId;
    accessoryDescriptor.idProduct = cAOAPId;
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).Times(2).WillRepeatedly(DoAll(SetArgReferee<1>(accessoryDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _))
            .WillOnce(Return(LIBUSB_ERROR_ACCESS))
            .WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));

    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();
    testing::Mock::VerifyAndClearExpectations(&promiseHandlerMock_);

    EXPECT_CALL(promiseHandlerMock_, onResolve(deviceHandle_));
    EXPECT_CALL(promiseHandlerMock_, onReject(_)).Times(0);
    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    usbHub->cancel();
    ioService_.run();
}

// cancel() deregisters the hotplug callback, the next start() registers it again (with LIBUSB_HOTPLUG_ENUMERATE,
// so connected devices are reported again). OpenHU relies on this to re-arm the wait.
TEST_F(USBHubUnitTest, USBHub_CancelDeregistersAndStartRegistersAgain)
{
    int deregistered = 0;
    libusb_hotplug_callback_handle rawHandles[2] = {1, 2};
    auto makeHandle = [&deregistered](libusb_hotplug_callback_handle* raw) {
        return testing::InvokeWithoutArgs([&deregistered, raw]() {
            return HotplugCallbackHandle(raw, [&deregistered](auto*) { ++deregistered; });
        });
    };

    void* userData = nullptr;
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(_, LIBUSB_HOTPLUG_ENUMERATE, _, _, _, _, _))
            .WillOnce(makeHandle(&rawHandles[0]))
            .WillOnce(DoAll(SaveArg<5>(&hotplugCallback_), SaveArg<6>(&userData), makeHandle(&rawHandles[1])));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    usbHub->start(std::move(promise_));
    usbHub->cancel();
    ioService_.run();
    ioService_.reset();
    EXPECT_EQ(deregistered, 1);

    USBHubPromiseHandlerMock secondHandlerMock;
    auto secondPromise = IUSBHub::Promise::defer(ioService_);
    secondPromise->then(std::bind(&USBHubPromiseHandlerMock::onResolve, &secondHandlerMock, std::placeholders::_1),
                        std::bind(&USBHubPromiseHandlerMock::onReject, &secondHandlerMock, std::placeholders::_1));
    usbHub->start(std::move(secondPromise));
    ioService_.run();
    ioService_.reset();

    libusb_device_descriptor accessoryDescriptor = {0};
    accessoryDescriptor.idVendor = cGoogleVendorId;
    accessoryDescriptor.idProduct = cAOAPId;
    EXPECT_CALL(usbWrapperMock_, getDeviceDescriptor(device_, _)).WillOnce(DoAll(SetArgReferee<1>(accessoryDescriptor), Return(0)));
    EXPECT_CALL(usbWrapperMock_, open(device_, _)).WillOnce(DoAll(SetArgReferee<1>(deviceHandle_), Return(0)));
    EXPECT_CALL(secondHandlerMock, onResolve(deviceHandle_));
    hotplugCallback_(nullptr, device_, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, userData);
    ioService_.run();
    ioService_.reset();

    usbHub->cancel();
    ioService_.run();
    EXPECT_EQ(deregistered, 2);
}

// Without a registered callback no device is ever reported: the promise is rejected (not left pending forever)
// and the next start() tries to register again.
TEST_F(USBHubUnitTest, USBHub_RegistrationFailedRejects)
{
    EXPECT_CALL(usbWrapperMock_, hotplugRegisterCallback(_, _, _, _, _, _, _))
            .WillOnce(Return(HotplugCallbackHandle()))
            .WillOnce(Return(hotplugCallbackHandle_));

    USBHub::Pointer usbHub(std::make_shared<USBHub>(usbWrapperMock_, ioService_, queryChainFactoryMock_));
    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onReject(error::Error(error::ErrorCode::USB_LIST_DEVICES)));
    usbHub->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    USBHubPromiseHandlerMock secondHandlerMock;
    auto secondPromise = IUSBHub::Promise::defer(ioService_);
    secondPromise->then(std::bind(&USBHubPromiseHandlerMock::onResolve, &secondHandlerMock, std::placeholders::_1),
                        std::bind(&USBHubPromiseHandlerMock::onReject, &secondHandlerMock, std::placeholders::_1));
    EXPECT_CALL(secondHandlerMock, onReject(error::Error(error::ErrorCode::OPERATION_ABORTED)));
    usbHub->start(std::move(secondPromise));
    usbHub->cancel();
    ioService_.run();
}

}
}
}
