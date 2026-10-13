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

#pragma once

#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Data.hpp>
#include <aap_protobuf/service/control/message/AudioFocusRequest.pb.h>
#include <aap_protobuf/service//control/message/BatteryStatusNotification.pb.h>
#include <aap_protobuf/service/control/message/ByeByeRequest.pb.h>
#include <aap_protobuf/service/control/message/ByeByeResponse.pb.h>
#include <aap_protobuf/service/control/message/NavFocusRequestNotification.pb.h>
#include <aap_protobuf/service/control/message/PingRequest.pb.h>
#include <aap_protobuf/service/control/message/PingResponse.pb.h>
#include <aap_protobuf/service/control/message/ServiceDiscoveryRequest.pb.h>
#include <aap_protobuf/service/control/message/VoiceSessionNotification.pb.h>
#include <aap_protobuf/shared/MessageStatus.pb.h>
#include <aap_protobuf/service/control/message/CallAvailabilityStatus.pb.h>
#include <aap_protobuf/service/control/message/CarConnectedDevices.pb.h>
#include <aap_protobuf/service/control/message/CarConnectedDevicesRequest.pb.h>
#include <aap_protobuf/service/control/message/ChannelCloseNotification.pb.h>
#include <aap_protobuf/service/control/message/ServiceDiscoveryUpdate.pb.h>
#include <aap_protobuf/service/control/message/UserSwitchRequest.pb.h>
#include <aap_protobuf/service/control/message/UserSwitchResponse.pb.h>
#include <aap_protobuf/service/control/message/VersionResponseOptions.pb.h>

namespace aasdk::channel::control {

  class IControlServiceChannelEventHandler {
  public:
    typedef std::shared_ptr<IControlServiceChannelEventHandler> Pointer;

    IControlServiceChannelEventHandler() = default;

    virtual ~IControlServiceChannelEventHandler() = default;

    virtual void onVersionResponse(uint16_t majorCode, uint16_t minorCode,
                                   aap_protobuf::shared::MessageStatus status) = 0;

    virtual void onHandshake(const common::DataConstBuffer &payload) = 0;

    virtual void onServiceDiscoveryRequest(
        const aap_protobuf::service::control::message::ServiceDiscoveryRequest &request) = 0;

    virtual void
    onAudioFocusRequest(const aap_protobuf::service::control::message::AudioFocusRequest &request) = 0;

    virtual void onByeByeRequest(const aap_protobuf::service::control::message::ByeByeRequest &request) = 0;

    virtual void
    onByeByeResponse(const aap_protobuf::service::control::message::ByeByeResponse &response) = 0;

    virtual void onBatteryStatusNotification(const aap_protobuf::service::control::message::BatteryStatusNotification &notification) = 0;

    virtual void
    onNavigationFocusRequest(
        const aap_protobuf::service::control::message::NavFocusRequestNotification &request) = 0;

    virtual void
    onVoiceSessionRequest(const aap_protobuf::service::control::message::VoiceSessionNotification &request) = 0;

    virtual void onPingRequest(const aap_protobuf::service::control::message::PingRequest &request) = 0;

    virtual void onPingResponse(const aap_protobuf::service::control::message::PingResponse &response) = 0;

    virtual void onChannelError(const error::Error &e) = 0;

    // Messages of protocol 1.6 and later (#7). The defaults do nothing. After each of these callbacks
    // ControlServiceChannel keeps receiving by itself: do not call receive() in an override.

    // Options the phone appends to its version response (ping and wireless TCP configuration, #5);
    // called before onVersionResponse.
    virtual void onVersionResponseOptions(const aap_protobuf::service::control::message::VersionResponseOptions &) {}

    virtual void onChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &) {}

    virtual void onServiceDiscoveryUpdate(const aap_protobuf::service::control::message::ServiceDiscoveryUpdate &) {}

    virtual void onCallAvailabilityStatus(const aap_protobuf::service::control::message::CallAvailabilityStatus &) {}

    virtual void onCarConnectedDevicesRequest(const aap_protobuf::service::control::message::CarConnectedDevicesRequest &) {}

    virtual void onCarConnectedDevices(const aap_protobuf::service::control::message::CarConnectedDevices &) {}

    virtual void onUserSwitchRequest(const aap_protobuf::service::control::message::UserSwitchRequest &) {}

    virtual void onUserSwitchResponse(const aap_protobuf::service::control::message::UserSwitchResponse &) {}

    // The phone reports a message of ours as unexpected (MESSAGE_UNEXPECTED_MESSAGE), payload as sent
    virtual void onUnexpectedMessage(const common::DataConstBuffer &) {}


  };
}


