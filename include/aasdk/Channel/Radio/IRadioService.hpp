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
#include <aap_protobuf/service/radio/message/RadioProgramListNotification.pb.h>
#include <aap_protobuf/service/radio/message/RadioProgramInfoNotification.pb.h>
#include <aap_protobuf/service/radio/message/RadioMuteResponse.pb.h>
#include <aap_protobuf/service/radio/message/RadioTuneResponse.pb.h>
#include <aap_protobuf/service/radio/message/RadioFavoriteListNotification.pb.h>
#include "IRadioServiceEventHandler.hpp"

namespace aasdk::channel::radio {

  class IRadioService : public virtual IChannel {
  public:
    typedef std::shared_ptr<IRadioService> Pointer;

    IRadioService() = default;

    virtual ~IRadioService() = default;

    virtual void receive(IRadioServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    // Radio v2 (Android Auto 16.2+, #32): state of the head unit's tuner. The phone expects the station list,
    // the current station and the favorites right after the channel is open, and again whenever they change.

    virtual void sendProgramListNotification(
        const aap_protobuf::service::radio::message::RadioProgramListNotification &notification,
        SendPromise::Pointer promise) = 0;

    virtual void sendProgramInfoNotification(
        const aap_protobuf::service::radio::message::RadioProgramInfoNotification &notification,
        SendPromise::Pointer promise) = 0;

    virtual void sendMuteResponse(const aap_protobuf::service::radio::message::RadioMuteResponse &response,
                                  SendPromise::Pointer promise) = 0;

    virtual void sendTuneResponse(const aap_protobuf::service::radio::message::RadioTuneResponse &response,
                                  SendPromise::Pointer promise) = 0;

    virtual void sendFavoriteListNotification(
        const aap_protobuf::service::radio::message::RadioFavoriteListNotification &notification,
        SendPromise::Pointer promise) = 0;
  };
}
