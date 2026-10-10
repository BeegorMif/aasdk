// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
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

#include <libusb.h>

namespace aasdk::usb {

  // True for devices that cannot be an Android phone: every interface is a hub, HID (keyboard, mouse), audio,
  // video, printer or smart card. They get no Android accessory requests: each one waits for a timeout on
  // devices that do not answer (seconds at startup with many devices) and the request is vendor specific.
  // Network interfaces (CDC, RNDIS) are kept, a phone in USB tethering looks like that.
  bool isCertainlyNotPhone(libusb_device *device);

}
