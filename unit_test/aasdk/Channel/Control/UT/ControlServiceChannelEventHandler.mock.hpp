#pragma once

#include <gmock/gmock.h>
#include <aasdk/Channel/Control/IControlServiceChannelEventHandler.hpp>


namespace aasdk
{
namespace channel
{
namespace control
{
namespace ut
{

class ControlServiceChannelEventHandlerMock: public IControlServiceChannelEventHandler
{
public:
    MOCK_METHOD3(onVersionResponse, void(uint16_t majorCode, uint16_t minorCode, aap_protobuf::shared::MessageStatus status));
    MOCK_METHOD1(onHandshake, void(const common::DataConstBuffer& payload));
    MOCK_METHOD1(onServiceDiscoveryRequest, void(const aap_protobuf::service::control::message::ServiceDiscoveryRequest& request));
    MOCK_METHOD1(onAudioFocusRequest, void(const aap_protobuf::service::control::message::AudioFocusRequest& request));
    MOCK_METHOD1(onByeByeRequest, void(const aap_protobuf::service::control::message::ByeByeRequest& request));
    MOCK_METHOD1(onByeByeResponse, void(const aap_protobuf::service::control::message::ByeByeResponse& response));
    MOCK_METHOD1(onBatteryStatusNotification, void(const aap_protobuf::service::control::message::BatteryStatusNotification& notification));
    MOCK_METHOD1(onNavigationFocusRequest, void(const aap_protobuf::service::control::message::NavFocusRequestNotification& request));
    MOCK_METHOD1(onVoiceSessionRequest, void(const aap_protobuf::service::control::message::VoiceSessionNotification& request));
    MOCK_METHOD1(onPingRequest, void(const aap_protobuf::service::control::message::PingRequest& request));
    MOCK_METHOD1(onPingResponse, void(const aap_protobuf::service::control::message::PingResponse& response));
    MOCK_METHOD1(onChannelError, void(const error::Error& e));
};

}
}
}
}
