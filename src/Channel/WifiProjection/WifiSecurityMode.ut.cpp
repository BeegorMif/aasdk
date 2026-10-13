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

// Wire values of WifiSecurityMode (average-web-dev/aasdk#4)

#include <gtest/gtest.h>
#include <aap_protobuf/aaw/WifiInfoResponse.pb.h>
#include <aap_protobuf/service/wifiprojection/message/WifiSecurityMode.pb.h>


namespace aasdk
{
namespace channel
{
namespace wifiprojection
{
namespace ut
{

using aap_protobuf::service::wifiprojection::message::WifiSecurityMode;

TEST(WifiSecurityModeUnitTest, ValuesAsAndroidAutoExpectsThem)
{
    EXPECT_EQ(WifiSecurityMode::UNKNOWN_SECURITY_MODE, 0);
    EXPECT_EQ(WifiSecurityMode::OPEN, 1);
    EXPECT_EQ(WifiSecurityMode::WEP_64, 2);
    EXPECT_EQ(WifiSecurityMode::WEP_128, 3);
    EXPECT_EQ(WifiSecurityMode::WPA_PERSONAL, 4);
    EXPECT_EQ(WifiSecurityMode::WPA2_PERSONAL, 8);
    EXPECT_EQ(WifiSecurityMode::WPA_WPA2_PERSONAL, 12);
    EXPECT_EQ(WifiSecurityMode::WPA_ENTERPRISE, 20);
    EXPECT_EQ(WifiSecurityMode::WPA2_ENTERPRISE, 24);
    EXPECT_EQ(WifiSecurityMode::WPA_WPA2_ENTERPRISE, 28);
    EXPECT_EQ(WifiSecurityMode::WPA3_PERSONAL, 32);
    EXPECT_EQ(WifiSecurityMode::WPA2_WPA3_PERSONAL, 40);
}

TEST(WifiSecurityModeUnitTest, WifiInfoResponse_SerialisesWpa2PersonalAs8)
{
    aap_protobuf::aaw::WifiInfoResponse response;
    response.set_ssid("s");
    response.set_password("p");
    response.set_bssid("b");
    response.set_security_mode(WifiSecurityMode::WPA2_PERSONAL);

    // field 4, varint: tag 0x20, value 8
    const std::string expected("\x0a\x01s\x12\x01p\x1a\x01" "b\x20\x08", 11);
    EXPECT_EQ(response.SerializeAsString(), expected);
}

}
}
}
}
