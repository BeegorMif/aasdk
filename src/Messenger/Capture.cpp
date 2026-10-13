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

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <aasdk/Messenger/Capture.hpp>
#include <aasdk/Common/Log.hpp>

namespace aasdk::messenger::capture {

  namespace {

    struct Capture {
      std::mutex mutex;
      std::ofstream file;
      bool media = false;
      std::chrono::steady_clock::time_point start;

      Capture() {
        const char *path = std::getenv("AASDK_CAPTURE");
        const char *media = std::getenv("AASDK_CAPTURE_MEDIA");
        if (path != nullptr && *path != '\0') {
          this->open(path, media != nullptr && std::string(media) == "1");
        }
      }

      bool open(const std::string &path, bool withMedia) {
        file.close();
        file.clear();
        file.open(path, std::ios::app);
        media = withMedia;
        start = std::chrono::steady_clock::now();
        if (!file) {
          AASDK_LOG(error) << "[Capture] cannot open " << path;
          return false;
        }
        AASDK_LOG(info) << "[Capture] writing protocol capture to " << path;
        return true;
      }
    };

    Capture &instance() {
      static Capture capture;
      return capture;
    }

  }

  bool start(const std::string &path, bool media) {
    auto &capture = instance();
    std::lock_guard<std::mutex> lock(capture.mutex);
    return capture.open(path, media);
  }

  void stop() {
    auto &capture = instance();
    std::lock_guard<std::mutex> lock(capture.mutex);
    capture.file.close();
  }

  bool enabled() {
    auto &capture = instance();
    std::lock_guard<std::mutex> lock(capture.mutex);
    return capture.file.is_open();
  }

  void record(const char *direction, const Message &message) {
    auto &capture = instance();
    std::lock_guard<std::mutex> lock(capture.mutex);
    if (!capture.file.is_open()) {
      return;
    }

    const auto &payload = message.getPayload();
    const int id = payload.size() >= 2 ? (payload[0] << 8 | payload[1]) : -1;
    const bool isData = id == 0 && message.getChannelId() != ChannelId::CONTROL;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - capture.start).count();

    std::string line = "{\"t\":" + std::to_string(ms) + ",\"dir\":\"" + direction + "\",\"ch\":" +
                       std::to_string(static_cast<int>(message.getChannelId())) + ",\"chName\":\"" +
                       channelIdToString(message.getChannelId()) + "\",\"enc\":" +
                       (message.getEncryptionType() == EncryptionType::ENCRYPTED ? "true" : "false") +
                       ",\"control\":" + (message.getType() == MessageType::CONTROL ? "true" : "false") +
                       ",\"id\":" + std::to_string(id) + ",\"len\":" + std::to_string(payload.size());
    if (!isData || capture.media) {
      static const char digits[] = "0123456789abcdef";
      line += ",\"hex\":\"";
      for (const auto byte: payload) {
        line += digits[byte >> 4];
        line += digits[byte & 0x0f];
      }
      line += "\"";
    }
    line += "}\n";
    capture.file << line;
    capture.file.flush();
  }

}
