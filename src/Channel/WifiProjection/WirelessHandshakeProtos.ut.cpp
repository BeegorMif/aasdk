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

// Messages of the wireless handshake (aaw) against hand-written wire bytes (average-web-dev/aasdk#8)

#include <gtest/gtest.h>
#include <aap_protobuf/aaw/MessageId.pb.h>
#include <aap_protobuf/aaw/Status.pb.h>
#include <aap_protobuf/aaw/WifiConnectionRejection.pb.h>
#include <aap_protobuf/aaw/WifiConnectionStatus.pb.h>
#include <aap_protobuf/aaw/WifiInfoResponse.pb.h>
#include <aap_protobuf/aaw/WifiPingRequest.pb.h>
#include <aap_protobuf/aaw/WifiStartRequest.pb.h>
#include <aap_protobuf/aaw/WifiStartResponse.pb.h>
#include <aap_protobuf/aaw/WifiVersionRequest.pb.h>
#include <aap_protobuf/aaw/WifiVersionResponse.pb.h>


namespace aasdk
{
namespace channel
{
namespace wifiprojection
{
namespace ut
{

namespace aaw = aap_protobuf::aaw;

std::string bytes(std::initializer_list<uint8_t> list)
{
    return std::string(list.begin(), list.end());
}

TEST(WirelessHandshakeProtosUnitTest, MessageIds)
{
    EXPECT_EQ(aaw::MessageId::WIFI_START_REQUEST, 1);
    EXPECT_EQ(aaw::MessageId::WIFI_START_RESPONSE, 7);
    EXPECT_EQ(aaw::MessageId::WIFI_PING_REQUEST, 8);
    EXPECT_EQ(aaw::MessageId::WIFI_PING_RESPONSE, 9);
    EXPECT_EQ(aaw::MessageId::WIFI_CONNECTION_REJECTION, 10);
    EXPECT_EQ(aaw::MessageId::WIFI_SETUP_INFO, 11);
}

TEST(WirelessHandshakeProtosUnitTest, ConnectionStatus_UnknownStatusStaysReadable)
{
    // status -12 is not in Status; with a required enum the message did not parse at all
    aaw::WifiConnectionStatus status;
    ASSERT_TRUE(status.ParseFromString(bytes({0x08, 0xF4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                                              0x12, 0x02, 'n', 'o'})));
    EXPECT_EQ(status.status(), -12);
    EXPECT_FALSE(aaw::Status_IsValid(status.status()));
    EXPECT_EQ(status.error_message(), "no");
}

TEST(WirelessHandshakeProtosUnitTest, ConnectionStatus_Success)
{
    aaw::WifiConnectionStatus status;
    ASSERT_TRUE(status.ParseFromString(bytes({0x08, 0x00})));
    EXPECT_EQ(status.status(), aaw::Status::STATUS_SUCCESS);
}

TEST(WirelessHandshakeProtosUnitTest, StartResponse_AllFields)
{
    aaw::WifiStartResponse response;
    ASSERT_TRUE(response.ParseFromString(bytes({0x0A, 0x01, 'h', 0x10, 0xC8, 0x29, 0x18, 0x00})));
    EXPECT_EQ(response.ip_address(), "h");
    EXPECT_EQ(response.port(), 5320u);
    EXPECT_EQ(response.status(), aaw::Status::STATUS_SUCCESS);
}

TEST(WirelessHandshakeProtosUnitTest, VersionResponse_AllFields)
{
    aaw::WifiVersionResponse response;
    ASSERT_TRUE(response.ParseFromString(bytes({
        0x08, 0x01,                                    // major 1
        0x10, 0x07,                                    // minor 7
        0x1A, 0x02, 's', 'n',                          // device_serial
        0x20, 0x00,                                    // status
        0x28, 0x02,                                    // selected_wifi_channel_type
        0x32, 0x08, 0x0A, 0x02, 'i', 'd', 0x22, 0x02, 'b', 't', // device_info: device_id, bluetooth_address
        0x3A, 0x02, 0x01, 0x02                         // setup_token
    })));
    EXPECT_EQ(response.major(), 1);
    EXPECT_EQ(response.minor(), 7);
    EXPECT_EQ(response.device_serial(), "sn");
    EXPECT_EQ(response.status(), aaw::Status::STATUS_SUCCESS);
    EXPECT_EQ(response.selected_wifi_channel_type(), aaw::WifiChannelType::CHANNELS_DUAL_BAND);
    EXPECT_EQ(response.device_info().device_id(), "id");
    EXPECT_EQ(response.device_info().bluetooth_address(), "bt");
    EXPECT_EQ(response.setup_token(), bytes({0x01, 0x02}));
}

TEST(WirelessHandshakeProtosUnitTest, VersionResponse_EmptyIsValid)
{
    // the fields were required before; a phone that leaves them out must not break the handshake
    aaw::WifiVersionResponse response;
    EXPECT_TRUE(response.ParseFromString(std::string()));
}

TEST(WirelessHandshakeProtosUnitTest, VersionRequest_Serialised)
{
    aaw::WifiVersionRequest request;
    request.set_major(1);
    request.set_minor(7);
    request.set_supported_wifi_channel_type(aaw::WifiChannelType::CHANNELS_5GHZ_ONLY);
    request.add_supported_wifi_channels(36);
    request.mutable_car_info()->set_head_unit_make("O");
    request.mutable_wpp_info()->set_ip_address("i");
    request.mutable_wpp_info()->set_port(5288);

    EXPECT_EQ(request.SerializeAsString(), bytes({
        0x08, 0x01, 0x10, 0x07, 0x18, 0x00, 0x20, 0x24,
        0x2A, 0x03, 0x2A, 0x01, 'O',                     // car_info.head_unit_make (5)
        0x32, 0x06, 0x0A, 0x01, 'i', 0x10, 0xA8, 0x29    // wpp_info: ip_address, port
    }));
}

TEST(WirelessHandshakeProtosUnitTest, StartRequest_Reason)
{
    aaw::WifiStartRequest request;
    request.set_ip_address("i");
    request.set_port(5288);
    request.set_start_request_reason(aaw::StartRequestReason::START_REQUEST_REASON_AUTO_LAUNCH);
    EXPECT_EQ(request.SerializeAsString(), bytes({0x0A, 0x01, 'i', 0x10, 0xA8, 0x29, 0x18, 0x01}));
}

TEST(WirelessHandshakeProtosUnitTest, InfoResponse_WithoutBssid)
{
    aaw::WifiInfoResponse response;
    response.set_ssid("s");
    response.set_password("p");
    EXPECT_TRUE(response.IsInitialized());
    EXPECT_EQ(response.SerializeAsString(), bytes({0x0A, 0x01, 's', 0x12, 0x01, 'p'}));
}

TEST(WirelessHandshakeProtosUnitTest, PingAndRejection)
{
    aaw::WifiPingRequest ping;
    ASSERT_TRUE(ping.ParseFromString(bytes({0x08, 0x2A})));
    EXPECT_EQ(ping.timestamp(), 42);

    aaw::WifiConnectionRejection rejection;
    ASSERT_TRUE(rejection.ParseFromString(bytes({0x08, 0x02})));
    EXPECT_EQ(rejection.reason(), aaw::ConnectionRejectionReason::CONNECTION_REJECTION_REASON_INVALID_SETUP_TOKEN);
}

}
}
}
}
