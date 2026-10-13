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

#include <aasdk/Messenger/MessageInStream.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>
#include <iostream>


namespace aasdk::messenger {

  MessageInStream::MessageInStream(boost::asio::io_service &ioService, transport::ITransport::Pointer transport,
                                   ICryptor::Pointer cryptor)
      : strand_(ioService), transport_(std::move(transport)), cryptor_(std::move(cryptor)), isBroken_(false) {

  }

  void MessageInStream::startReceive(ReceivePromise::Pointer promise) {
    AASDK_LOG_MESSENGER(debug, "startReceiveCalled()");
    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
      if (isBroken_) {
        // the position of the next frame header is unknown, nothing after the error can be read (#12)
        promise->reject(brokenError_);
      } else if (promise_ == nullptr) {
        promise_ = std::move(promise);
        auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
        transportPromise->then(
            [this, self = this->shared_from_this()](common::Data data) mutable {
              this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
            },
            [this, self = this->shared_from_this()](const error::Error &e) mutable {
              AASDK_LOG_MESSENGER(debug, "Rejecting message.");
              this->rejectMessage(e);
            });

        transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
      } else {
        // promise_ belongs to the receive that is still running, only the new one is rejected
        AASDK_LOG_MESSENGER(debug, "Already Handling Promise");
        promise->reject(error::Error(error::ErrorCode::OPERATION_IN_PROGRESS));
      }
    });
  }

  void MessageInStream::receiveFrameHeaderHandler(const common::DataConstBuffer &buffer) {
    FrameHeader frameHeader(buffer);

    AASDK_LOG(debug) << "[MessageInStream] Processing Frame Header: Ch "
                     << channelIdToString(frameHeader.getChannelId()) << " Fr "
                     << frameTypeToString(frameHeader.getType());

    isValidFrame_ = true;

    auto bufferedMessage = messageBuffer_.find(frameHeader.getChannelId());
    if (bufferedMessage != messageBuffer_.end()) {
      // We have found a message...
      message_ = std::move(bufferedMessage->second);
      messageBuffer_.erase(bufferedMessage);

      AASDK_LOG_MESSENGER(debug, "Found existing message.");

      if (frameHeader.getType() == FrameType::FIRST || frameHeader.getType() == FrameType::BULK) {
        // If it's first or bulk, we need to override the message anyhow, so we will start again.
        // Need to start a new message anyhow
        announcedSize_.erase(frameHeader.getChannelId());
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(),
                                             frameHeader.getMessageType());
      }
    } else {
      AASDK_LOG_MESSENGER(debug, "Could not find existing message.");
      // No Message Found in Buffers and this is a middle or last frame, this an error.
      if (frameHeader.getType() == FrameType::MIDDLE || frameHeader.getType() == FrameType::LAST) {
        AASDK_LOG_MESSENGER(debug, "Rejecting intertwined channel frame.");
        message_.reset();
        this->rejectMessage(error::Error(error::ErrorCode::MESSENGER_INTERTWINED_CHANNELS));
        return;
      }

      message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(),
                                           frameHeader.getMessageType());
    }

    thisFrameType_ = frameHeader.getType();
    const size_t frameSize = FrameSize::getSizeOf(
        frameHeader.getType() == FrameType::FIRST ? FrameSizeType::EXTENDED : FrameSizeType::SHORT);

    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
          this->receiveFrameSizeHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error &e) mutable {
          AASDK_LOG_MESSENGER(debug, "Rejecting message.");
          this->rejectMessage(e);
        });

    transport_->receive(frameSize, std::move(transportPromise));
  }

  void MessageInStream::receiveFrameSizeHandler(const common::DataConstBuffer &buffer) {
    FrameSize frameSize(buffer);
    if (thisFrameType_ == FrameType::FIRST) {
      if (frameSize.getTotalSize() > cMaxMessageSize) {
        AASDK_LOG(error) << "[MessageInStream] Rejecting message of " << frameSize.getTotalSize() << " bytes on channel "
                         << channelIdToString(message_->getChannelId());
        this->rejectMessage(error::Error(error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE));
        return;
      }
      announcedSize_[message_->getChannelId()] = frameSize.getTotalSize();
    }

    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
          this->receiveFramePayloadHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error &e) mutable {
          AASDK_LOG_MESSENGER(debug, "Rejecting message.");
          this->rejectMessage(e);
        });

    frameSize_ = (int) frameSize.getFrameSize();
    transport_->receive(frameSize.getFrameSize(), std::move(transportPromise));
  }

  void MessageInStream::receiveFramePayloadHandler(const common::DataConstBuffer &buffer) {
    if (message_->getEncryptionType() == EncryptionType::ENCRYPTED) {
      try {
        cryptor_->decrypt(message_->getPayload(), buffer, frameSize_);
      }
      catch (const error::Error &e) {
        AASDK_LOG_MESSENGER(debug, "Rejecting message.");
        this->rejectMessage(e);
        return;
      }
    } else {
      message_->insertPayload(buffer);
    }

    const auto channelId = message_->getChannelId();
    const size_t payloadSize = message_->getPayload().size();
    if (thisFrameType_ != FrameType::BULK) {
      // Compared with the decrypted size: the sender announces the size of the plain message
      // (MessageOutStream does the same), the encrypted frames are larger.
      const auto announced = announcedSize_.find(channelId);
      if (announced == announcedSize_.end() || payloadSize > announced->second
          || this->bufferedSize() + payloadSize > cMaxBufferedSize) {
        AASDK_LOG(error) << "[MessageInStream] Rejecting message on channel " << channelIdToString(channelId)
                         << ": " << payloadSize << " bytes received, announced "
                         << (announced == announcedSize_.end() ? 0 : announced->second);
        this->rejectMessage(error::Error(error::ErrorCode::MESSENGER_MESSAGE_TOO_LARGE));
        return;
      }
    }

    bool isResolved = false;

    // If this is the LAST frame or a BULK frame...
    if ((thisFrameType_ == FrameType::BULK || thisFrameType_ == FrameType::LAST) && isValidFrame_) {
      if (thisFrameType_ == FrameType::LAST && payloadSize != announcedSize_[channelId]) {
        // the LAST frame must complete exactly the size the FIRST frame announced (#12)
        AASDK_LOG(error) << "[MessageInStream] Rejecting message on channel " << channelIdToString(channelId)
                         << ": " << payloadSize << " bytes received, announced " << announcedSize_[channelId];
        this->rejectMessage(error::Error(error::ErrorCode::MESSENGER_MESSAGE_TOO_SHORT));
        return;
      }
      announcedSize_.erase(channelId);
      // Every message starts with its 2 byte id, the channels read it unchecked
      if (payloadSize < sizeof(uint16_t)) {
        AASDK_LOG(error) << "[MessageInStream] Rejecting message of " << payloadSize << " bytes on channel "
                         << channelIdToString(channelId);
        this->rejectMessage(error::Error(error::ErrorCode::MESSENGER_MESSAGE_TOO_SHORT));
        return;
      }
      AASDK_LOG_MESSENGER(debug, "Resolving message.");
      promise_->resolve(std::move(message_));
      promise_.reset();
      isResolved = true;

    } else {
      // First or Middle message, we'll store in our buffer...
      messageBuffer_[message_->getChannelId()] = std::move(message_);
    }

    // If the main promise isn't resolved, then carry on retrieving frame headers.
    if (!isResolved) {
      auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
      transportPromise->then(
          [this, self = this->shared_from_this()](common::Data data) mutable {
            this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
          },
          [this, self = this->shared_from_this()](const error::Error &e) mutable {
            AASDK_LOG_MESSENGER(debug, "Rejecting message.");
            this->rejectMessage(e);
          });

      transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
    }
  }

  void MessageInStream::rejectMessage(const error::Error &e) {
    // A rejected frame may not have been read to its end, so the stream cannot be read any further (#12)
    isBroken_ = true;
    brokenError_ = e;
    announcedSize_.clear();
    messageBuffer_.clear();
    message_.reset();
    promise_->reject(e);
    promise_.reset();
  }

  size_t MessageInStream::bufferedSize() const {
    size_t size = 0;
    for (const auto &buffered: messageBuffer_) {
      size += buffered.second->getPayload().size();
    }
    return size;
  }

}

