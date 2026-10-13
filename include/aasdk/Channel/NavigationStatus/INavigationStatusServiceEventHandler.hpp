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

#include <aasdk/Common/Data.hpp>
#include <aap_protobuf/service/control/message/ChannelCloseNotification.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationNextTurnDistanceEvent.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationNextTurnEvent.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationStatus.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationState.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationCurrentPosition.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::navigationstatus {

  class INavigationStatusServiceEventHandler {
  public:
    typedef std::shared_ptr<INavigationStatusServiceEventHandler> Pointer;

    INavigationStatusServiceEventHandler() = default;

    virtual ~INavigationStatusServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void onChannelError(const error::Error &e) = 0;


    // The phone closes this channel (protocol 1.6+, #7). The default does nothing; the service keeps

    // receiving by itself, do not call receive() in an override.

    virtual void onChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &) {}


    // Sent by the phone with protocol >= 5.1 when navigation starts; payload not decoded yet (openhu#54).

    // Default: nothing, the service keeps receiving by itself.

    virtual void onVehicleEnergyForecast(const common::DataConstBuffer &) {}

    virtual void onStatusUpdate(const aap_protobuf::service::navigationstatus::message::NavigationStatus &navStatus) = 0;

    virtual void onTurnEvent(const aap_protobuf::service::navigationstatus::message::NavigationNextTurnEvent &turnEvent) = 0;

    virtual void
    onDistanceEvent(const aap_protobuf::service::navigationstatus::message::NavigationNextTurnDistanceEvent &distanceEvent) = 0;

    // Current protocol (replaces the deprecated turn/distance events): upcoming steps and destinations
    virtual void onNavigationState(const aap_protobuf::service::navigationstatus::message::NavigationState &state) = 0;

    // Current protocol: distance/time to the next step and to the destinations
    virtual void
    onCurrentPosition(const aap_protobuf::service::navigationstatus::message::NavigationCurrentPosition &position) = 0;
  };

}


