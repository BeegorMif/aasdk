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

// Protocol capture (average-web-dev/aasdk#14)

#include <cstdio>
#include <fstream>
#include <sstream>
#include <gtest/gtest.h>
#include <aasdk/Messenger/Capture.hpp>
#include <aasdk/Messenger/UT/MessageInStream.mock.hpp>
#include <aasdk/Messenger/UT/MessageOutStream.mock.hpp>
#include <aasdk/Messenger/Messenger.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

using ::testing::_;

class CaptureUnitTest : public testing::Test
{
protected:
    CaptureUnitTest()
        : path_(testing::TempDir() + "aasdk-capture-test.jsonl")
    {
        std::remove(path_.c_str());
    }

    ~CaptureUnitTest() override
    {
        capture::stop();
        std::remove(path_.c_str());
    }

    std::vector<std::string> lines()
    {
        std::ifstream file(path_);
        std::vector<std::string> result;
        for(std::string line; std::getline(file, line);)
        {
            result.push_back(line);
        }
        return result;
    }

    static Message message(ChannelId channel, EncryptionType encryption, MessageType type, const common::Data& payload)
    {
        Message message(channel, encryption, type);
        message.insertPayload(payload);
        return message;
    }

    std::string path_;
};

TEST_F(CaptureUnitTest, OffWithoutStart)
{
    capture::stop();
    EXPECT_FALSE(capture::enabled());
    capture::record("in", message(ChannelId::CONTROL, EncryptionType::PLAIN, MessageType::SPECIFIC, {0x00, 0x02}));
    EXPECT_TRUE(lines().empty());
}

TEST_F(CaptureUnitTest, ControlMessageWithPayload)
{
    ASSERT_TRUE(capture::start(path_));
    capture::record("in", message(ChannelId::CONTROL, EncryptionType::PLAIN, MessageType::SPECIFIC,
                                  {0x00, 0x02, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00}));

    const auto result = lines();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_NE(result[0].find("\"dir\":\"in\",\"ch\":0,\"chName\":\"CONTROL\",\"enc\":false,\"control\":false,\"id\":2,\"len\":8,\"hex\":\"0002000100060000\"}"),
              std::string::npos) << result[0];
    EXPECT_EQ(result[0].rfind("{\"t\":", 0), 0u) << result[0];
}

TEST_F(CaptureUnitTest, MediaDataWithoutPayload)
{
    ASSERT_TRUE(capture::start(path_));
    capture::record("in", message(ChannelId::MEDIA_SINK_VIDEO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC,
                                  {0x00, 0x00, 0x01, 0x02, 0x03}));
    // control message type on a service channel (e.g. channel open) keeps its payload
    capture::record("out", message(ChannelId::MEDIA_SINK_VIDEO, EncryptionType::ENCRYPTED, MessageType::CONTROL,
                                   {0x00, 0x08, 0x08, 0x00}));

    const auto result = lines();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_NE(result[0].find("\"enc\":true,\"control\":false,\"id\":0,\"len\":5}"), std::string::npos) << result[0];
    EXPECT_EQ(result[0].find("hex"), std::string::npos) << result[0];
    EXPECT_NE(result[1].find("\"dir\":\"out\""), std::string::npos) << result[1];
    EXPECT_NE(result[1].find("\"control\":true,\"id\":8,\"len\":4,\"hex\":\"00080800\"}"), std::string::npos) << result[1];
}

TEST_F(CaptureUnitTest, MediaDataWithPayloadOnRequest)
{
    ASSERT_TRUE(capture::start(path_, true));
    capture::record("in", message(ChannelId::MEDIA_SINK_VIDEO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC, {0x00, 0x00, 0xAB}));

    const auto result = lines();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_NE(result[0].find("\"hex\":\"0000ab\""), std::string::npos) << result[0];
}

TEST_F(CaptureUnitTest, MessengerRecordsSentMessages)
{
    ASSERT_TRUE(capture::start(path_));

    boost::asio::io_service ioService;
    MessageInStreamMock inStreamMock;
    MessageOutStreamMock outStreamMock;
    auto messenger = std::make_shared<Messenger>(ioService, IMessageInStream::Pointer(&inStreamMock, [](auto*) {}),
                                                 IMessageOutStream::Pointer(&outStreamMock, [](auto*) {}));
    EXPECT_CALL(outStreamMock, stream(_, _));

    auto sent = std::make_shared<Message>(ChannelId::INPUT_SOURCE, EncryptionType::ENCRYPTED, MessageType::SPECIFIC);
    sent->insertPayload(common::Data{0x80, 0x01, 0x0A});
    messenger->enqueueSend(sent, SendPromise::defer(ioService));
    ioService.run();

    const auto result = lines();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_NE(result[0].find("\"dir\":\"out\",\"ch\":8,\"chName\":\"INPUT_SOURCE\""), std::string::npos) << result[0];
    EXPECT_NE(result[0].find("\"id\":32769,\"len\":3,\"hex\":\"80010a\""), std::string::npos) << result[0];
}

}
}
}
