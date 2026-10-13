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

// Messages from the peer that must be rejected instead of crashing or growing without limit
// (average-web-dev/aasdk#1, #2). A real MessageInStream is fed frame by frame through the transport mock.

#include <gtest/gtest.h>
#include <aasdk/Transport/UT/Transport.mock.hpp>
#include <aasdk/Transport/UT/SSLWrapper.mock.hpp>
#include <aasdk/Messenger/UT/Cryptor.mock.hpp>
#include <aasdk/Messenger/UT/MessageOutStream.mock.hpp>
#include <aasdk/Messenger/Promise.hpp>
#include <aasdk/Messenger/MessageInStream.hpp>
#include <aasdk/Messenger/Messenger.hpp>
#include <aasdk/Messenger/MessageId.hpp>
#include <aasdk/Messenger/Cryptor.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::SaveArg;
using ::testing::Return;

struct ReceiveResult
{
    bool resolved = false;
    bool rejected = false;
    Message::Pointer message;
    error::Error error;
};

class MessageInStreamValidationUnitTest : public testing::Test
{
protected:
    MessageInStreamValidationUnitTest()
        : transport_(&transportMock_, [](auto*) {})
        , cryptor_(&cryptorMock_, [](auto*) {})
    {
        EXPECT_CALL(transportMock_, receive(_, _)).WillRepeatedly(DoAll(SaveArg<0>(&pendingSize_), SaveArg<1>(&pending_)));
    }

    ReceivePromise::Pointer promise(ReceiveResult& result)
    {
        auto promise = ReceivePromise::defer(ioService_);
        promise->then([&result](Message::Pointer message) { result.resolved = true; result.message = std::move(message); },
                      [&result](const error::Error& e) { result.rejected = true; result.error = e; });
        return promise;
    }

    void run()
    {
        ioService_.run();
        ioService_.reset();
    }

    // Resolves the transport read that is pending with data
    void feed(common::Data data)
    {
        ASSERT_NE(pending_, nullptr);
        auto pending = std::move(pending_);
        pending_.reset();
        pending->resolve(std::move(data));
        run();
    }

    void sendFrame(FrameType type, const common::Data& payload, uint32_t totalSize = 0,
                   EncryptionType encryption = EncryptionType::PLAIN, ChannelId channel = ChannelId::CONTROL)
    {
        feed(FrameHeader(channel, type, encryption, MessageType::SPECIFIC).getData());
        feed(type == FrameType::FIRST ? FrameSize(payload.size(), totalSize).getData() : FrameSize(payload.size()).getData());
        if(pending_ != nullptr)
        {
            feed(payload);
        }
    }

    boost::asio::io_service ioService_;
    transport::ut::TransportMock transportMock_;
    transport::ITransport::Pointer transport_;
    CryptorMock cryptorMock_;
    ICryptor::Pointer cryptor_;
    transport::ITransport::ReceivePromise::Pointer pending_;
    size_t pendingSize_ = 0;
};

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_MessageMatchingItsTotalSize)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    sendFrame(FrameType::FIRST, common::Data(1000, 0x01), 3000);
    sendFrame(FrameType::MIDDLE, common::Data(1000, 0x02));
    sendFrame(FrameType::LAST, common::Data(1000, 0x03));

    ASSERT_TRUE(result.resolved);
    EXPECT_EQ(result.message->getPayload().size(), 3000u);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_RejectPayloadBeyondTotalSize)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    const common::Data chunk(0x4000, 0xAB);
    sendFrame(FrameType::FIRST, chunk, 0x4000 + 16);
    sendFrame(FrameType::MIDDLE, chunk);

    EXPECT_FALSE(result.resolved);
    ASSERT_TRUE(result.rejected);
    EXPECT_EQ(result.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE);
    EXPECT_EQ(pending_, nullptr);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_RejectHugeTotalSize)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    sendFrame(FrameType::FIRST, common::Data(100, 0x01), 0xFFFFFFFFu);

    ASSERT_TRUE(result.rejected);
    EXPECT_EQ(result.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE);
    EXPECT_EQ(pending_, nullptr) << "the payload of the rejected frame was still requested";
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_RejectWhenAllPartialMessagesExceedLimit)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    // every channel stays below its announced size, together they exceed cMaxBufferedSize
    const common::Data chunk(0xFFFF, 0xAB);
    const uint32_t total = MessageInStream::cMaxMessageSize;
    const auto channels = {ChannelId::CONTROL, ChannelId::BLUETOOTH, ChannelId::MEDIA_SINK_VIDEO};
    for(auto channel: channels)
    {
        sendFrame(FrameType::FIRST, chunk, total, EncryptionType::PLAIN, channel);
    }
    for(size_t i = 0; !result.rejected && i < 2 * MessageInStream::cMaxBufferedSize / chunk.size(); ++i)
    {
        for(auto channel: channels)
        {
            if(!result.rejected)
            {
                sendFrame(FrameType::MIDDLE, chunk, 0, EncryptionType::PLAIN, channel);
            }
        }
    }

    ASSERT_TRUE(result.rejected);
    EXPECT_EQ(result.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_EncryptedFramesAgainstPlainTotalSize)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    // the encrypted frames are larger than the plain text the sender announces as total size
    EXPECT_CALL(cryptorMock_, decrypt(_, _, _)).Times(2).WillRepeatedly(Invoke(
        [](common::Data& output, const common::DataConstBuffer& buffer, int) {
            output.insert(output.end(), buffer.size - 29, 0x42);
            return buffer.size - 29;
        }));
    sendFrame(FrameType::FIRST, common::Data(529, 0x01), 1000, EncryptionType::ENCRYPTED);
    sendFrame(FrameType::LAST, common::Data(529, 0x02), 0, EncryptionType::ENCRYPTED);

    ASSERT_TRUE(result.resolved);
    EXPECT_EQ(result.message->getPayload().size(), 1000u);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_RejectMessageWithoutId)
{
    for(size_t size: {0, 1})
    {
        auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
        ReceiveResult result;
        inStream->startReceive(promise(result));
        run();

        sendFrame(FrameType::BULK, common::Data(size, 0x00));

        EXPECT_FALSE(result.resolved) << size;
        ASSERT_TRUE(result.rejected) << size;
        EXPECT_EQ(result.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_SHORT);
    }
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_SecondReceiveKeepsTheRunningOne)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult first;
    ReceiveResult second;
    inStream->startReceive(promise(first));
    run();
    inStream->startReceive(promise(second));
    run();

    ASSERT_TRUE(second.rejected);
    EXPECT_EQ(second.error.getCode(), error::ErrorCode::OPERATION_IN_PROGRESS);

    sendFrame(FrameType::BULK, common::Data{0x00, 0x01, 0x02});
    ASSERT_TRUE(first.resolved);
    EXPECT_EQ(first.message->getPayload().size(), 3u);
}

TEST_F(MessageInStreamValidationUnitTest, Messenger_TwoReceivesOnTheSameChannel)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    MessageOutStreamMock outStreamMock;
    IMessageOutStream::Pointer outStream(&outStreamMock, [](auto*) {});
    auto messenger = std::make_shared<Messenger>(ioService_, inStream, outStream);

    ReceiveResult first;
    ReceiveResult second;
    messenger->enqueueReceive(ChannelId::CONTROL, promise(first));
    run();
    messenger->enqueueReceive(ChannelId::CONTROL, promise(second));
    run();
    EXPECT_FALSE(first.rejected);
    EXPECT_FALSE(second.rejected);

    sendFrame(FrameType::BULK, common::Data{0x00, 0x01});
    ASSERT_TRUE(first.resolved);
    EXPECT_FALSE(second.resolved);

    sendFrame(FrameType::BULK, common::Data{0x00, 0x02});
    ASSERT_TRUE(second.resolved);
    EXPECT_EQ(second.message->getPayload()[1], 0x02);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_RejectLastFrameBelowTotalSize)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult result;
    inStream->startReceive(promise(result));
    run();

    // the FIRST frame announced 3000 bytes, the message ends after 2000 (#12)
    sendFrame(FrameType::FIRST, common::Data(1000, 0x01), 3000);
    sendFrame(FrameType::LAST, common::Data(1000, 0x03));

    EXPECT_FALSE(result.resolved);
    ASSERT_TRUE(result.rejected);
    EXPECT_EQ(result.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_SHORT);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_NoReadAfterRejectedFrame)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult first;
    inStream->startReceive(promise(first));
    run();

    // rejected before its payload was read: the payload is still in the transport
    sendFrame(FrameType::FIRST, common::Data(100, 0x01), 0xFFFFFFFFu);
    ASSERT_TRUE(first.rejected);
    ASSERT_EQ(pending_, nullptr);

    // the next receive would read that payload as a frame header (#12)
    ReceiveResult second;
    inStream->startReceive(promise(second));
    run();

    EXPECT_EQ(pending_, nullptr) << "the stream was read after a rejected frame";
    ASSERT_TRUE(second.rejected);
    EXPECT_EQ(second.error.getCode(), error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE);
}

TEST_F(MessageInStreamValidationUnitTest, MessageInStream_NoReadAfterIntertwinedFrame)
{
    auto inStream = std::make_shared<MessageInStream>(ioService_, transport_, cryptor_);
    ReceiveResult first;
    inStream->startReceive(promise(first));
    run();

    // a LAST frame without FIRST is rejected after its header only
    feed(FrameHeader(ChannelId::CONTROL, FrameType::LAST, EncryptionType::PLAIN, MessageType::SPECIFIC).getData());
    ASSERT_TRUE(first.rejected);
    EXPECT_EQ(first.error.getCode(), error::ErrorCode::MESSENGER_INTERTWINED_CHANNELS);

    ReceiveResult second;
    inStream->startReceive(promise(second));
    run();

    EXPECT_EQ(pending_, nullptr) << "the stream was read after a rejected frame";
    ASSERT_TRUE(second.rejected);
    EXPECT_EQ(second.error.getCode(), error::ErrorCode::MESSENGER_INTERTWINED_CHANNELS);
}

TEST(MessageIdValidationUnitTest, MessageId_ShortPayload)
{
    EXPECT_THROW(MessageId(common::Data()), error::Error);
    EXPECT_THROW(MessageId(common::Data{0x42}), error::Error);
    EXPECT_EQ(MessageId(common::Data{0x80, 0x07, 0x99}).getId(), 0x8007);
}

TEST(FrameSizeValidationUnitTest, FrameSize_ParseExtended)
{
    const common::Data data = FrameSize(0x1234, 0xA1B2C3D4).getData();
    const FrameSize frameSize{common::DataConstBuffer(data)};
    EXPECT_EQ(frameSize.getFrameSize(), 0x1234u);
    EXPECT_EQ(frameSize.getTotalSize(), 0xA1B2C3D4u);
}

TEST(CryptorValidationUnitTest, Cryptor_DecryptFrameEndingInsideRecord)
{
    // OpenSSL keeps the bytes of an unfinished record and asks for more (#6)
    auto sslWrapper = std::make_shared<NiceMock<transport::ut::SSLWrapperMock>>();
    ON_CALL(*sslWrapper, bioWrite(_, _, _)).WillByDefault(testing::ReturnArg<2>());
    ON_CALL(*sslWrapper, sslRead(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*sslWrapper, getError(_, -1)).WillByDefault(Return(SSL_ERROR_WANT_READ));
    EXPECT_CALL(*sslWrapper, hasPending(_)).WillOnce(Return(true));

    Cryptor cryptor(sslWrapper);
    common::Data output;
    const common::Data frame(10, 0x17);
    try
    {
        cryptor.decrypt(output, common::DataConstBuffer(frame), static_cast<int>(frame.size()));
        FAIL() << "no error for a frame that ends inside a TLS record";
    }
    catch(const error::Error& e)
    {
        EXPECT_EQ(e.getCode(), error::ErrorCode::MESSENGER_FRAME_TOO_SHORT);
    }
    EXPECT_TRUE(output.empty());
}

}
}
}
