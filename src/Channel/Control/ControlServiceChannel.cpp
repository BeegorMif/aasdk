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

#include <cstdlib>
#include <cstdio>
#include <boost/endian/conversion.hpp>
#include <aasdk/Version.hpp>
#include <aasdk/IO/PromiseLink.hpp>
#include <aasdk/Channel/Control/ControlServiceChannel.hpp>
#include <aasdk/Channel/Control/IControlServiceChannelEventHandler.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>


namespace aasdk {
  namespace channel {
    namespace control {

      namespace {
        // AASDK_VERSION=<major>.<minor> asks the phone for another protocol version, for captures of what
        // newer versions change (#14). The phone selects the version it uses in its response.
        std::pair<uint16_t, uint16_t> requestedVersion() {
          const char *value = std::getenv("AASDK_VERSION");
          unsigned major = 0, minor = 0;
          if (value != nullptr && std::sscanf(value, "%u.%u", &major, &minor) == 2 && major <= 0xFFFF && minor <= 0xFFFF) {
            AASDK_LOG(info) << "[ControlServiceChannel] AASDK_VERSION: requesting version " << major << "." << minor;
            return {static_cast<uint16_t>(major), static_cast<uint16_t>(minor)};
          }
          return {AASDK_MAJOR, AASDK_MINOR};
        }
      }

      ControlServiceChannel::ControlServiceChannel(boost::asio::io_service::strand &strand,
                                                   messenger::IMessenger::Pointer messenger)
          : Channel(strand, messenger, messenger::ChannelId::CONTROL) {

      }

      void ControlServiceChannel::sendVersionRequest(SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendVersionRequest()");

        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VERSION_REQUEST).getData());

        common::Data versionBuffer(4, 0);
        const auto version = requestedVersion();
        boost::endian::store_big_u16(&versionBuffer[0], version.first);
        boost::endian::store_big_u16(&versionBuffer[2], version.second);
        message->insertPayload(versionBuffer);
        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendHandshake(common::Data handshakeBuffer, SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendHandshake()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_ENCAPSULATED_SSL).getData());
        message->insertPayload(handshakeBuffer);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendAuthComplete(const aap_protobuf::service::control::message::AuthResponse &response,
                                                   SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendAuthComplete()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUTH_COMPLETE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendServiceDiscoveryResponse(
          const aap_protobuf::service::control::message::ServiceDiscoveryResponse &response,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendServiceDiscoveryResponse()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_RESPONSE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void
      ControlServiceChannel::sendAudioFocusResponse(
          const aap_protobuf::service::control::message::AudioFocusNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendAudioFocusResponse()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUDIO_FOCUS_NOTIFICATION).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendShutdownRequest(
          const aap_protobuf::service::control::message::ByeByeRequest &request,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendShutdownRequest()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_REQUEST).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendShutdownResponse(
          const aap_protobuf::service::control::message::ByeByeResponse &response,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendShutdownResponse()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_RESPONSE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendNavigationFocusResponse(
          const aap_protobuf::service::control::message::NavFocusNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendNavigationFocusResponse()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_NAV_FOCUS_NOTIFICATION).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void
      ControlServiceChannel::sendVoiceSessionFocusResponse(
          const aap_protobuf::service::control::message::VoiceSessionNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendVoiceSessionFocusResponse()");
      }

      void ControlServiceChannel::sendPingResponse(const aap_protobuf::service::control::message::PingResponse &request,
                                                   SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendPingResponse()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_RESPONSE).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendPingRequest(const aap_protobuf::service::control::message::PingRequest &request,
                                                  SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendPingRequest()");
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_REQUEST).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }


      void ControlServiceChannel::sendServiceDiscoveryUpdate(const aap_protobuf::service::control::message::ServiceDiscoveryUpdate &update,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendServiceDiscoveryUpdate()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_UPDATE, update, std::move(promise));
      }

      void ControlServiceChannel::sendCallAvailabilityStatus(const aap_protobuf::service::control::message::CallAvailabilityStatus &status,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendCallAvailabilityStatus()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CALL_AVAILABILITY_STATUS, status, std::move(promise));
      }

      void ControlServiceChannel::sendCarConnectedDevicesRequest(const aap_protobuf::service::control::message::CarConnectedDevicesRequest &request,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendCarConnectedDevicesRequest()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_REQUEST, request, std::move(promise));
      }

      void ControlServiceChannel::sendCarConnectedDevices(const aap_protobuf::service::control::message::CarConnectedDevices &devices,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendCarConnectedDevices()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_RESPONSE, devices, std::move(promise));
      }

      void ControlServiceChannel::sendUserSwitchRequest(const aap_protobuf::service::control::message::UserSwitchRequest &request,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendUserSwitchRequest()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_USER_SWITCH_REQUEST, request, std::move(promise));
      }

      void ControlServiceChannel::sendUserSwitchResponse(const aap_protobuf::service::control::message::UserSwitchResponse &response,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendUserSwitchResponse()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_USER_SWITCH_RESPONSE, response, std::move(promise));
      }

      void ControlServiceChannel::sendChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &notification,
                                       SendPromise::Pointer promise) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "sendChannelCloseNotification()");
        this->sendControlMessage(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_CLOSE_NOTIFICATION, notification, std::move(promise));
      }

      void ControlServiceChannel::sendControlMessage(uint16_t messageId, const google::protobuf::Message &payload,
                                                     SendPromise::Pointer promise) {
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(messenger::MessageId(messageId).getData());
        message->insertPayload(payload);
        this->send(std::move(message), std::move(promise));
      }

      template<typename Proto>
      void ControlServiceChannel::handleNotification(const common::DataConstBuffer &payload,
                                                     IControlServiceChannelEventHandler::Pointer eventHandler,
                                                     void (IControlServiceChannelEventHandler::*callback)(const Proto &),
                                                     const char *name) {
        Proto message;
        if (message.ParseFromArray(payload.cdata, payload.size)) {
          AASDK_LOG(info) << "[ControlServiceChannel] " << name << ": " << message.ShortDebugString();
          ((*eventHandler).*callback)(message);
        } else {
          // informational: a message we cannot read must not end the session
          AASDK_LOG(warning) << "[ControlServiceChannel] " << name << ": payload not readable (" << payload.size << " bytes)";
        }
        this->receive(std::move(eventHandler));
      }

      void ControlServiceChannel::receive(IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "receive()");
        auto receivePromise = messenger::ReceivePromise::defer(strand_);
        receivePromise->then(
            std::bind(&ControlServiceChannel::messageHandler, this->shared_from_this(), std::placeholders::_1,
                      eventHandler),
            std::bind(&IControlServiceChannelEventHandler::onChannelError, eventHandler, std::placeholders::_1));

        messenger_->enqueueReceive(channelId_, std::move(receivePromise));
      }

      void ControlServiceChannel::messageHandler(messenger::Message::Pointer message,
                                                 IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "messageHandler()");

        messenger::MessageId messageId(message->getPayload());
        common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

        AASDK_LOG(debug) << "[ControlServiceChannel] MessageId: " << messageId.getId();

        switch (messageId.getId()) {
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VERSION_RESPONSE:
            this->handleVersionResponse(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_ENCAPSULATED_SSL:
            eventHandler->onHandshake(payload);
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_REQUEST:
            this->handleServiceDiscoveryRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_REQUEST:
            this->handlePingRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_RESPONSE:
            this->handlePingResponse(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUDIO_FOCUS_REQUEST:
            this->handleAudioFocusRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_NAV_FOCUS_REQUEST:
            this->handleNavigationFocusRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VOICE_SESSION_NOTIFICATION:
            this->handleVoiceSessionRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BATTERY_STATUS_NOTIFICATION:
            this->handleBatteryStatusNotification(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_REQUEST:
            this->handleShutdownRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_RESPONSE:
            this->handleShutdownResponse(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_CLOSE_NOTIFICATION:
            this->handleNotification<aap_protobuf::service::control::message::ChannelCloseNotification>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onChannelCloseNotification, "MESSAGE_CHANNEL_CLOSE_NOTIFICATION");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_UPDATE:
            this->handleNotification<aap_protobuf::service::control::message::ServiceDiscoveryUpdate>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onServiceDiscoveryUpdate, "MESSAGE_SERVICE_DISCOVERY_UPDATE");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CALL_AVAILABILITY_STATUS:
            this->handleNotification<aap_protobuf::service::control::message::CallAvailabilityStatus>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onCallAvailabilityStatus, "MESSAGE_CALL_AVAILABILITY_STATUS");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_REQUEST:
            this->handleNotification<aap_protobuf::service::control::message::CarConnectedDevicesRequest>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onCarConnectedDevicesRequest, "MESSAGE_CAR_CONNECTED_DEVICES_REQUEST");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CAR_CONNECTED_DEVICES_RESPONSE:
            this->handleNotification<aap_protobuf::service::control::message::CarConnectedDevices>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onCarConnectedDevices, "MESSAGE_CAR_CONNECTED_DEVICES_RESPONSE");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_USER_SWITCH_REQUEST:
            this->handleNotification<aap_protobuf::service::control::message::UserSwitchRequest>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onUserSwitchRequest, "MESSAGE_USER_SWITCH_REQUEST");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_USER_SWITCH_RESPONSE:
            this->handleNotification<aap_protobuf::service::control::message::UserSwitchResponse>(payload, std::move(eventHandler),
                &IControlServiceChannelEventHandler::onUserSwitchResponse, "MESSAGE_USER_SWITCH_RESPONSE");
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_UNEXPECTED_MESSAGE:
            AASDK_LOG(warning) << "[ControlServiceChannel] the phone reports an unexpected message of ours ("
                               << payload.size << " bytes)";
            eventHandler->onUnexpectedMessage(payload);
            this->receive(std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_FRAMING_ERROR:
            // the phone could not read our framing: the session cannot go on
            AASDK_LOG(error) << "[ControlServiceChannel] the phone reports a framing error";
            eventHandler->onChannelError(error::Error(error::ErrorCode::MESSENGER_PEER_FRAMING_ERROR));
            break;
          default:
            AASDK_LOG(error) << "[ControlServiceChannel] Message Id not Handled: " << messageId.getId();
            this->receive(std::move(eventHandler));
            break;
        }
      }

      void ControlServiceChannel::handleVersionResponse(const common::DataConstBuffer &payload,
                                                        IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleVersionResponse()");

        // major, minor and status, each a big-endian uint16 (#5); VersionResponseOptions may follow
        if (payload.size < 3 * sizeof(uint16_t)) {
          AASDK_LOG(error) << "[ControlServiceChannel] Version response of " << payload.size << " bytes, expected 6";
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
          return;
        }

        if (versionResponseReceived_) {
          // a late answer to an earlier version request, the handshake is already running
          AASDK_LOG(warning) << "[ControlServiceChannel] ignoring another version response";
          this->receive(std::move(eventHandler));
          return;
        }
        versionResponseReceived_ = true;

        const uint16_t major = boost::endian::load_big_u16(payload.cdata);
        const uint16_t minor = boost::endian::load_big_u16(payload.cdata + 2);
        // MessageStatus is signed: 0xFFFF is STATUS_NO_COMPATIBLE_VERSION (-1)
        const auto status = static_cast<aap_protobuf::shared::MessageStatus>(
            static_cast<int16_t>(boost::endian::load_big_u16(payload.cdata + 4)));
        AASDK_LOG(info) << "[ControlServiceChannel] Handling Version - Major: " << major << " Minor: " << minor
                        << " Status: " << status;

        if (payload.size > 3 * sizeof(uint16_t)) {
          aap_protobuf::service::control::message::VersionResponseOptions options;
          if (options.ParseFromArray(payload.cdata + 6, static_cast<int>(payload.size - 6))) {
            AASDK_LOG(info) << "[ControlServiceChannel] version response options: " << options.ShortDebugString();
            eventHandler->onVersionResponseOptions(options);
          } else {
            AASDK_LOG(warning) << "[ControlServiceChannel] version response: " << payload.size - 6
                               << " trailing bytes are no VersionResponseOptions";
          }
        }

        eventHandler->onVersionResponse(major, minor, status);
      }

      void ControlServiceChannel::handleServiceDiscoveryRequest(const common::DataConstBuffer &payload,
                                                                IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleServiceDiscoveryRequest()");
        aap_protobuf::service::control::message::ServiceDiscoveryRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onServiceDiscoveryRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleAudioFocusRequest(const common::DataConstBuffer &payload,
                                                          IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleAudioFocusRequest()");
        aap_protobuf::service::control::message::AudioFocusRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onAudioFocusRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleVoiceSessionRequest(const common::DataConstBuffer &payload,
                                                            IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleVoiceSessionRequest()");
        aap_protobuf::service::control::message::VoiceSessionNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onVoiceSessionRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleBatteryStatusNotification(const common::DataConstBuffer &payload,
                                                            IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleBatteryStatusNotification()");

        aap_protobuf::service::control::message::BatteryStatusNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onBatteryStatusNotification(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleShutdownRequest(const common::DataConstBuffer &payload,
                                                        IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleShutdownRequest()");
        aap_protobuf::service::control::message::ByeByeRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onByeByeRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleShutdownResponse(const common::DataConstBuffer &payload,
                                                         IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleShutdownResponse()");
        aap_protobuf::service::control::message::ByeByeResponse response;
        if (response.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onByeByeResponse(response);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleNavigationFocusRequest(const common::DataConstBuffer &payload,
                                                               IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handleNavigationFocusRequest()");
        aap_protobuf::service::control::message::NavFocusRequestNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onNavigationFocusRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handlePingRequest(const common::DataConstBuffer &payload,
                                                    IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handlePingRequest()");
        aap_protobuf::service::control::message::PingRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onPingRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handlePingResponse(const common::DataConstBuffer &payload,
                                                     IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG_CHANNEL_CONTROL(debug, "handlePingResponse()");
        aap_protobuf::service::control::message::PingResponse response;
        if (response.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onPingResponse(response);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

    }
  }
}
