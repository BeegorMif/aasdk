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

#include <gtest/gtest.h>
#include <aap_protobuf/service/Service.pb.h>
#include <aap_protobuf/service/carlocalmedia/CarLocalMediaMessageId.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackStatus.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackMetadata.pb.h>
#include <aap_protobuf/service/carlocalmedia/message/CarLocalMediaPlaybackRequest.pb.h>
#include <aasdk/Messenger/ChannelId.hpp>

namespace aasdk::channel::carlocalmedia::ut {

  namespace msg = aap_protobuf::service::carlocalmedia::message;

  std::string fromHex(const std::string &hex) {
    std::string out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
      out.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
  }

  std::string toHex(const std::string &bytes) {
    static const char *digits = "0123456789abcdef";
    std::string out;
    for (unsigned char c: bytes) {
      out.push_back(digits[c >> 4]);
      out.push_back(digits[c & 15]);
    }
    return out;
  }

  TEST(CarLocalMediaProtocol, MessageIds) {
    EXPECT_EQ(aap_protobuf::service::carlocalmedia::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_STATUS, 0x8001);
    EXPECT_EQ(aap_protobuf::service::carlocalmedia::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_METADATA, 0x8002);
    EXPECT_EQ(aap_protobuf::service::carlocalmedia::CAR_LOCAL_MEDIA_MESSAGE_PLAYBACK_REQUEST, 0x8003);
    EXPECT_EQ(messenger::channelIdToString(messenger::ChannelId::CAR_LOCAL_MEDIA), "CAR_LOCAL_MEDIA");
  }

  // the service descriptor is field 16 of the channel, an empty message
  TEST(CarLocalMediaProtocol, Descriptor) {
    aap_protobuf::service::Service service;
    service.set_id(20);
    service.mutable_car_local_media_service();
    EXPECT_EQ(toHex(service.SerializeAsString()), "081482" "0100");
  }

  // the status the phone showed as PLAYING with four buttons (openhu#147 spike): actions not packed
  TEST(CarLocalMediaProtocol, StatusAsInTheSpike) {
    msg::CarLocalMediaPlaybackStatus status;
    status.set_playback_state(msg::CAR_LOCAL_MEDIA_PLAYBACK_STATE_PLAYING);
    status.set_media_source("OpenHU Music");
    status.set_playback_position(15);
    status.add_actions(msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_PLAY);
    status.add_actions(msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_PAUSE);
    status.add_actions(msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_PREVIOUS);
    status.add_actions(msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_NEXT);
    EXPECT_EQ(toHex(status.SerializeAsString()),
              "0802" "120c" + toHex("OpenHU Music") + "180f" "2000" "2001" "2002" "2003");
  }

  TEST(CarLocalMediaProtocol, Metadata) {
    msg::CarLocalMediaPlaybackMetadata metadata;
    metadata.set_song("Spike Song 1");
    metadata.set_artist("OpenHU");
    metadata.set_album("Car Local Media");
    metadata.set_album_art(std::string("\x89PNG", 4));
    metadata.set_duration_seconds(180);
    EXPECT_EQ(toHex(metadata.SerializeAsString()),
              "0a0c" + toHex("Spike Song 1") + "1206" + toHex("OpenHU") + "1a0f" + toHex("Car Local Media")
              + "2204" "89504e47" "28b401");
  }

  // requests the phone sent from its media card: PLAY, NEXT, PREVIOUS
  TEST(CarLocalMediaProtocol, RequestsFromThePhone) {
    msg::CarLocalMediaPlaybackRequest request;
    ASSERT_TRUE(request.ParseFromString(fromHex("0800")));
    EXPECT_EQ(request.action(), msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_PLAY);
    ASSERT_TRUE(request.ParseFromString(fromHex("0803")));
    EXPECT_EQ(request.action(), msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_NEXT);
    ASSERT_TRUE(request.ParseFromString(fromHex("0802")));
    EXPECT_EQ(request.action(), msg::CAR_LOCAL_MEDIA_PLAYBACK_ACTION_PREVIOUS);
  }

}
