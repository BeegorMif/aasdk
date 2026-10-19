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
#include "ICarControlService.hpp"

namespace aasdk::channel::carcontrol {

  class CarControlService
      : public ICarControlService, public Channel, public std::enable_shared_from_this<CarControlService> {
  public:
    CarControlService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger);

    void receive(ICarControlServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

    void sendSetPropertyValueResponse(
        const aap_protobuf::service::carcontrol::message::SetCarPropertyValueResponse &response,
        SendPromise::Pointer promise) override;

    void sendRegisterListenersResponse(
        const aap_protobuf::service::carcontrol::message::RegisterCarPropertyListenersResponse &response,
        SendPromise::Pointer promise) override;

    void sendPropertyChangeEvent(const aap_protobuf::service::carcontrol::message::CarPropertyChangeEvent &event,
                                 SendPromise::Pointer promise) override;

    void sendControlGroupUpdate(const aap_protobuf::service::carcontrol::message::CarControlGroupUpdate &update,
                                SendPromise::Pointer promise) override;

  private:
    using std::enable_shared_from_this<CarControlService>::shared_from_this;

    void messageHandler(messenger::Message::Pointer message, ICarControlServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  ICarControlServiceEventHandler::Pointer eventHandler);

    void sendMessage(uint16_t messageId, const google::protobuf::Message &payload, SendPromise::Pointer promise);

    // parses a request and passes it to the handler; a payload that does not parse is logged and dropped
    template<typename Request>
    void handleRequest(const common::DataConstBuffer &payload, ICarControlServiceEventHandler::Pointer eventHandler,
                       void (ICarControlServiceEventHandler::*callback)(const Request &), const char *name);
  };

}
