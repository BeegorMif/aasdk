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

#include "aasdk/Channel/Channel.hpp"
#include "IRadioService.hpp"

namespace aasdk::channel::radio {


  class RadioService
      : public IRadioService, public Channel, public std::enable_shared_from_this<RadioService> {
  public:
    RadioService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger);

    // Senders and Receivers

    void receive(IRadioServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

    void sendProgramListNotification(
        const aap_protobuf::service::radio::message::RadioProgramListNotification &notification,
        SendPromise::Pointer promise) override;

    void sendProgramInfoNotification(
        const aap_protobuf::service::radio::message::RadioProgramInfoNotification &notification,
        SendPromise::Pointer promise) override;

    void sendMuteResponse(const aap_protobuf::service::radio::message::RadioMuteResponse &response,
                          SendPromise::Pointer promise) override;

    void sendTuneResponse(const aap_protobuf::service::radio::message::RadioTuneResponse &response,
                          SendPromise::Pointer promise) override;

    void sendFavoriteListNotification(
        const aap_protobuf::service::radio::message::RadioFavoriteListNotification &notification,
        SendPromise::Pointer promise) override;


  private:
    using std::enable_shared_from_this<RadioService>::shared_from_this;

    void messageHandler(messenger::Message::Pointer message, IRadioServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  IRadioServiceEventHandler::Pointer eventHandler);

    void sendMessage(uint16_t messageId, const google::protobuf::Message &payload, SendPromise::Pointer promise);

    // parses a v2 request and passes it to the handler; a payload that does not parse is logged and dropped
    template<typename Request>
    void handleRequest(const common::DataConstBuffer &payload, IRadioServiceEventHandler::Pointer eventHandler,
                       void (IRadioServiceEventHandler::*callback)(const Request &), const char *name);

  };

}
