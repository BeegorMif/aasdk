<div align="center">

# aasdk · OpenHU fork

**C++ implementation of the Android Auto™ head unit protocol**<br>
USB · TCP (wireless) · TLS · audio, video, input, sensor and status channels

[![License: GPL v3](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Platforms](https://img.shields.io/badge/platform-x86__64%20%7C%20arm64%20%7C%20armhf-lightgrey)
[![Upstream](https://img.shields.io/badge/upstream-opencardev%2Faasdk-6e7781)](https://github.com/opencardev/aasdk)
[![Used by](https://img.shields.io/badge/used%20by-OpenHU-2ea44f)](https://github.com/average-web-dev/openhu)

</div>

---

> [!CAUTION]
> ## 🤖 AI-generated changes: read this first
>
> **The changes in this fork were written largely by an AI model (Anthropic Claude) and only
> reviewed and tested by a single hobbyist on a single phone.** They have **not** been through
> professional code review, security audit or any automotive qualification.
>
> - This is an **experimental hobby project**. It is **not** safety-certified and must **not** be relied on for
>   anything that affects driving. Do not interact with a head unit while driving.
> - The code may contain bugs, security issues or protocol behaviour that differs from real head units, even
>   where it looks correct.
> - There is **no warranty** of any kind (see the GPLv3). You use this software **entirely at your own risk**.
> - Not affiliated with, endorsed or supported by Google, the Android Auto team, opencardev or f1x.studio.
>
> If you need something dependable, use [upstream aasdk](https://github.com/opencardev/aasdk) and review every
> change yourself.

---

## What is this?

aasdk is the protocol library behind [OpenAuto](https://github.com/opencardev/openauto) and
**[OpenHU](https://github.com/average-web-dev/openhu)**. It handles everything between the phone and your head unit
software: transport (USB/AOAP, TCP), the TLS handshake, message framing, and one service class per Android Auto
channel. Your application implements the event handlers and decides what to do with audio, video and input.

This fork tracks **[opencardev/aasdk](https://github.com/opencardev/aasdk)** (branch `newdev`) and adds what OpenHU
needs. The branch is **`openhu`**.

### Changes compared to upstream

| Area | Change |
|---|---|
| 🧭 Navigation status | Parses `INSTRUMENT_CLUSTER_NAVIGATION_STATE` and `…_CURRENT_POSITION`, which current Android Auto versions send instead of the deprecated turn/distance events. New handlers `onNavigationState()` and `onCurrentPosition()`; cluster start/stop is no longer logged as an error. |
| 📞 Phone status | `PHONE_STATUS` was dropped as "Message Id not Handled". It is now parsed and passed to `onPhoneStatus()` (calls with state, duration, caller name/number, signal strength). |

Everything else is upstream code. Upstream changes are merged in from time to time.

## Quick start

<details open>
<summary><b>Debian / Ubuntu / Raspberry Pi OS</b></summary>

```bash
sudo apt install -y build-essential cmake ninja-build git \
    libboost-all-dev libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler

git clone -b openhu https://github.com/average-web-dev/aasdk.git
cd aasdk
./build.sh release          # or: ./build.sh debug, ./build.sh release clean
```

</details>

<details>
<summary><b>Dev container (VS Code)</b></summary>

Open the folder in VS Code and run **Dev Containers: Reopen in Container**, then `./build.sh debug`.
Details and the arm64/armhf containers: [DEVCONTAINER.md](DEVCONTAINER.md).

</details>

<details>
<summary><b>Packages (.deb)</b></summary>

```bash
BUILD_PACKAGE=ON ./build.sh release
sudo apt install ./build-release/packages/*.deb
```

Package layout and versioning: [PACKAGING.md](PACKAGING.md).

</details>

<details>
<summary><b>Building OpenHU against this fork</b></summary>

OpenHU's CI clones this repository on branch `openhu`. For local development, `docker/dev.sh build --aasdk`
builds the checkout next to the OpenHU repository (`../aasdk-ref`, or `AASDK_DIR=/path/to/aasdk`), so aasdk
changes can be tested before pushing.

</details>

## Architecture at a glance

```
 Phone ──USB (AOAP) / TCP──► Transport ──► Messenger (TLS, framing) ──► Channel services ──► your event handlers
                                                                         ├─ Control            ├─ Video / audio sinks
                                                                         ├─ Media sink/source  ├─ Input source
                                                                         ├─ Sensor, Bluetooth  └─ Navigation, media,
                                                                         └─ Status channels       phone status
```

| Path | Contents |
|---|---|
| `include/aasdk/USB`, `TCP`, `Transport` | Device discovery, AOAP handshake, transports |
| `include/aasdk/Messenger` | TLS (`Cryptor`), message framing, send/receive queues |
| `include/aasdk/Channel/*` | One service per channel, each with an `I…ServiceEventHandler` interface |
| `protobuf/` | Android Auto protocol definitions (`aap_protobuf`) |

## Documentation

| | |
|---|---|
| 📖 [DOCUMENTATION.md](DOCUMENTATION.md) | Overview of everything |
| 🔨 [BUILD.md](BUILD.md) | Build options, cross-compilation |
| 🧪 [TESTING.md](TESTING.md) | Unit tests and validation |
| 🔧 [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | USB permissions, build errors, debugging |
| 📦 [PACKAGING.md](PACKAGING.md) | Debian packages and versions |
| 📝 [MODERN_LOGGER.md](MODERN_LOGGER.md) | Logging categories |

## Issues and contributing

- **Problems in this fork's changes** (navigation/phone status) → [average-web-dev/aasdk issues](https://github.com/average-web-dev/aasdk/issues)
- **Everything else** is most likely upstream → [opencardev/aasdk](https://github.com/opencardev/aasdk/issues)

Pull requests are welcome. If a change is useful beyond OpenHU, it is better sent upstream.

## License

**GNU GPLv3.** Copyright © 2018 f1x.studio (Michal Szwaj), extended by the OpenCarDev community and the OpenHU
contributors.

Built on [Boost](https://www.boost.org/), [libusb](https://libusb.info/), [OpenSSL](https://www.openssl.org/),
[Protocol Buffers](https://protobuf.dev/) and [GoogleTest](https://github.com/google/googletest).

<sub>Android Auto is a trademark of Google LLC. This project is not affiliated with or endorsed by Google.</sub>
