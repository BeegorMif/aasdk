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

TEST_F(ControlServiceChannelUnitTest, VersionResponse_TrailingOptionsIgnored)
{
    EXPECT_CALL(*eventHandler_, onVersionResponse(1, 6, aap_protobuf::shared::MessageStatus::STATUS_SUCCESS));
    receive(cVersionResponse, {0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x0A, 0x02, 0x0A, 0x00});
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

}
}
}
}
