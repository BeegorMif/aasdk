/*
*  This file is part of aasdk library project.
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

// Service channels in 2.0: channel close notification on every channel, new messages of #10 and the
// vehicle energy forecast (#7, #10, openhu#54); radio v2 (#32); car control (#40)

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <aasdk/Messenger/UT/Messenger.mock.hpp>
#include <aasdk/Messenger/MessageId.hpp>
#include <aasdk/Channel/Promise.hpp>
#include <aasdk/Channel/SensorSource/SensorSourceService.hpp>
#include <aasdk/Channel/MediaSink/Video/VideoMediaSinkService.hpp>
#include <aasdk/Channel/MediaSink/Audio/AudioMediaSinkService.hpp>
#include <aasdk/Channel/InputSource/InputSourceService.hpp>
#include <aasdk/Channel/NavigationStatus/NavigationStatusService.hpp>
#include <aasdk/Channel/MediaBrowser/MediaBrowserService.hpp>
#include <aasdk/Channel/Radio/RadioService.hpp>
#include <aasdk/Channel/CarControl/CarControlService.hpp>
#include <aap_protobuf/service/carcontrol/CarControlMessageId.pb.h>
#include <aap_protobuf/service/carcontrol/message/VehiclePropertyId.pb.h>
#include <aap_protobuf/service/radio/RadioMessageId.pb.h>
#include <aap_protobuf/service/media/sink/MediaMessageId.pb.h>
#include <aap_protobuf/service/sensorsource/SensorMessageId.pb.h>
#include <aap_protobuf/service/inputsource/InputMessageId.pb.h>
#include <aap_protobuf/service/navigationstatus/NavigationStatusMessageId.pb.h>


namespace aasdk
{
namespace channel
{
namespace ut
{

using ::testing::_;
using ::testing::NiceMock;
using ::testing::SaveArg;
namespace pb = aap_protobuf::service;

class SensorHandlerMock: public sensorsource::ISensorSourceServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onSensorStartRequest, void(const pb::sensorsource::message::SensorRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
    MOCK_METHOD1(onChannelCloseNotification, void(const pb::control::message::ChannelCloseNotification&));
};

class VideoHandlerMock: public mediasink::video::IVideoMediaSinkServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onMediaChannelSetupRequest, void(const pb::media::shared::message::Setup&));
    MOCK_METHOD1(onMediaChannelStartIndication, void(const pb::media::shared::message::Start&));
    MOCK_METHOD1(onMediaChannelStopIndication, void(const pb::media::shared::message::Stop&));
    MOCK_METHOD2(onMediaWithTimestampIndication, void(messenger::Timestamp::ValueType, const common::DataConstBuffer&));
    MOCK_METHOD1(onMediaIndication, void(const common::DataConstBuffer&));
    MOCK_METHOD1(onVideoFocusRequest, void(const pb::media::video::message::VideoFocusRequestNotification&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
    MOCK_METHOD1(onChannelCloseNotification, void(const pb::control::message::ChannelCloseNotification&));
    MOCK_METHOD1(onUpdateUiConfigReply, void(const pb::control::message::UpdateUiConfigReply&));
};

class InputHandlerMock: public inputsource::IInputSourceServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onKeyBindingRequest, void(const pb::media::sink::message::KeyBindingRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
    MOCK_METHOD1(onInputFeedback, void(const pb::inputsource::message::InputFeedback&));
};

class NavigationHandlerMock: public navigationstatus::INavigationStatusServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
    MOCK_METHOD1(onStatusUpdate, void(const pb::navigationstatus::message::NavigationStatus&));
    MOCK_METHOD1(onTurnEvent, void(const pb::navigationstatus::message::NavigationNextTurnEvent&));
    MOCK_METHOD1(onDistanceEvent, void(const pb::navigationstatus::message::NavigationNextTurnDistanceEvent&));
    MOCK_METHOD1(onNavigationState, void(const pb::navigationstatus::message::NavigationState&));
    MOCK_METHOD1(onCurrentPosition, void(const pb::navigationstatus::message::NavigationCurrentPosition&));
    MOCK_METHOD1(onVehicleEnergyForecast, void(const common::DataConstBuffer&));
};

class MediaBrowserHandlerMock: public mediabrowser::IMediaBrowserServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
};

// an application that knows nothing about radio v2 (OpenHU today)
class RadioMinimalHandlerMock: public radio::IRadioServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
};

class RadioHandlerMock: public RadioMinimalHandlerMock
{
public:
    MOCK_METHOD1(onMuteRequest, void(const pb::radio::message::RadioMuteRequest&));
    MOCK_METHOD1(onTuneRequest, void(const pb::radio::message::RadioTuneRequest&));
    MOCK_METHOD1(onFavoriteToggleRequest, void(const pb::radio::message::RadioFavoriteToggleRequest&));
    MOCK_METHOD1(onTuneDirectionRequest, void(const pb::radio::message::RadioTuneDirectionRequest&));
    MOCK_METHOD1(onSearchRequest, void(const pb::radio::message::RadioSearchRequest&));
};

class CarControlMinimalHandlerMock: public carcontrol::ICarControlServiceEventHandler
{
public:
    MOCK_METHOD1(onChannelOpenRequest, void(const pb::control::message::ChannelOpenRequest&));
    MOCK_METHOD1(onChannelError, void(const error::Error&));
};

class CarControlHandlerMock: public CarControlMinimalHandlerMock
{
public:
    MOCK_METHOD1(onRegisterListenersRequest, void(const pb::carcontrol::message::RegisterCarPropertyListenersRequest&));
    MOCK_METHOD1(onSetPropertyValueRequest, void(const pb::carcontrol::message::SetCarPropertyValueRequest&));
    MOCK_METHOD1(onCarActionNotification, void(const pb::carcontrol::message::CarActionNotification&));
};

class ServiceChannelsUnitTest : public testing::Test
{
protected:
    ServiceChannelsUnitTest()
        : strand_(ioService_)
        , messenger_(std::make_shared<NiceMock<messenger::ut::MessengerMock>>())
    {
    }

    // Delivers one message to the service and returns how often the service asked for the next one
    template<typename Service, typename Handler>
    int deliver(Service& service, std::shared_ptr<Handler> handler, messenger::ChannelId channel, uint16_t messageId,
                const common::Data& payload, messenger::MessageType type = messenger::MessageType::SPECIFIC)
    {
        std::vector<messenger::ReceivePromise::Pointer> promises;
        ON_CALL(*messenger_, enqueueReceive(_, _)).WillByDefault(
            [&promises](messenger::ChannelId, messenger::ReceivePromise::Pointer promise) { promises.push_back(std::move(promise)); });
        service->receive(handler);
        if(promises.size() != 1)
        {
            ADD_FAILURE() << "receive() did not ask the messenger";
            return -1;
        }

        auto message = std::make_shared<messenger::Message>(channel, messenger::EncryptionType::ENCRYPTED, type);
        message->insertPayload(messenger::MessageId(messageId).getData());
        message->insertPayload(payload);
        promises.front()->resolve(std::move(message));
        ioService_.run();
        ioService_.restart();
        return static_cast<int>(promises.size()) - 1;
    }

    // id of the message a sender put on the messenger (payload behind the id in *payload)
    uint16_t sentId(const std::function<void(SendPromise::Pointer)>& send, common::Data* payload = nullptr)
    {
        messenger::Message::Pointer message;
        EXPECT_CALL(*messenger_, enqueueSend(_, _)).WillOnce(SaveArg<0>(&message));
        send(SendPromise::defer(ioService_));
        testing::Mock::VerifyAndClearExpectations(messenger_.get());
        if(message == nullptr || message->getPayload().size() < 2)
        {
            ADD_FAILURE() << "nothing sent";
            return 0;
        }
        if(payload != nullptr)
        {
            payload->assign(message->getPayload().begin() + 2, message->getPayload().end());
        }
        return message->getPayload()[0] << 8 | message->getPayload()[1];
    }

    boost::asio::io_service ioService_;
    boost::asio::io_service::strand strand_;
    std::shared_ptr<NiceMock<messenger::ut::MessengerMock>> messenger_;
};

constexpr uint16_t cChannelClose = pb::control::message::ControlMessageType::MESSAGE_CHANNEL_CLOSE_NOTIFICATION;

TEST_F(ServiceChannelsUnitTest, ChannelClose_PassedOnAndKeepsReceiving)
{
    auto sensor = std::make_shared<sensorsource::SensorSourceService>(strand_, messenger_);
    auto sensorHandler = std::make_shared<testing::StrictMock<SensorHandlerMock>>();
    EXPECT_CALL(*sensorHandler, onChannelCloseNotification(_));
    EXPECT_EQ(deliver(sensor, sensorHandler, messenger::ChannelId::SENSOR, cChannelClose, {}, messenger::MessageType::CONTROL), 1);

    auto video = std::make_shared<mediasink::video::VideoMediaSinkService>(strand_, messenger_, messenger::ChannelId::MEDIA_SINK_VIDEO);
    auto videoHandler = std::make_shared<testing::StrictMock<VideoHandlerMock>>();
    EXPECT_CALL(*videoHandler, onChannelCloseNotification(_));
    EXPECT_EQ(deliver(video, videoHandler, messenger::ChannelId::MEDIA_SINK_VIDEO, cChannelClose, {}, messenger::MessageType::CONTROL), 1);
}

TEST_F(ServiceChannelsUnitTest, ChannelClose_DefaultKeepsReceiving)
{
    // an application that does not override the callback (OpenHU today)
    auto input = std::make_shared<inputsource::InputSourceService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<InputHandlerMock>>();
    EXPECT_EQ(deliver(input, handler, messenger::ChannelId::INPUT_SOURCE, cChannelClose, {}, messenger::MessageType::CONTROL), 1);
}

TEST_F(ServiceChannelsUnitTest, MediaBrowser_ChannelOpenDoesNotFallThrough)
{
    // the channel open request fell through into the media browser ids and then into "not handled"
    auto browser = std::make_shared<mediabrowser::MediaBrowserService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<MediaBrowserHandlerMock>>();
    EXPECT_CALL(*handler, onChannelOpenRequest(_));
    EXPECT_EQ(deliver(browser, handler, messenger::ChannelId::MEDIA_BROWSER,
                      pb::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST, {0x08, 0x00, 0x10, 0x0F},
                      messenger::MessageType::CONTROL), 0);
}

TEST_F(ServiceChannelsUnitTest, Video_UpdateUiConfig)
{
    auto video = std::make_shared<mediasink::video::VideoMediaSinkService>(strand_, messenger_, messenger::ChannelId::MEDIA_SINK_VIDEO);
    EXPECT_EQ(sentId([&](auto p) { video->sendUpdateUiConfigRequest(pb::control::message::UpdateUiConfigRequest(), std::move(p)); }),
              pb::media::sink::MediaMessageId::MEDIA_MESSAGE_UPDATE_UI_CONFIG_REQUEST);

    auto handler = std::make_shared<testing::StrictMock<VideoHandlerMock>>();
    EXPECT_CALL(*handler, onUpdateUiConfigReply(_));
    EXPECT_EQ(deliver(video, handler, messenger::ChannelId::MEDIA_SINK_VIDEO,
                      pb::media::sink::MediaMessageId::MEDIA_MESSAGE_UPDATE_UI_CONFIG_REPLY, {}), 1);
}

TEST_F(ServiceChannelsUnitTest, Audio_UnderflowNotification)
{
    auto audio = std::make_shared<mediasink::audio::AudioMediaSinkService>(strand_, messenger_, messenger::ChannelId::MEDIA_SINK_MEDIA_AUDIO);
    pb::media::sink::message::AudioUnderflowNotification notification;
    notification.set_session_id(1);
    EXPECT_EQ(sentId([&](auto p) { audio->sendAudioUnderflowNotification(notification, std::move(p)); }),
              pb::media::sink::MediaMessageId::MEDIA_MESSAGE_AUDIO_UNDERFLOW_NOTIFICATION);
}

TEST_F(ServiceChannelsUnitTest, Sensor_Error)
{
    auto sensor = std::make_shared<sensorsource::SensorSourceService>(strand_, messenger_);
    pb::sensorsource::message::SensorError error;
    error.set_sensor_type(pb::sensorsource::message::SensorType::SENSOR_SPEED);
    error.set_sensor_error_type(pb::sensorsource::message::SensorErrorType::SENSOR_ERROR_TRANSIENT);
    EXPECT_EQ(sentId([&](auto p) { sensor->sendSensorError(error, std::move(p)); }),
              pb::sensorsource::SensorMessageId::SENSOR_MESSAGE_ERROR);
}

TEST_F(ServiceChannelsUnitTest, Input_Feedback)
{
    auto input = std::make_shared<inputsource::InputSourceService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<InputHandlerMock>>();
    EXPECT_CALL(*handler, onInputFeedback(_));
    EXPECT_EQ(deliver(input, handler, messenger::ChannelId::INPUT_SOURCE,
                      pb::inputsource::InputMessageId::INPUT_MESSAGE_INPUT_FEEDBACK, {}), 1);
}

TEST_F(ServiceChannelsUnitTest, Navigation_VehicleEnergyForecast)
{
    // as captured with protocol 5.1 (phone selected 6.1): "0a 00"
    auto navigation = std::make_shared<navigationstatus::NavigationStatusService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<NavigationHandlerMock>>();
    common::Data forecast;
    EXPECT_CALL(*handler, onVehicleEnergyForecast(_)).WillOnce(
        [&forecast](const common::DataConstBuffer& buffer) { forecast.assign(buffer.cdata, buffer.cdata + buffer.size); });
    EXPECT_EQ(deliver(navigation, handler, messenger::ChannelId::NAVIGATION_STATUS,
                      pb::navigationstatus::NavigationStatusMessageId::INSTRUMENT_CLUSTER_VEHICLE_ENERGY_FORECAST, {0x0A, 0x00}), 1);
    EXPECT_EQ(forecast, (common::Data{0x0A, 0x00}));
}


TEST_F(ServiceChannelsUnitTest, Radio_SendsV2Messages)
{
    auto radio = std::make_shared<radio::RadioService>(strand_, messenger_);
    namespace msg = pb::radio::message;

    msg::RadioProgramInfoNotification info;
    auto* identifier = info.mutable_program_info()->mutable_program_selector()->mutable_primary_identifier();
    identifier->set_type(msg::RADIO_IDENTIFIER_TYPE_AMFM_FREQUENCY);
    identifier->set_value(96200);
    info.mutable_program_info()->mutable_metadata()->set_station_name("NDR 2");
    info.set_is_muted(false);
    info.set_is_favorite(true);
    common::Data payload;
    EXPECT_EQ(sentId([&](auto p) { radio->sendProgramInfoNotification(info, std::move(p)); }, &payload),
              pb::radio::RadioMessageId::RADIO_MESSAGE_PROGRAM_INFO_NOTIFICATION);
    msg::RadioProgramInfoNotification parsed;
    ASSERT_TRUE(parsed.ParseFromArray(payload.data(), static_cast<int>(payload.size())));
    EXPECT_EQ(parsed.program_info().program_selector().primary_identifier().value(), 96200u);
    EXPECT_EQ(parsed.program_info().metadata().station_name(), "NDR 2");
    EXPECT_TRUE(parsed.is_favorite());

    EXPECT_EQ(sentId([&](auto p) { radio->sendProgramListNotification(msg::RadioProgramListNotification(), std::move(p)); }),
              0x801A);
    msg::RadioMuteResponse mute;
    mute.set_is_muted(true);
    EXPECT_EQ(sentId([&](auto p) { radio->sendMuteResponse(mute, std::move(p)); }, &payload), 0x801D);
    EXPECT_EQ(payload, (common::Data{0x08, 0x01}));
    msg::RadioTuneResponse tune;
    tune.set_status(msg::RADIO_TUNE_STATUS_SUCCESS);
    EXPECT_EQ(sentId([&](auto p) { radio->sendTuneResponse(tune, std::move(p)); }, &payload), 0x801F);
    EXPECT_EQ(payload, (common::Data{0x08, 0x01}));
    EXPECT_EQ(sentId([&](auto p) { radio->sendFavoriteListNotification(msg::RadioFavoriteListNotification(), std::move(p)); }),
              0x8020);
}

TEST_F(ServiceChannelsUnitTest, Radio_V2RequestsReachTheHandler)
{
    auto radio = std::make_shared<radio::RadioService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<RadioHandlerMock>>();
    const auto channel = messenger::ChannelId::RADIO;

    EXPECT_CALL(*handler, onMuteRequest(testing::Property(&pb::radio::message::RadioMuteRequest::mute, true)));
    EXPECT_EQ(deliver(radio, handler, channel, 0x801C, {0x08, 0x01}), 1);

    // FM 101.9 MHz as kHz: 101900 = 0x18E0C
    pb::radio::message::RadioTuneRequest tune;
    EXPECT_CALL(*handler, onTuneRequest(_)).WillOnce(SaveArg<0>(&tune));
    EXPECT_EQ(deliver(radio, handler, channel, 0x801E, {0x0A, 0x08, 0x0A, 0x06, 0x08, 0x01, 0x10, 0x8C, 0x9C, 0x06}), 1);
    EXPECT_EQ(tune.program_selector().primary_identifier().type(),
              pb::radio::message::RADIO_IDENTIFIER_TYPE_AMFM_FREQUENCY);
    EXPECT_EQ(tune.program_selector().primary_identifier().value(), 101900u);

    EXPECT_CALL(*handler, onFavoriteToggleRequest(
        testing::Property(&pb::radio::message::RadioFavoriteToggleRequest::is_favorite, true)));
    EXPECT_EQ(deliver(radio, handler, channel, 0x8021, {0x08, 0x01}), 1);

    EXPECT_CALL(*handler, onTuneDirectionRequest(testing::Property(
        &pb::radio::message::RadioTuneDirectionRequest::direction, pb::radio::message::RADIO_TUNE_DIRECTION_DOWN)));
    EXPECT_EQ(deliver(radio, handler, channel, 0x8022, {0x08, 0x02}), 1);

    EXPECT_CALL(*handler, onSearchRequest(
        testing::Property(&pb::radio::message::RadioSearchRequest::action_id, "scan")));
    EXPECT_EQ(deliver(radio, handler, channel, 0x8023, {0x0A, 0x04, 's', 'c', 'a', 'n'}), 1);
}

TEST_F(ServiceChannelsUnitTest, Radio_DefaultsAndUnexpectedMessagesKeepReceiving)
{
    auto radio = std::make_shared<radio::RadioService>(strand_, messenger_);
    auto minimal = std::make_shared<testing::StrictMock<RadioMinimalHandlerMock>>();
    // v2 request without an override, a v1 id, a head unit message sent by the phone
    EXPECT_EQ(deliver(radio, minimal, messenger::ChannelId::RADIO, 0x801C, {0x08, 0x01}), 1);
    EXPECT_EQ(deliver(radio, minimal, messenger::ChannelId::RADIO,
                      pb::radio::RadioMessageId::RADIO_MESSAGE_MUTE_RADIO_REQUEST, {}), 1);
    EXPECT_EQ(deliver(radio, minimal, messenger::ChannelId::RADIO, 0x801B, {}), 1);

    // a request that does not parse is dropped, the channel stays open
    auto handler = std::make_shared<testing::StrictMock<RadioHandlerMock>>();
    EXPECT_EQ(deliver(radio, handler, messenger::ChannelId::RADIO, 0x801E, {0x0A, 0x7F}), 1);
}

TEST_F(ServiceChannelsUnitTest, CarControl_SendsMessages)
{
    auto carControl = std::make_shared<carcontrol::CarControlService>(strand_, messenger_);
    namespace msg = pb::carcontrol::message;
    EXPECT_EQ(carControl->getId(), messenger::ChannelId::CAR_CONTROL);

    msg::CarPropertyChangeEvent event;
    event.mutable_property()->set_property_id(msg::HVAC_AC_ON);
    event.mutable_property()->mutable_area()->set_global(msg::GLOBAL_AREA);
    event.mutable_value()->set_bool_value(true);
    event.set_status(msg::CAR_PROPERTY_STATUS_AVAILABLE);
    common::Data payload;
    EXPECT_EQ(sentId([&](auto p) { carControl->sendPropertyChangeEvent(event, std::move(p)); }, &payload), 0x8005);
    msg::CarPropertyChangeEvent parsed;
    ASSERT_TRUE(parsed.ParseFromArray(payload.data(), static_cast<int>(payload.size())));
    EXPECT_TRUE(parsed.value().bool_value());

    msg::SetCarPropertyValueResponse response;
    response.set_status(aap_protobuf::shared::MessageStatus::STATUS_SUCCESS);
    response.set_request_id("e8c20e00-d3d6-4bd1-94a0-7c16972207ae");
    EXPECT_EQ(sentId([&](auto p) { carControl->sendSetPropertyValueResponse(response, std::move(p)); }), 0x8002);
    EXPECT_EQ(sentId([&](auto p) {
        carControl->sendRegisterListenersResponse(msg::RegisterCarPropertyListenersResponse(), std::move(p)); }), 0x8004);
    EXPECT_EQ(sentId([&](auto p) { carControl->sendControlGroupUpdate(msg::CarControlGroupUpdate(), std::move(p)); }),
              0x8007);
}

TEST_F(ServiceChannelsUnitTest, CarControl_RequestsReachTheHandler)
{
    auto carControl = std::make_shared<carcontrol::CarControlService>(strand_, messenger_);
    auto handler = std::make_shared<testing::StrictMock<CarControlHandlerMock>>();
    const auto channel = messenger::ChannelId::CAR_CONTROL;
    namespace msg = pb::carcontrol::message;

    EXPECT_CALL(*handler, onChannelOpenRequest(_));
    EXPECT_EQ(deliver(carControl, handler, channel, pb::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST,
                      {0x08, 0x00, 0x10, 0x13}, messenger::MessageType::CONTROL), 0);

    // HVAC_AC_ON (354419973), global area
    msg::RegisterCarPropertyListenersRequest registration;
    EXPECT_CALL(*handler, onRegisterListenersRequest(_)).WillOnce(SaveArg<0>(&registration));
    EXPECT_EQ(deliver(carControl, handler, channel, 0x8003,
                      {0x0A, 0x0A, 0x08, 0x85, 0x8A, 0x80, 0xA9, 0x01, 0x12, 0x02, 0x08, 0x00}), 1);
    ASSERT_EQ(registration.properties_size(), 1);
    EXPECT_EQ(registration.properties(0).property_id(), msg::HVAC_AC_ON);

    EXPECT_CALL(*handler, onSetPropertyValueRequest(testing::Property(
        &msg::SetCarPropertyValueRequest::request_id, "1")));
    EXPECT_EQ(deliver(carControl, handler, channel, 0x8001, {0x1A, 0x01, '1'}), 1);

    msg::CarActionNotification action;
    EXPECT_CALL(*handler, onCarActionNotification(_)).WillOnce(SaveArg<0>(&action));
    EXPECT_EQ(deliver(carControl, handler, channel, 0x8006, {0x0A, 0x02, 0x08, 0x01}), 1);
    EXPECT_EQ(action.action().action_id(), msg::CAR_ACTION_LAUNCH_HVAC);
}

TEST_F(ServiceChannelsUnitTest, CarControl_DefaultsAndUnexpectedMessagesKeepReceiving)
{
    auto carControl = std::make_shared<carcontrol::CarControlService>(strand_, messenger_);
    auto minimal = std::make_shared<testing::StrictMock<CarControlMinimalHandlerMock>>();
    const auto channel = messenger::ChannelId::CAR_CONTROL;
    // a request without an override, a head unit message sent by the phone, an unknown id
    EXPECT_EQ(deliver(carControl, minimal, channel, 0x8006, {0x0A, 0x02, 0x08, 0x01}), 1);
    EXPECT_EQ(deliver(carControl, minimal, channel, 0x8005, {}), 1);
    EXPECT_EQ(deliver(carControl, minimal, channel, 0x8010, {}), 1);

    // a request that does not parse is dropped, the channel stays open
    auto handler = std::make_shared<testing::StrictMock<CarControlHandlerMock>>();
    EXPECT_EQ(deliver(carControl, handler, channel, 0x8001, {0x0A, 0x7F}), 1);
}

}
}
}
