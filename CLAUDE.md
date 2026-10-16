# CLAUDE.md – aasdk (OpenHU fork)

The Android Auto protocol library that OpenHU uses. It is a fork of opencardev/aasdk, and our default branch is `openhu`. Fixes that also help upstream go there as a separate PR, branched from upstream `newdev`. Ask the user before opening a PR upstream.

## This repo
- `protobuf/aap_protobuf/`: protos (proto2). New messages and fields follow open-android-auto. Write a comment with the source and the confidence, and say when something was verified with a phone.
- `include/aasdk/`, `src/`: transport (USB, TCP), messenger (framing, TLS, max. 16128 bytes per frame), channels and services. Unit tests sit next to the code (`*.ut.cpp`, gtest/gmock), and the binary is `bin/aasdk_ut`.
- Build in Docker (OpenHU's `openhu:dev` image): `BUILD_DIR=/build/aasdk BUILD_TESTS=ON bash ./build.sh release --skip-protobuf --skip-absl`. Make packages with `cpack -G DEB` in the build dir.
- Version in `CMakeLists.txt`: patch for fixes, minor for new API or ABI, major for breaking API (e.g. renamed generated accessors). Bump it in a separate "Version x.y.z" PR at release time. Tag `openhu-v<version>` (workflow `release.yml`). The `openhu.yml` workflow builds the packages (`aasdk-trixie-<arch>`) on every push to `openhu`, and OpenHU's CI uses them while there is no fitting release.

## The OpenHU project

Four repositories under github.com/average-web-dev, usually checked out next to each other:

| Repo | What | Default branch |
|---|---|---|
| `openhu` | the head unit shell (C++/Qt6/QML), Android Auto via aasdk, plugin host, EventBus API | `main` |
| `aasdk` | fork of opencardev/aasdk, the Android Auto protocol library OpenHU builds against | `openhu` |
| `openhu-os` | the OS image (Debian trixie, mkosi, cage kiosk, read-only root, A/B updates), VM scripts | `main` |
| `openhu-plugins` | plugins (radio, camera, CAN, steering wheel) and the plugin store index | `main` |

### How to work
- **Develop in Docker** (`openhu/docker/dev.sh`, image `openhu:dev`): build, unit tests, running OpenHU against the host's display, audio and USB. Only work that is about the OS itself (mkosi config, systemd units, update/boot scripts, packages in the image) is done in `openhu-os` with its own scripts and the VM.
- Several sessions may work in the same checkouts at once: do branch work in a **git worktree** (`<repo>.worktrees/<name>`), never switch branches in the shared checkout, and give parallel Docker builds their **own volume names** instead of the `dev.sh` volumes.
- Bigger changes: write the concept (in the issue) and wait for the user's approval first.
- Code, comments, commits, PRs and issues in **English**. UI strings in English with a **German translation** (English is the default language).
- Commits are signed (SSH key from the user's agent). If signing fails, stop and ask; never bypass it.
- Merge only with the user's explicit OK. Tags and releases are public: only when the user asks (see Releases).

### Testing
- Unit tests: QtTest/ctest (openhu, `QT_QPA_PLATFORM=offscreen`), gtest (aasdk, `BUILD_TESTS=ON`), Python unittest (plugins).
- The dev VM (`openhu-os/scripts/run-vm.sh`, ssh on port 2222 via `scripts/ssh.sh`, `scripts/screenshot.sh`, clicks via QMP `vm/qmp.sock`) belongs to the user, with their phone passed through. Change it (updates, reboots, plugin files) only when the user asks for a test there. For your own tests start a separate instance (`--instance <name> --ssh-port <port> --headless --no-gl --no-phone`).
- Test images: build OpenHU/aasdk debs in Docker, `scripts/fetch-debs.sh --dir <debs>`, `scripts/build.sh --profile dev --version <higher than installed>`, `scripts/add-update.sh <version> vm/upif-updates` (served on port 8070), then `openhu-cli update install --version <v> --reboot` in the VM.
- Phone logs: `adb` over WiFi (wireless debugging must be re-enabled after each Android Auto connect). Logcat buffers are small, so enlarge them first (`adb logcat -G 64M`).
- Before `docker/dev.sh run`, make sure wireless Android Auto is off in `openhu/docker/config`, otherwise OpenHU takes over the host's WiFi for its hotspot.

### Releases and versions
- All earlier dev releases and tags were deleted. The first real release is version **1** everywhere: aasdk `1.0.0`, OpenHU `1.0.0`, plugins `1.0.0`, OpenHU OS `1.0`. The OS follows distribution conventions: point releases `1.1`, `1.2`, and dev builds `<next>~dev.<date>.<time>`, which sort before the release.
- Release order: aasdk → OpenHU (its CI builds against the newest fitting aasdk release, or the aasdk CI artifacts while there is none) → OpenHU OS.
- No release per fix: the user releases when (almost) all open issues are done.

### Rules
- Plugins must not depend on internet services. Use only local files placed by the user, or the broadcast itself. Don't bundle third-party logos (trademarks). The simulated SDR stick (streams from the internet) is a dev tool only.

### Protocol facts found with a real phone (Fairphone 4, Android Auto 17.x)
- Messages from the head unit must be split into frames of at most **16128** bytes. With 16384 the encrypted frame overflows the phone's buffer and the phone drops the connection (aasdk#37, upstream opencardev/aasdk#98).
- The radio uses the v2 API (0x801A–0x8023): frequencies in kHz, band groups in field 2 of `RadioService`, and the band scan action is sent when a band tab opens. Field 3 of `RadioProgramInfoNotification` is the **favorite heart**, not audio focus. DAB `DAB_SID_EXT`: SId in bits 0–15, ECC in bits 16–23, plus `DAB_ENSEMBLE` and `DAB_FREQUENCY` (kHz).
- The Assistant requests `AUDIO_FOCUS_GAIN_TRANSIENT` and holds it until the release, also while it listens (guidance stopped, microphone open). Navigation prompts play on the guidance channel.
- On `VIDEO_FOCUS_NATIVE` the phone stops video. After `PROJECTED` it first sends about 170 ms of black frames and then fades its UI in.
- Reference for the protocol: github.com/mrmees/open-android-auto (docs/channels/*, phenotype flags).
