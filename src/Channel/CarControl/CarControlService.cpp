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

#include <aap_protobuf/service/carcontrol/CarControlMessageId.pb.h>
#include <aasdk/Channel/CarControl/ICarControlServiceEventHandler.hpp>
#include <aasdk/Channel/CarControl/CarControlService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk::channel::carcontrol {

  using aap_protobuf::service::carcontrol::CarControlMessageId;

  CarControlService::CarControlService(boost::asio::io_service::strand &strand,
                                       messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::CAR_CONTROL) {
  }

  void CarControlService::receive(ICarControlServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&CarControlService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&ICarControlServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void CarControlService::sendChannelOpenResponse(
      const aap_protobuf::service::control::message::ChannelOpenResponse &response, SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void CarControlService::sendSetPropertyValueResponse(
      const aap_protobuf::service::carcontrol::message::SetCarPropertyValueResponse &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "sendSetPropertyValueResponse()");
    this->sendMessage(CarControlMessageId::CAR_CONTROL_MESSAGE_SET_CAR_PROPERTY_VALUE_RESPONSE, response,
                      std::move(promise));
  }

  void CarControlService::sendRegisterListenersResponse(
      const aap_protobuf::service::carcontrol::message::RegisterCarPropertyListenersResponse &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "sendRegisterListenersResponse()");
    this->sendMessage(CarControlMessageId::CAR_CONTROL_MESSAGE_REGISTER_CAR_PROPERTY_LISTENERS_RESPONSE, response,
                      std::move(promise));
  }

  void CarControlService::sendPropertyChangeEvent(
      const aap_protobuf::service::carcontrol::message::CarPropertyChangeEvent &event, SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "sendPropertyChangeEvent()");
    this->sendMessage(CarControlMessageId::CAR_CONTROL_MESSAGE_CAR_PROPERTY_CHANGE_EVENT, event, std::move(promise));
  }

  void CarControlService::sendControlGroupUpdate(
      const aap_protobuf::service::carcontrol::message::CarControlGroupUpdate &update, SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "sendControlGroupUpdate()");
    this->sendMessage(CarControlMessageId::CAR_CONTROL_MESSAGE_CAR_CONTROL_GROUP_UPDATE, update, std::move(promise));
  }

  void CarControlService::sendMessage(uint16_t messageId, const google::protobuf::Message &payload,
                                      SendPromise::Pointer promise) {
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(messageId).getData());
    message->insertPayload(payload);
    this->send(std::move(message), std::move(promise));
  }

  template<typename Request>
  void CarControlService::handleRequest(const common::DataConstBuffer &payload,
                                        ICarControlServiceEventHandler::Pointer eventHandler,
                                        void (ICarControlServiceEventHandler::*callback)(const Request &),
                                        const char *name) {
    Request request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      AASDK_LOG_CHANNEL_CAR_CONTROL(debug, name);
      ((*eventHandler).*callback)(request);
    } else {
      AASDK_LOG(warning) << "[CarControlService] " << name << " not readable";
    }
  }

  void CarControlService::messageHandler(messenger::Message::Pointer message,
                                         ICarControlServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case CarControlMessageId::CAR_CONTROL_MESSAGE_REGISTER_CAR_PROPERTY_LISTENERS_REQUEST:
        this->handleRequest(payload, eventHandler, &ICarControlServiceEventHandler::onRegisterListenersRequest,
                            "onRegisterListenersRequest");
        this->receive(std::move(eventHandler));
        break;
      case CarControlMessageId::CAR_CONTROL_MESSAGE_SET_CAR_PROPERTY_VALUE_REQUEST:
        this->handleRequest(payload, eventHandler, &ICarControlServiceEventHandler::onSetPropertyValueRequest,
                            "onSetPropertyValueRequest");
        this->receive(std::move(eventHandler));
        break;
      case CarControlMessageId::CAR_CONTROL_MESSAGE_CAR_ACTION_NOTIFICATION:
        this->handleRequest(payload, eventHandler, &ICarControlServiceEventHandler::onCarActionNotification,
                            "onCarActionNotification");
        this->receive(std::move(eventHandler));
        break;
      // messages the head unit sends: not expected from the phone
      case CarControlMessageId::CAR_CONTROL_MESSAGE_SET_CAR_PROPERTY_VALUE_RESPONSE:
      case CarControlMessageId::CAR_CONTROL_MESSAGE_REGISTER_CAR_PROPERTY_LISTENERS_RESPONSE:
      case CarControlMessageId::CAR_CONTROL_MESSAGE_CAR_PROPERTY_CHANGE_EVENT:
      case CarControlMessageId::CAR_CONTROL_MESSAGE_CAR_CONTROL_GROUP_UPDATE:
        AASDK_LOG(warning) << "[CarControlService] unexpected head unit message from the phone: "
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
        AASDK_LOG(error) << "[CarControlService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void CarControlService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                   ICarControlServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_CAR_CONTROL(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }
}
