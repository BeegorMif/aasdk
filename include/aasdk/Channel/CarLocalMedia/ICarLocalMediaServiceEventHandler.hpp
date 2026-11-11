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

#pragma once

#include <memory>
#include <aap_protobuf/service/control/message/ChannelCloseNotification.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackRequest.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::carlocalmedia {

  class ICarLocalMediaServiceEventHandler {
  public:
    typedef std::shared_ptr<ICarLocalMediaServiceEventHandler> Pointer;

    ICarLocalMediaServiceEventHandler() = default;
    virtual ~ICarLocalMediaServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;
    virtual void onChannelError(const error::Error &e) = 0;
    // The phone closes this channel (protocol 1.6+). The default does nothing; the service keeps receiving by
    // itself, do not call receive() in an override.
    virtual void onChannelCloseNotification(const aap_protobuf::service::control::message::ChannelCloseNotification &) {}
    // A button in the phone's media card (play, pause, previous, next, stop); no reply. The default does nothing;
    // the service keeps receiving by itself, do not call receive() in an override.
    virtual void onPlaybackRequest(
        const aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackRequest &) {}
  };

}
