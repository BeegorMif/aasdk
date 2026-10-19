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

// Car control protos (#40) against bytes that worked with a phone (Fairphone 4, Android Auto 17.9, openhu#121):
// the hand-encoded descriptor of the OpenHU spike and messages the phone sent.

#include <gtest/gtest.h>
#include <aap_protobuf/service/Service.pb.h>
#include <aap_protobuf/service/carcontrol/CarControlService.pb.h>
#include <aap_protobuf/service/carcontrol/message/VehiclePropertyId.pb.h>
#include <aap_protobuf/service/carcontrol/message/SetCarPropertyValueRequest.pb.h>
#include <aap_protobuf/service/carcontrol/message/RegisterCarPropertyListenersRequest.pb.h>
#include <aap_protobuf/service/carcontrol/message/CarPropertyChangeEvent.pb.h>
#include <aap_protobuf/shared/MessageStatus.pb.h>

namespace aasdk::channel::carcontrol::ut {

  namespace msg = aap_protobuf::service::carcontrol::message;

  std::string fromHex(const std::string &hex) {
    std::string out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
      out.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
  }

  // the descriptor of the spike (commit 8b5f983) that showed the rail with temperatures
  const std::string cSpikeDescriptor =
      "0a4c08838a80ab0110031803220312010122031201042803320aa001980205e204b4060a3a130a031201011205150000e0411a0515000080"
      "413a130a031201041205150000e0411a0515000080410a34088b8a80aa01100118032203120101220312010428033a0d0a031201011202"
      "08031a0208003a0d0a03120104120208031a0208000a1e088d8a808a01100118032202080028003a0c0a020800120208031a0208000a22"
      "08858a80a9011002180322041202010428033a0e0a0412020104120218011a0218000a1e088e8a808a01100118032202080028003a0c0a"
      "020800120208311a0208300a1e088988808a01100118032202080028003a0c0a020800120208071a0208000a3b08908a80a90110021803"
      "2203120101220312010428033205838a80ab013a0d0a03120101120218011a0218003a0d0a03120104120218011a0218001282011a7e08"
      "0112130a0d0a0b08838a80ab0112031201012001300212130a0d0a0b08838a80ab0112031201042001300312130a0d0a0b088b8a80aa01"
      "12031201012001300212130a0d0a0b088b8a80aa0112031201042001300312120a0c0a0a088d8a808a01120208002001300212120a0e0a"
      "0c08858a80a901120412020104200120011a040a020801";

  constexpr int cDriver = msg::SEAT_ROW_1_LEFT;
  constexpr int cPassenger = msg::SEAT_ROW_1_RIGHT;

  // area of the given seat bits, or the global area for 0
  void setArea(msg::CarAreaId *area, const std::vector<int> &seats) {
    if (seats.size() == 0) {
      area->set_global(msg::GLOBAL_AREA);
    }
    for (int seat: seats) {
      area->add_seats(static_cast<msg::VehicleAreaSeat>(seat));
    }
  }

  void setValue(msg::CarPropertyValue *value, msg::CarPropertyValueType type, double v) {
    if (type == msg::CAR_PROPERTY_VALUE_TYPE_FLOAT) {
      value->set_float_value(static_cast<float>(v));
    } else if (type == msg::CAR_PROPERTY_VALUE_TYPE_BOOL) {
      value->set_bool_value(v != 0);
    } else {
      value->set_int32_value(static_cast<int>(v));
    }
  }

  // one config per area list entry; an empty list entry is the global area
  void addConfig(aap_protobuf::service::carcontrol::CarControlService &service, msg::VehiclePropertyId id,
                 msg::CarPropertyValueType type, std::vector<std::vector<int>> areas, double min, double max,
                 std::initializer_list<int> configArray = {}) {
    auto *config = service.add_property_configs();
    config->set_property_id(id);
    config->set_value_type(type);
    config->set_access(msg::CAR_PROPERTY_ACCESS_READ_WRITE);
    for (const auto &seats: areas) {
      setArea(config->add_area_ids(), seats);
    }
    config->set_area_type(areas.front().empty() ? msg::CAR_AREA_TYPE_GLOBAL : msg::CAR_AREA_TYPE_SEAT);
    for (int v: configArray) {
      config->add_config_array(v);
    }
    for (int i = 0; i < config->area_ids_size(); ++i) {
      auto *areaConfig = config->add_area_configs();
      *areaConfig->mutable_area() = config->area_ids(i);
      setValue(areaConfig->mutable_max_value(), type, max);
      setValue(areaConfig->mutable_min_value(), type, min);
    }
  }

  void addControl(msg::CarControlGroup *group, msg::VehiclePropertyId id, const std::vector<int> &seats,
                  msg::CarControlSideAffinity side) {
    auto *control = group->add_controls();
    auto *property = control->mutable_property_control()->mutable_property();
    property->set_property_id(id);
    setArea(property->mutable_area(), seats);
    control->set_enabled(true);
    if (side != msg::CAR_CONTROL_SIDE_AFFINITY_UNKNOWN) {
      control->set_side_affinity(side);
    }
  }

  TEST(CarControlProtocolUnitTest, DescriptorMatchesTheSpike) {
    aap_protobuf::service::carcontrol::CarControlService service;
    const auto seat = std::vector<std::vector<int>>{{cDriver}, {cPassenger}};
    const auto global = std::vector<std::vector<int>>{{}};
    addConfig(service, msg::HVAC_TEMPERATURE_SET, msg::CAR_PROPERTY_VALUE_TYPE_FLOAT, seat, 16.0, 28.0,
              {160, 280, 5, 610, 820, 10});
    addConfig(service, msg::HVAC_SEAT_TEMPERATURE, msg::CAR_PROPERTY_VALUE_TYPE_INT32, seat, 0, 3);
    addConfig(service, msg::HVAC_STEERING_WHEEL_HEAT, msg::CAR_PROPERTY_VALUE_TYPE_INT32, global, 0, 3);
    addConfig(service, msg::HVAC_AC_ON, msg::CAR_PROPERTY_VALUE_TYPE_BOOL, {{cDriver, cPassenger}}, 0, 1);
    addConfig(service, msg::HVAC_TEMPERATURE_DISPLAY_UNITS, msg::CAR_PROPERTY_VALUE_TYPE_INT32, global, 48, 49);
    addConfig(service, msg::IGNITION_STATE, msg::CAR_PROPERTY_VALUE_TYPE_INT32, global, 0, 7);
    addConfig(service, msg::HVAC_POWER_ON, msg::CAR_PROPERTY_VALUE_TYPE_BOOL, seat, 0, 1, {msg::HVAC_TEMPERATURE_SET});

    auto *top = service.add_controls();
    auto *group = top->mutable_control_group();
    group->set_group_type(msg::CAR_CONTROL_GROUP_TYPE_PRIMARY);
    addControl(group, msg::HVAC_TEMPERATURE_SET, {cDriver}, msg::CAR_CONTROL_SIDE_AFFINITY_DRIVER);
    addControl(group, msg::HVAC_TEMPERATURE_SET, {cPassenger}, msg::CAR_CONTROL_SIDE_AFFINITY_PASSENGER);
    addControl(group, msg::HVAC_SEAT_TEMPERATURE, {cDriver}, msg::CAR_CONTROL_SIDE_AFFINITY_DRIVER);
    addControl(group, msg::HVAC_SEAT_TEMPERATURE, {cPassenger}, msg::CAR_CONTROL_SIDE_AFFINITY_PASSENGER);
    addControl(group, msg::HVAC_STEERING_WHEEL_HEAT, {}, msg::CAR_CONTROL_SIDE_AFFINITY_DRIVER);
    addControl(group, msg::HVAC_AC_ON, {cDriver, cPassenger}, msg::CAR_CONTROL_SIDE_AFFINITY_UNKNOWN);
    top->set_enabled(true);
    service.add_supported_actions()->mutable_action()->set_action_id(msg::CAR_ACTION_LAUNCH_HVAC);

    EXPECT_EQ(service.SerializeAsString(), fromHex(cSpikeDescriptor));

    // in the service discovery: Service field 15
    aap_protobuf::service::Service channel;
    channel.set_id(19);
    ASSERT_TRUE(channel.mutable_car_control_service()->ParseFromString(fromHex(cSpikeDescriptor)));
    aap_protobuf::service::Service reparsed;
    ASSERT_TRUE(reparsed.ParseFromString(channel.SerializeAsString()));
    ASSERT_TRUE(reparsed.has_car_control_service());
    const auto &parsed = reparsed.car_control_service();
    ASSERT_EQ(parsed.property_configs_size(), 7);
    EXPECT_EQ(parsed.property_configs(0).area_configs(0).max_value().float_value(), 28.0f);
    EXPECT_EQ(parsed.property_configs(0).area_configs(0).min_value().float_value(), 16.0f);
    EXPECT_EQ(parsed.property_configs(2).area_ids(0).has_global(), true);
    EXPECT_EQ(parsed.controls(0).control_group().controls(1).side_affinity(), msg::CAR_CONTROL_SIDE_AFFINITY_PASSENGER);
  }

  TEST(CarControlProtocolUnitTest, PhoneRequests) {
    // the user pressed "-" on the passenger temperature: 21.0
    msg::SetCarPropertyValueRequest set;
    ASSERT_TRUE(set.ParseFromString(fromHex("0a0b08838a80ab0112031201041205150000a8411a24") +
                                    "e8c20e00-d3d6-4bd1-94a0-7c16972207ae"));
    EXPECT_EQ(set.property().property_id(), msg::HVAC_TEMPERATURE_SET);
    ASSERT_EQ(set.property().area().seats_size(), 1);
    EXPECT_EQ(set.property().area().seats(0), msg::SEAT_ROW_1_RIGHT);
    EXPECT_EQ(set.value().value_case(), msg::CarPropertyValue::kFloatValue);
    EXPECT_EQ(set.value().float_value(), 21.0f);
    EXPECT_EQ(set.request_id(), "e8c20e00-d3d6-4bd1-94a0-7c16972207ae");

    // registration when the channel opened: temperature, seat heat, steering wheel, A/C
    msg::RegisterCarPropertyListenersRequest registration;
    ASSERT_TRUE(registration.ParseFromString(fromHex(
        "0a0b08838a80ab0112031201010a0b08838a80ab0112031201040a0b088b8a80aa0112031201010a0b088b8a80aa0112031201040a0a"
        "088d8a808a01120208000a0c08858a80a901120412020104")));
    ASSERT_EQ(registration.properties_size(), 6);
    EXPECT_EQ(registration.properties(4).property_id(), msg::HVAC_STEERING_WHEEL_HEAT);
    EXPECT_TRUE(registration.properties(4).area().has_global());
    EXPECT_EQ(registration.properties(5).area().seats_size(), 2);
  }

  TEST(CarControlProtocolUnitTest, ChangeEventAsSentByTheSpike) {
    msg::CarPropertyChangeEvent event;
    event.mutable_property()->set_property_id(msg::HVAC_TEMPERATURE_SET);
    event.mutable_property()->mutable_area()->add_seats(msg::SEAT_ROW_1_RIGHT);
    event.mutable_value()->set_float_value(21.0f);
    event.set_status(msg::CAR_PROPERTY_STATUS_AVAILABLE);
    EXPECT_EQ(event.SerializeAsString(), fromHex("0a0b08838a80ab0112031201041205150000a8411801"));
  }

  TEST(CarControlProtocolUnitTest, StatusValues) {
    EXPECT_EQ(aap_protobuf::shared::MessageStatus::STATUS_CAR_PROPERTY_SET_REQUEST_FAILED, -26);
    EXPECT_EQ(aap_protobuf::shared::MessageStatus::STATUS_CAR_PROPERTY_LISTENER_REGISTRATION_FAILED, -27);
  }

}
