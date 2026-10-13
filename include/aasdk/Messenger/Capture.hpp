// This file is part of aasdk library project.
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

#include <string>
#include <aasdk/Messenger/Message.hpp>

namespace aasdk::messenger::capture {

  // Protocol capture for analysis (average-web-dev/aasdk#14): with the environment variable
  // AASDK_CAPTURE=<file> every message is appended to <file> as one JSON line, incoming after
  // decryption and outgoing before encryption:
  //   {"t":<ms since the first message>,"dir":"in"|"out","ch":<channel number>,"chName":"...",
  //    "enc":<encrypted>,"control":<control message type>,"id":<message id>,"len":<payload bytes>,"hex":"..."}
  // Audio and video data (message id 0 outside the control channel) are written without "hex"
  // unless AASDK_CAPTURE_MEDIA=1. The file contains personal data from the phone (contacts,
  // notifications, location): keep it local.
  void record(const char *direction, const Message &message);

  // Starts (or restarts) a capture without the environment variable, e.g. from a setting.
  // Returns false if the file cannot be opened.
  bool start(const std::string &path, bool media = false);

  void stop();

  bool enabled();

}
