/*
*  This file is part of aasdk library project.
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

// Messages on the control channel, delivered through a messenger mock (average-web-dev/aasdk#5)

#include <gtest/gtest.h>
#include <aasdk/Messenger/UT/Messenger.mock.hpp>
#include <aasdk/Channel/Control/UT/ControlServiceChannelEventHandler.mock.hpp>
#include <aasdk/Channel/Control/ControlServiceChannel.hpp>
#include <aasdk/Channel/Promise.hpp>
#include <aasdk/Messenger/MessageId.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace channel
{
namespace control
{
namespace ut
{

using ::testing::_;
using ::testing::SaveArg;
using ::testing::StrictMock;

class ControlServiceChannelUnitTest : public testing::Test
{
protected:
    ControlServiceChannelUnitTest()
        : strand_(ioService_)
        , messenger_(std::make_shared<StrictMock<messenger::ut::MessengerMock>>())
        , eventHandler_(std::make_shared<StrictMock<ControlServiceChannelEventHandlerMock>>())
        , channel_(std::make_shared<ControlServiceChannel>(strand_, messenger_))
    {
    }

    // what the phone sends on the control channel: message id followed by the payload
    void receive(uint16_t messageId, const common::Data& payload)
    {
        messenger::ReceivePromise::Pointer promise;
        EXPECT_CALL(*messenger_, enqueueReceive(messenger::ChannelId::CONTROL, _)).WillOnce(SaveArg<1>(&promise));
        channel_->receive(eventHandler_);
        ASSERT_NE(promise, nullptr);

        auto message = std::make_shared<messenger::Message>(messenger::ChannelId::CONTROL, messenger::EncryptionType::PLAIN,
                                                            messenger::MessageType::SPECIFIC);
        message->insertPayload(messenger::MessageId(messageId).getData());
        message->insertPayload(payload);
        promise->resolve(std::move(message));
        ioService_.run();
        ioService_.restart();
    }

    // like receive(), for messages after which the channel keeps receiving by itself (#7)
    void receiveAndRearm(uint16_t messageId, const common::Data& payload)
    {
        messenger::ReceivePromise::Pointer promise;
        EXPECT_CALL(*messenger_, enqueueReceive(messenger::ChannelId::CONTROL, _))
            .WillOnce(SaveArg<1>(&promise))
            .WillOnce(testing::Return());
        channel_->receive(eventHandler_);
        ASSERT_NE(promise, nullptr);

        auto message = std::make_shared<messenger::Message>(messenger::ChannelId::CONTROL, messenger::EncryptionType::ENCRYPTED,
                                                            messenger::MessageType::SPECIFIC);
        message->insertPayload(messenger::MessageId(messageId).getData());
        message->insertPayload(payload);
        promise->resolve(std::move(message));
        ioService_.run();
        ioService_.restart();
        testing::Mock::VerifyAndClearExpectations(messenger_.get());
    }

    // id of a message the channel sent
    uint16_t sentId(const std::function<void(SendPromise::Pointer)>& send)
    {
        messenger::Message::Pointer message;
        EXPECT_CALL(*messenger_, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
        send(SendPromise::defer(ioService_));
        EXPECT_NE(message, nullptr);
        if(message == nullptr || message->getPayload().size() < 2)
        {
            return 0;
        }
        return message->getPayload()[0] << 8 | message->getPayload()[1];
    }

    boost::asio::io_service ioService_;
    boost::asio::io_service::strand strand_;
    std::shared_ptr<StrictMock<messenger::ut::MessengerMock>> messenger_;
    std::shared_ptr<StrictMock<ControlServiceChannelEventHandlerMock>> eventHandler_;
    ControlServiceChannel::Pointer channel_;
};

constexpr uint16_t cVersionResponse = aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VERSION_RESPONSE;

TEST_F(ControlServiceChannelUnitTest, VersionResponse_BigEndianValues)
{
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 7, aap_protobuf::shared::MessageStatus::STATUS_SUCCESS));
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x07, 0x00, 0x00});
}

TEST_F(ControlServiceChannelUnitTest, VersionResponse_NoCompatibleVersionIsNegative)
{
    // the status is a signed 16 bit value, 0xFFFF must not arrive as 65535
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 6, aap_protobuf::shared::MessageStatus::STATUS_NO_COMPATIBLE_VERSION));
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x06, 0xFF, 0xFF});
}

TEST_F(ControlServiceChannelUnitTest, VersionResponse_UnreadableTrailingBytesIgnored)
{
    // bytes after the version that are no VersionResponseOptions: the version still counts
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 6, aap_protobuf::shared::MessageStatus::STATUS_SUCCESS));
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0xFF, 0xFF});
}

TEST_F(ControlServiceChannelUnitTest, VersionResponse_TooShort)
{
    for(size_t size = 0; size < 6; ++size)
    {
        SCOPED_TRACE(size);
        error::Error error;
        EXPECT_CALL(*eventHandler_, onChannelError(_)).WillOnce(SaveArg<0>(&error));
        receive(cVersionResponse, common::Data(size, 0x01));
        EXPECT_EQ(error.getCode(), error::ErrorCode::PARSE_PAYLOAD);
        testing::Mock::VerifyAndClearExpectations(eventHandler_.get());
    }
}

TEST_F(ControlServiceChannelUnitTest, VersionRequest_BigEndianValues)
{
    messenger::Message::Pointer message;
    EXPECT_CALL(*messenger_, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
    channel_->sendVersionRequest(SendPromise::defer(ioService_));
    ASSERT_NE(message, nullptr);

    const common::Data expected{0x00, 0x05, 0x00, 0x01};
    const auto& payload = message->getPayload();
    ASSERT_EQ(payload.size(), 6u);
    EXPECT_EQ(common::Data(payload.begin() + 2, payload.end()), expected);
}

TEST_F(ControlServiceChannelUnitTest, VersionRequest_FromEnvironment)
{
    ::setenv("AASDK_VERSION", "6.1", 1);
    messenger::Message::Pointer message;
    EXPECT_CALL(*messenger_, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
    channel_->sendVersionRequest(SendPromise::defer(ioService_));
    ::unsetenv("AASDK_VERSION");
    ASSERT_NE(message, nullptr);

    const common::Data expected{0x00, 0x06, 0x00, 0x01};
    const auto& payload = message->getPayload();
    EXPECT_EQ(common::Data(payload.begin() + 2, payload.end()), expected);
}

TEST_F(ControlServiceChannelUnitTest, VersionRequest_InvalidEnvironmentIgnored)
{
    ::setenv("AASDK_VERSION", "five", 1);
    messenger::Message::Pointer message;
    EXPECT_CALL(*messenger_, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
    channel_->sendVersionRequest(SendPromise::defer(ioService_));
    ::unsetenv("AASDK_VERSION");
    ASSERT_NE(message, nullptr);

    const common::Data expected{0x00, 0x05, 0x00, 0x01};
    const auto& payload = message->getPayload();
    EXPECT_EQ(common::Data(payload.begin() + 2, payload.end()), expected);
}


namespace cm = aap_protobuf::service::control::message;

TEST_F(ControlServiceChannelUnitTest, VersionResponse_OptionsPassedOn)
{
    // 6 bytes version, then VersionResponseOptions: connection_configuration.ping_configuration.interval_ms = 1000
    testing::Sequence order;
    cm::VersionResponseOptions options;
    EXPECT_CALL(*eventHandler_, onVersionResponseOptions(_)).InSequence(order).WillOnce(SaveArg<0>(&options));
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 7, aap_protobuf::shared::MessageStatus::STATUS_SUCCESS)).InSequence(order);
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x07, 0x00, 0x00, 0x0A, 0x05, 0x0A, 0x03, 0x10, 0xE8, 0x07});
    EXPECT_EQ(options.connection_configuration().ping_configuration().interval_ms(), 1000u);
}

TEST_F(ControlServiceChannelUnitTest, VersionResponse_SecondOneIgnored)
{
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 7, _)).Times(1);
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x07, 0x00, 0x00});
    // a late answer to an earlier request: no second handshake, the channel keeps receiving
    receiveAndRearm(cVersionResponse, {0x00, 0x01, 0x00, 0x06, 0x00, 0x00});
}

TEST_F(ControlServiceChannelUnitTest, Notifications_PassedOnAndChannelKeepsReceiving)
{
    cm::CallAvailabilityStatus callStatus;
    EXPECT_CALL(*eventHandler_, onCallAvailabilityStatus(_)).WillOnce(SaveArg<0>(&callStatus));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_CALL_AVAILABILITY_STATUS, {0x08, 0x01});
    EXPECT_TRUE(callStatus.call_available());

    EXPECT_CALL(*eventHandler_, onChannelCloseNotification(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_CHANNEL_CLOSE_NOTIFICATION, {});

    EXPECT_CALL(*eventHandler_, onServiceDiscoveryUpdate(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_UPDATE, {});

    EXPECT_CALL(*eventHandler_, onCarConnectedDevicesRequest(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_REQUEST, {});

    cm::CarConnectedDevices devices;
    EXPECT_CALL(*eventHandler_, onCarConnectedDevices(_)).WillOnce(SaveArg<0>(&devices));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_RESPONSE, {0x0A, 0x03, 0x0A, 0x01, 'p'});
    ASSERT_EQ(devices.connected_devices_size(), 1);
    EXPECT_EQ(devices.connected_devices(0).device_name(), "p");

    EXPECT_CALL(*eventHandler_, onUserSwitchRequest(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_USER_SWITCH_REQUEST, {});

    EXPECT_CALL(*eventHandler_, onUserSwitchResponse(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_USER_SWITCH_RESPONSE, {0x08, 0x00});
}

TEST_F(ControlServiceChannelUnitTest, Notification_UnreadableIgnored)
{
    // field 1 announced with 5 bytes, 1 present: not readable, no callback, the session goes on
    receiveAndRearm(cm::ControlMessageType::MESSAGE_CALL_AVAILABILITY_STATUS, {0x0A, 0x05, 0x01});
}

TEST_F(ControlServiceChannelUnitTest, UnexpectedMessage_PassedOn)
{
    EXPECT_CALL(*eventHandler_, onUnexpectedMessage(_));
    receiveAndRearm(cm::ControlMessageType::MESSAGE_UNEXPECTED_MESSAGE, {0x01, 0x02});
}

TEST_F(ControlServiceChannelUnitTest, FramingError_EndsSession)
{
    error::Error error;
    EXPECT_CALL(*eventHandler_, onChannelError(_)).WillOnce(SaveArg<0>(&error));
    receive(cm::ControlMessageType::MESSAGE_FRAMING_ERROR, {});
    EXPECT_EQ(error.getCode(), error::ErrorCode::MESSENGER_PEER_FRAMING_ERROR);
}

TEST_F(ControlServiceChannelUnitTest, NewSenders_MessageIds)
{
    EXPECT_EQ(sentId([this](auto p) { channel_->sendServiceDiscoveryUpdate(cm::ServiceDiscoveryUpdate(), std::move(p)); }), 26);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendCallAvailabilityStatus(cm::CallAvailabilityStatus(), std::move(p)); }), 24);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendCarConnectedDevicesRequest(cm::CarConnectedDevicesRequest(), std::move(p)); }), 20);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendCarConnectedDevices(cm::CarConnectedDevices(), std::move(p)); }), 21);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendUserSwitchRequest(cm::UserSwitchRequest(), std::move(p)); }), 22);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendUserSwitchResponse(cm::UserSwitchResponse(), std::move(p)); }), 25);
    EXPECT_EQ(sentId([this](auto p) { channel_->sendChannelCloseNotification(cm::ChannelCloseNotification(), std::move(p)); }), 9);
}

}
}
}
}
