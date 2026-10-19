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

#include <memory>
#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include "aasdk/Messenger/ChannelId.hpp"
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include <aap_protobuf/service/carcontrol/message/SetCarPropertyValueResponse.pb.h>
#include <aap_protobuf/service/carcontrol/message/RegisterCarPropertyListenersResponse.pb.h>
#include <aap_protobuf/service/carcontrol/message/CarPropertyChangeEvent.pb.h>
#include <aap_protobuf/service/carcontrol/message/CarControlGroupUpdate.pb.h>
#include "ICarControlServiceEventHandler.hpp"

namespace aasdk::channel::carcontrol {

  // Car control channel (Service field 15, #40): the phone's climate rail reads and writes properties of the car.
  class ICarControlService : public virtual IChannel {
  public:
    typedef std::shared_ptr<ICarControlService> Pointer;

    ICarControlService() = default;

    virtual ~ICarControlService() = default;

    virtual void receive(ICarControlServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void sendSetPropertyValueResponse(
        const aap_protobuf::service::carcontrol::message::SetCarPropertyValueResponse &response,
        SendPromise::Pointer promise) = 0;

    virtual void sendRegisterListenersResponse(
        const aap_protobuf::service::carcontrol::message::RegisterCarPropertyListenersResponse &response,
        SendPromise::Pointer promise) = 0;

    // current value of a registered property; status AVAILABLE with a value, otherwise the phone shows it as off
    virtual void sendPropertyChangeEvent(
        const aap_protobuf::service::carcontrol::message::CarPropertyChangeEvent &event,
        SendPromise::Pointer promise) = 0;

    // replaces the control group of the same type on the phone (new layout)
    virtual void sendControlGroupUpdate(
        const aap_protobuf::service::carcontrol::message::CarControlGroupUpdate &update,
        SendPromise::Pointer promise) = 0;
  };
}
