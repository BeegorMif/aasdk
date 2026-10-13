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

// Messages the microphone channel sends (found in a protocol capture, #14)

#include <gtest/gtest.h>
#include <aasdk/Messenger/UT/Messenger.mock.hpp>
#include <aasdk/Channel/MediaSource/MediaSourceService.hpp>
#include <aasdk/Channel/Promise.hpp>


namespace aasdk
{
namespace channel
{
namespace mediasource
{
namespace ut
{

using ::testing::_;
using ::testing::SaveArg;

TEST(MediaSourceServiceUnitTest, MicrophoneResponseHasResponseId)
{
    boost::asio::io_service ioService;
    boost::asio::io_service::strand strand(ioService);
    auto messenger = std::make_shared<testing::StrictMock<messenger::ut::MessengerMock>>();
    auto service = std::make_shared<MediaSourceService>(strand, messenger, messenger::ChannelId::MEDIA_SOURCE_MICROPHONE);

    messenger::Message::Pointer message;
    EXPECT_CALL(*messenger, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
    aap_protobuf::service::media::source::message::MicrophoneResponse response;
    response.set_status(0);
    response.set_session_id(0);
    service->sendMicrophoneOpenResponse(response, SendPromise::defer(ioService));

    ASSERT_NE(message, nullptr);
    const auto& payload = message->getPayload();
    ASSERT_GE(payload.size(), 2u);
    // MEDIA_MESSAGE_MICROPHONE_RESPONSE = 32774; the request id 32773 was sent before
    EXPECT_EQ(payload[0], 0x80);
    EXPECT_EQ(payload[1], 0x06);
}

}
}
}
}
