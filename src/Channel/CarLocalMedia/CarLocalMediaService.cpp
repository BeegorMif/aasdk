// This file is part of aasdk library project.
// Copyright (C) 2026 Leon Strauß
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

#include <aap_protobuf/service/carlocalmedia/CarLocalMediaMessageId.pb.h>
#include <aasdk/Channel/CarLocalMedia/ICarLocalMediaServiceEventHandler.hpp>
#include <aasdk/Channel/CarLocalMedia/CarLocalMediaService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk::channel::carlocalmedia {

  using aap_protobuf::service::carlocalmedia::CarLocalMediaMessageId;

  CarLocalMediaService::CarLocalMediaService(boost::asio::io_service::strand &strand,
                                             messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::CAR_LOCAL_MEDIA) {
  }

  void CarLocalMediaService::receive(ICarLocalMediaServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&CarLocalMediaService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&ICarLocalMediaServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));
    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void CarLocalMediaService::sendChannelOpenResponse(
      const aap_protobuf::service::control::message::ChannelOpenResponse &response, SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);
    this->send(std::move(message), std::move(promise));
  }

  void CarLocalMediaService::sendPlaybackStatus(
      const aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackStatus &status,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "sendPlaybackStatus()");
    this->sendMessage(CarLocalMediaMessageId::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_STATUS, status, std::move(promise));
  }

  void CarLocalMediaService::sendPlaybackMetadata(
      const aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackMetadata &metadata,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "sendPlaybackMetadata()");
    this->sendMessage(CarLocalMediaMessageId::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_METADATA, metadata,
                      std::move(promise));
  }

  void CarLocalMediaService::sendMessage(uint16_t messageId, const google::protobuf::Message &payload,
                                         SendPromise::Pointer promise) {
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(messageId).getData());
    message->insertPayload(payload);
    this->send(std::move(message), std::move(promise));
  }

  void CarLocalMediaService::messageHandler(messenger::Message::Pointer message,
                                            ICarLocalMediaServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "messageHandler()");
    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case CarLocalMediaMessageId::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_REQUEST: {
        aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "onPlaybackRequest");
          eventHandler->onPlaybackRequest(request);
        } else {
          AASDK_LOG(warning) << "[CarLocalMediaService] playback request not readable";
        }
        this->receive(std::move(eventHandler));
        break;
      }
      // messages the head unit sends: not expected from the phone
      case CarLocalMediaMessageId::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_STATUS:
      case CarLocalMediaMessageId::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_METADATA:
        AASDK_LOG(warning) << "[CarLocalMediaService] unexpected head unit message from the phone: "
                           << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_CLOSE_NOTIFICATION: {
        aap_protobuf::service::control::message::ChannelCloseNotification notification;
        notification.ParseFromArray(payload.cdata, payload.size);
        AASDK_LOG(info) << "[" << messenger::channelIdToString(channelId_) << "] channel close notification";
        eventHandler->onChannelCloseNotification(notification);
        this->receive(std::move(eventHandler));
        break;
      }
      default:
        AASDK_LOG(error) << "[CarLocalMediaService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void CarLocalMediaService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                      ICarLocalMediaServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_LOCAL_MEDIA(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

}
