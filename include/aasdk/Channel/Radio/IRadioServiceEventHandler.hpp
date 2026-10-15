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
#include <aap_protobuf/service/radio/message/RadioMuteRequest.pb.h>
#include <aap_protobuf/service/radio/message/RadioTuneRequest.pb.h>
#include <aap_protobuf/service/radio/message/RadioFavoriteToggleRequest.pb.h>
#include <aap_protobuf/service/radio/message/RadioTuneDirectionRequest.pb.h>
#include <aap_protobuf/service/radio/message/RadioSearchRequest.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::radio {


  class IRadioServiceEventHandler {
  public:
    typedef std::shared_ptr<IRadioServiceEventHandler> Pointer;

    IRadioServiceEventHandler() = default;

    virtual ~IRadioServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void onChannelError(const error::Error &e) = 0;


    // The phone closes this channel (protocol 1.6+, #7). The default does nothing; the service keeps

    // receiving by itself, do not call receive() in an override.

    virtual void onChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &) {}

    // Radio v2 requests of the phone (Android Auto 16.2+, #32). Defaults do nothing; the service keeps receiving by
    // itself, do not call receive() in an override. Expected answers (IRadioService):
    //   mute            -> sendMuteResponse
    //   tune, direction -> sendTuneResponse, then sendProgramInfoNotification (the phone waits 6 seconds)
    //   favorite toggle -> sendFavoriteListNotification (applies to the current station)

    virtual void onMuteRequest(const aap_protobuf::service::radio::message::RadioMuteRequest &) {}

    virtual void onTuneRequest(const aap_protobuf::service::radio::message::RadioTuneRequest &) {}

    virtual void onFavoriteToggleRequest(const aap_protobuf::service::radio::message::RadioFavoriteToggleRequest &) {}

    virtual void onTuneDirectionRequest(const aap_protobuf::service::radio::message::RadioTuneDirectionRequest &) {}

    // custom action of the phone's radio browser, e.g. RadioBandGroup.band_scan_action
    virtual void onSearchRequest(const aap_protobuf::service::radio::message::RadioSearchRequest &) {}
  };

}
