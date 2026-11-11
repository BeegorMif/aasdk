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
#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include "aasdk/Messenger/ChannelId.hpp"
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackStatus.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackMetadata.pb.h>
#include "ICarLocalMediaServiceEventHandler.hpp"

namespace aasdk::channel::carlocalmedia {

  // Car local media channel (Service field 16, average-web-dev/openhu#147): a source of the head unit (radio, USB
  // music) in the phone's media card, with title, artist, cover and buttons. The phone shows it while the status is
  // PLAYING (it moves to the top of the phone's media sessions) or PAUSED. No browsing.
  class ICarLocalMediaService : public virtual IChannel {
  public:
    typedef std::shared_ptr<ICarLocalMediaService> Pointer;

    ICarLocalMediaService() = default;
    virtual ~ICarLocalMediaService() = default;

    virtual void receive(ICarLocalMediaServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    // state, source name, position (s) and the buttons the phone shows
    virtual void sendPlaybackStatus(
        const aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackStatus &status,
        SendPromise::Pointer promise) = 0;

    // title, artist, album, cover (image bytes), duration
    virtual void sendPlaybackMetadata(
        const aap_protobuf::service::carlocalmedia::message::CarLocalMediaPlaybackMetadata &metadata,
        SendPromise::Pointer promise) = 0;
  };

}
