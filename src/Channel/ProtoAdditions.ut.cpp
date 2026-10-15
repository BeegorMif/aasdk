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

// Fields and enum values added from other Android Auto projects (average-web-dev/aasdk#11, #32)

#include <gtest/gtest.h>
#include <aap_protobuf/service/control/message/HeadUnitInfo.pb.h>
#include <aap_protobuf/service/control/message/ServiceDiscoveryResponse.pb.h>
#include <aap_protobuf/service/control/message/WirelessTcpConfiguration.pb.h>
#include <aap_protobuf/service/inputsource/message/PointerAction.pb.h>
#include <aap_protobuf/service/media/sink/MediaMessageId.pb.h>
#include <aap_protobuf/service/media/video/message/VideoFocusReason.pb.h>
#include <aap_protobuf/service/navigationstatus/NavigationStatusMessageId.pb.h>
#include <aap_protobuf/service/sensorsource/message/SensorType.pb.h>
#include <aap_protobuf/service/radio/RadioMessageId.pb.h>
#include <aap_protobuf/service/radio/RadioService.pb.h>


namespace aasdk
{
namespace channel
{
namespace ut
{

using namespace aap_protobuf::service;

std::string bytes(std::initializer_list<uint8_t> list)
{
    return std::string(list.begin(), list.end());
}

TEST(ProtoAdditionsUnitTest, EnumValues)
{
    EXPECT_EQ(inputsource::message::PointerAction::ACTION_CANCEL, 3);
    EXPECT_EQ(inputsource::message::PointerAction::ACTION_OUTSIDE, 4);
    EXPECT_EQ(media::video::message::VideoFocusReason::LAST_MODE, 3);
    EXPECT_EQ(media::video::message::VideoFocusReason::USER_SELECTION, 4);
    EXPECT_EQ(sensorsource::message::SensorType::SENSOR_VEHICLE_ENERGY_MODEL_DATA, 23);
    EXPECT_EQ(sensorsource::message::SensorType::SENSOR_TRAILER_DATA, 24);
    EXPECT_EQ(navigationstatus::NavigationStatusMessageId::INSTRUMENT_CLUSTER_VEHICLE_ENERGY_FORECAST, 0x8008);
    EXPECT_EQ(media::sink::MediaMessageId::MEDIA_MESSAGE_ACTION_TAKEN_NOTIFICATION, 0x800C);
    EXPECT_EQ(media::sink::MediaMessageId::MEDIA_MESSAGE_UPDATE_HU_UI_CONFIG_REQUEST, 0x8011);
    EXPECT_EQ(media::sink::MediaMessageId::MEDIA_MESSAGE_MEDIA_OPTIONS, 0x8014);
    EXPECT_EQ(media::sink::MediaMessageId::MEDIA_MESSAGE_CRITICAL_UI_NOTIFICATION, 0x8015);
}

TEST(ProtoAdditionsUnitTest, HeadUnitInfo_VehicleType)
{
    control::message::HeadUnitInfo info;
    info.set_vehicle_type(2);
    EXPECT_EQ(info.SerializeAsString(), bytes({0x48, 0x02}));
}

TEST(ProtoAdditionsUnitTest, ServiceDiscoveryResponse_HideProjectedClock)
{
    control::message::ServiceDiscoveryResponse response;
    response.set_hide_projected_clock(true);
    EXPECT_EQ(response.SerializeAsString(), bytes({0x60, 0x01}));
}

TEST(ProtoAdditionsUnitTest, WirelessTcpConfiguration_BufferSizes)
{
    control::message::WirelessTcpConfiguration configuration;
    ASSERT_TRUE(configuration.ParseFromString(bytes({0x20, 0x80, 0x80, 0x04, 0x28, 0x80, 0x40})));
    EXPECT_EQ(configuration.socket_receive_buffer_size(), 65536u);
    EXPECT_EQ(configuration.socket_send_buffer_size(), 8192u);
}


TEST(ProtoAdditionsUnitTest, Radio_V2Ids)
{
    // right after v1 (ends at 0x8019)
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_STATE_NOTIFICATION, 0x8019);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_PROGRAM_LIST_NOTIFICATION, 0x801A);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_PROGRAM_INFO_NOTIFICATION, 0x801B);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_MUTE_REQUEST, 0x801C);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_MUTE_RESPONSE, 0x801D);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_TUNE_REQUEST, 0x801E);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_TUNE_RESPONSE, 0x801F);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_FAVORITE_LIST_NOTIFICATION, 0x8020);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_FAVORITE_TOGGLE_REQUEST, 0x8021);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_TUNE_DIRECTION_REQUEST, 0x8022);
    EXPECT_EQ(radio::RadioMessageId::RADIO_MESSAGE_SEARCH_REQUEST, 0x8023);
}

TEST(ProtoAdditionsUnitTest, Radio_ServiceBandsInField2)
{
    // v2 band groups next to the v1 tuner list: field 2, field 1 unchanged
    radio::RadioService service;
    auto* group = service.mutable_bands()->add_groups();
    group->add_band_types(radio::message::RADIO_BAND_TYPE_FM);
    group->set_hide_empty_group(true);
    EXPECT_EQ(service.SerializeAsString(), bytes({0x12, 0x06, 0x0A, 0x04, 0x08, 0x02, 0x10, 0x01}));

    radio::RadioService parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes({0x12, 0x06, 0x0A, 0x04, 0x08, 0x02, 0x10, 0x01})));
    EXPECT_EQ(parsed.radio_properties_size(), 0);
    ASSERT_EQ(parsed.bands().groups_size(), 1);
    EXPECT_EQ(parsed.bands().groups(0).band_types(0), radio::message::RADIO_BAND_TYPE_FM);
}

}
}
}
