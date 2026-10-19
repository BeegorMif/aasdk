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

#include <aap_protobuf/service/control/message/ChannelCloseNotification.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include <aap_protobuf/service/carcontrol/message/SetCarPropertyValueRequest.pb.h>
#include <aap_protobuf/service/carcontrol/message/RegisterCarPropertyListenersRequest.pb.h>
#include <aap_protobuf/service/carcontrol/message/CarActionNotification.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::carcontrol {

  class ICarControlServiceEventHandler {
  public:
    typedef std::shared_ptr<ICarControlServiceEventHandler> Pointer;

    ICarControlServiceEventHandler() = default;

    virtual ~ICarControlServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void onChannelError(const error::Error &e) = 0;

    // The phone closes this channel (protocol 1.6+, #7). The default does nothing; the service keeps
    // receiving by itself, do not call receive() in an override.
    virtual void onChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &) {}

    // Requests of the phone (#40). Defaults do nothing; the service keeps receiving by itself, do not call
    // receive() in an override. Expected answers (ICarControlService):
    //   register listeners -> sendPropertyChangeEvent per property (current value), then
    //                         sendRegisterListenersResponse; later every change as sendPropertyChangeEvent
    //   set value          -> sendPropertyChangeEvent with the new value within 2 seconds, then
    //                         sendSetPropertyValueResponse with the request id

    virtual void onRegisterListenersRequest(
        const aap_protobuf::service::carcontrol::message::RegisterCarPropertyListenersRequest &) {}

    virtual void onSetPropertyValueRequest(
        const aap_protobuf::service::carcontrol::message::SetCarPropertyValueRequest &) {}

    // the user tapped an action control (e.g. LAUNCH_HVAC)
    virtual void onCarActionNotification(
        const aap_protobuf::service::carcontrol::message::CarActionNotification &) {}
  };

}
