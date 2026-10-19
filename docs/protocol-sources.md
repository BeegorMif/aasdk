# Protocol sources

Where the definitions in `protobuf/aap_protobuf/` come from, apart from the original set, and whether a phone has confirmed them. It covers what was added or changed from outside in this fork (average-web-dev/aasdk#14, #11, #32, #40, #46).

Built from the git history, the issues aasdk#4, #8, #10, #11, #14, #32, #40, #45 and their pull requests, openhu#78 and #121, and the source comments in the protos. Where a source says nothing, the entry says **unknown**; nothing here is guessed.

Update this file together with every proto change that takes a definition from outside. `tools/proto_compare.py` makes the comparison with the other projects repeatable.

## Licensing and method

aasdk is GPL-3.0-or-later (see `LICENSE`). Code and `.proto` files from GPL-2.0 (aa-proxy-rs) and AGPL-3.0 (Open Headunit) projects are not copied; only facts are taken (field numbers, names, types, message ids, flows) and the definitions are written for aasdk. open-android-auto and aasdk are both GPL-3.0, but its files are not copied verbatim either (other packages, partly proto3, renamed fields). Every addition carries a comment in the proto with the source.

## Sources

| Short name | Project | State used | License | How it was used |
|---|---|---|---|---|
| upstream | [opencardev/aasdk](https://github.com/opencardev/aasdk) (`newdev`, originally f1xpl/aasdk) | base of this fork; `develop` `f9cf21e` (2026-03-18) compared in #14 | GPL-3.0 | The original protos. The 1.6 restructuring (`e8236a6`, sjdean, 2024-11-21) names [milek7.pl galdocs](https://milek7.pl/.stuff/galdocs/readme.md) as its source. Before that: unknown. |
| OHU | [Open Headunit](https://github.com/andreknieriem/open-headunit) | `4ee1d58` (2026-10-10) | AGPL-3.0 | Facts only (Kotlin code, `wireless.proto`) |
| AAPRS | [aa-proxy-rs](https://github.com/aa-proxy/aa-proxy-rs) | `b99fed2` (2026-10-12) | GPL-2.0 | Facts only (`src/protos/protos.proto`, `ev.proto`, `src/mitm.rs`) |
| WAAD | [WirelessAndroidAutoDongle](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle) | `ef64522` (2025-07-27) | MIT | Cross-check of the wireless protos only (#14, #4) |
| OAA | [open-android-auto](https://github.com/mrmees/open-android-auto) | `61eab61` (2026-07-27) | GPL-3.0 | Radio v2, as a reference for the car control analysis |
| AA app | Android Auto app analysis (17.9) | Android Auto 17.9 | – | Own analysis of the app, recorded in the proto comments and in openhu#121; the decompile itself is not published |

## Tiers and "verified"

- **Tier** is the confidence label of the source. Only OAA has one: `gold`, `silver`, `bronze`, `unverified` (and `retracted`, which was skipped). It is reverse engineered, and its tiers are the only evidence it gives. OHU, AAPRS and WAAD have no tiers: "none" below. For the AA app analysis the tier column says so.
- **Phone-verified** means a real phone exercised it. The only phone so far is the Fairphone 4 (Android Auto 17.x, via OpenHU). "no" means no test is recorded, not that the definition is wrong. Static analysis of the app (OAA, AA app) and reading the code of another project are not verification.

## Original definitions

Everything not listed below is the original set of upstream aasdk (restructured in `e8236a6`). That set has not been checked against a phone systematically; which parts a phone confirmed is **unknown**.

## Added or changed

### Wireless handshake (`aaw/`)

PR #21 (`40eb630`, issue #8), 2026-10-13, released in 1.3.0.

| Definition | Source | Tier | Phone-verified |
|---|---|---|---|
| `MessageId` 8 `WIFI_PING_REQUEST`, 9 `WIFI_PING_RESPONSE`, 10 `WIFI_CONNECTION_REJECTION`, 11 `WIFI_SETUP_INFO` | OHU `4ee1d58` (`wireless.proto`), AAPRS | none | no. `WIFI_SETUP_INFO` has no payload defined (only unknown fields in OHU) |
| `WifiVersionRequest` 1–6 (`major`, `minor`, `supported_wifi_channel_type`, `supported_wifi_channels`, `car_info`, `wpp_info` with the TCP endpoint, which Android Auto 17.4 dials directly on later connections, #9) | OHU, AAPRS | none | no |
| `WifiVersionResponse` 1–7 named (was `unknown_value_a..d`), all optional; new 5–7 (`selected_wifi_channel_type`, `device_info`, `setup_token`) | OHU, AAPRS | none | no |
| `WifiStartRequest` 3 `start_request_reason` | OHU, AAPRS | none | no |
| `WifiStartResponse.status`, `WifiConnectionStatus.status` as `optional int32` instead of `required Status` (an unknown status made the message unreadable) | OHU, AAPRS | none | no. The parse failure is reproduced in a unit test only |
| `WifiInfoResponse.bssid`, `security_mode` optional | OHU, AAPRS | none | no |
| New: `WifiPingRequest/Response`, `WifiConnectionRejection`, `WppCarInfo`, `WppWifiDeviceInfo`, `WifiProjectionProtocolInfo`, enums `WifiChannelType`, `StartRequestReason`, `ConnectionRejectionReason` | OHU, AAPRS | none | no |

Comment in the files: "Field numbers and names from Android Auto traffic as documented by Open Headunit (wireless.proto, andreknieriem/open-headunit@4ee1d58) and aa-proxy-rs".

PR #16 (`66ec817`, issue #4): `WifiSecurityMode` values changed to the bit combinations `WPA2_PERSONAL = 8`, `WPA_WPA2_PERSONAL = 12`, `WPA_ENTERPRISE = 20`, `WPA2_ENTERPRISE = 24`, `WPA_WPA2_ENTERPRISE = 28`, new `WPA3_PERSONAL = 32`, `WPA2_WPA3_PERSONAL = 40`. Source: OHU (`wireless.proto`), AAPRS and WAAD (`WifiInfoResponse.proto`) agree. Tier: none. Phone-verified: no recorded test of the enum itself; OpenHU's wireless handshake already sent the value 8 for WPA2 through a cast (#4), the other values: **unknown**.

### Fields and values from other projects (#11)

PR #22 (`07c2262`), 2026-10-13, released in 1.3.0. Only additions.

| Definition | Source | Tier | Phone-verified |
|---|---|---|---|
| `HeadUnitInfo` 9 `vehicle_type` | OHU (`VehicleTypePolicy.kt`) | none | no |
| `ServiceDiscoveryResponse` 12 `hide_projected_clock` | OHU | none | no. Also listed in #45 as "only in aasdk, unconfirmed" |
| `WirelessTcpConfiguration` 4, 5 `socket_receive_buffer_size`, `socket_send_buffer_size` | OHU, AAPRS | none | no (#45: unconfirmed) |
| `PointerAction` `ACTION_CANCEL = 3`, `ACTION_OUTSIDE = 4` | OHU, AAPRS | none | no |
| `VideoFocusReason` `LAST_MODE = 3`, `USER_SELECTION = 4` | OHU, AAPRS | none | no |
| `SensorType` `SENSOR_VEHICLE_ENERGY_MODEL_DATA = 23`, `SENSOR_TRAILER_DATA = 24` | AAPRS (`ev.proto`) | none | no. The payload is only partly known; `SensorBatch.energy_model_control` is not defined (#41) |
| `NavigationStatusMessageId` `INSTRUMENT_CLUSTER_VEHICLE_ENERGY_FORECAST = 32776` | AAPRS | none | The id is seen: the Fairphone 4 sends the forecast after selecting protocol 6.1 (#45). The payload is not defined |
| `MediaMessageId` 32780–32789 (action taken, 4 integrated overlay ids, HU UI config request/response, media stats, media options, critical UI notification) | AAPRS ("ids seen in traffic") | none | Only the ids are defined. Payloads: unknown (#43) |

The conflicts found in the same analysis are **not** changed: `PhoneStatus.State` (aasdk `CONFERENCED = 5`, `MUTED = 6`, `INACTIVE = 3`; OHU `Muted = 5`, `Conferenced = 6`, `HangingUp = 3`) and `AudioStreamType` ≥ 4 (aasdk `TELEPHONY = 4`; OHU `ALARM = 4`, `GUIDANCE = 5`, `ANNOUNCEMENT = 6`, `RING = 7`, `NONE = 0`). Both need a capture (#45).

### Radio v2 (Android Auto 16.2+)

PR #33 (`54a27b8`, task 5 of #32), 2026-10-15, released in 2.1.0. Source for all of it: OAA `61eab61`, `oaa/radio/RadioMessages.proto`, `oaa/control/RadioChannelData.proto`, `docs/channels/radio.md` (APK static analysis). Every definition names it in a comment.

| Definition | Tier | Phone-verified |
|---|---|---|
| `RadioMessageId` 0x801A–0x8023 | silver ("verified 2026-03-07" in OAA means against the APK's switch, not a phone) | partly: see the messages below |
| `RadioProgramListNotification` (0x801A), `RadioProgramInfoNotification` (0x801B), `RadioFavoriteListNotification` (0x8020) | silver | yes, the phone receives and displays them (openhu#78, Fairphone 4) |
| `RadioMuteRequest` (0x801C), `RadioTuneRequest` (0x801E), `RadioSearchRequest` (0x8023) | silver | yes, sent by the phone: mute, tune with `AMFM_FREQUENCY` 92600 (kHz), search with the action id `scan_fm` |
| `RadioFavoriteToggleRequest` (0x8021) | silver | yes, adding and removing a favorite works (#35) |
| `RadioMuteResponse` (0x801D), `RadioTuneResponse` (0x801F), `RadioTuneDirectionRequest` (0x8022) | silver | **unknown**: not named in the test report (openhu#78: no timeouts, no unreadable messages) |
| `RadioService` 2 `bands` (`RadioBands`, `RadioBandGroup`) | silver; OAA's channel documentation says field 1, its audit of the 17.3 trace says 2 | yes: the phone opened the radio channel with the band groups in field 2 |
| `RadioIdentifierType`, `RadioProgramIdentifier`, `RadioProgramSelector`, `RadioMetadata`, `RadioSongMetadata`, `RadioImage`, `RadioProgramInfo`, `RadioProgramType`, `RadioTuneStatus`, `RadioTuneDirection` | silver | used in the exchanges above; individual fields: **unknown** |
| `RadioBandType` | unverified (values from the 16.2 runtime enum) | no |
| `RadioProgramTypeSchema` | unverified (values 1–2 from the APK validator) | no |

Where OAA contradicts itself, aasdk decided and noted it in the proto: frequency in **kHz** (OAA's comment says Hz; confirmed by the phone, 92600), `RadioSearchRequest` field 1 named `action_id` (OAA's proto: `query`; semantics confirmed by the phone), bands in `RadioService` field **2**.

PR #35 (`2e7b70f`): field 3 of `RadioProgramInfoNotification` renamed from `has_audio_focus` (OAA) to `is_favorite`. Found with a phone: it is shown as the favorite heart of the current station (openhu#78). Wire format unchanged, generated accessors changed. OAA is wrong here.

The v1 radio API (0x8001–0x8019, upstream) stays in the protos. Whether current phones also use it is **unknown**; the Fairphone 4 used v2.

### Car control and the new service descriptors

PR #47 (`a222dca`, issue #40), 2026-10-19. Source: **AA app** analysis of Android Auto 17.9 (decompile; recorded in openhu#121), tested with a spike in OpenHU (Fairphone 4, 1080p video). The layout follows 17.9, **not** OAA, which differs in several places. Tier: not applicable. A spike result on the phone is stronger than OAA's tiers; static analysis alone is not.

| Definition | Phone-verified |
|---|---|
| `Service` 15 `car_control_service`, message ids 0x8001–0x8007 (`CarControlMessageId`) | yes (service shown as the climate rail; 0x8001–0x8005 exercised) |
| `CarControlService`, `CarControl`/`CarControlGroup`, `CarAction`, `CarActionEntry`, `CarProperty`, `CarPropertyConfig` (2 value type, 3 access, 5 area type), `CarAreaConfig` (2 max, 3 min), `CarPropertyValue`, `CarPropertyChangeEvent`, `CarPropertyControl`, `CarPropertyListenerResult`, `RegisterCarPropertyListeners{Request,Response}`, `SetCarPropertyValue{Request,Response}`, `GlobalArea`, `CarControlGroupType`, `CarControlSideAffinity` (2 driver, 3 passenger), `CarPropertyValueType`, `CarPropertyStatus` | yes (fields in the working descriptor; OAA differs in `CarPropertyConfig`, `CarAreaConfig` and side affinity) |
| `CarAreaId`, `CarAreaType`, `VehicleAreaSeat` | partly: global and seats; `ROW_1_LEFT`, `ROW_1_RIGHT` only. Windows, doors, mirrors, wheels: no |
| `VehiclePropertyId` | partly: only `HVAC_POWER_ON` and `IGNITION_STATE` are marked verified in the proto; the rest of the list no |
| `CarActionId` (`LAUNCH_HVAC = 1`, corrected after the first spike from the decompile, openhu#121) | no (the proto says not verified with a phone) |
| `CarActionNotification` (0x8006), `CarControlGroupUpdate` (0x8007), `CarActionControl`, `CarControlMetadata`, `CarPropertyAccess` (the phone does not read it; 1 and 2 are inferred), `Int32Values`, `Int64Values`, `FloatValues`, `VehicleAreaDoor`, `VehicleAreaMirror`, `VehicleAreaWheel`, `VehicleAreaWindow` | no |
| `Service` 16 `car_local_media_service`, 17 `buffered_media_sink_service`, 18 `car_intent_service`: empty descriptors, the phone only checks that they are present | no |
| `MessageStatus` -26 `STATUS_CAR_PROPERTY_SET_REQUEST_FAILED`, -27 `STATUS_CAR_PROPERTY_LISTENER_REGISTRATION_FAILED` | **unknown**: source and tier are not noted in the proto (came with #47) |

Before the 17.9 layout, openhu#121 collected the OAA description (`docs/channels/carcontrol.md`, mostly silver, "runtime unverified") as the concept. The spike showed that wrong values from OAA crash Android Auto on the phone (for example area type 1).

## Not added yet, known from other projects

The following gaps from #32 and #14 have **no** definitions in the protos. They are tracked in the issues and are listed so nobody reads the table above as complete:

| Gap | Source | Issue |
|---|---|---|
| `SensorBatch` 23–26 (energy model, trailer, raw energy model, raw EV trip settings), `SensorType` 25, 26, `EvConnectorType` additions | OAA (`oaa/sensor/VehicleEnergyModelData.proto`), AAPRS | #41 |
| `VehicleEnergyForecast`, `NavigationDestination.charging_station` | OAA (unverified) | #42 |
| `MediaSinkService.color_scheme_support`, `Start.session_type`, `UiConfig` 5–8, payloads for media ids 32780–32789 | OAA | #43 |
| `ByeByeReason` 6–8, `BluetoothPairingRequest.phone_name` | OAA | #44 |
| Conflicts: `PhoneStatus.State`, `AudioStreamType` ≥ 4, `NavigationNextTurnEvent`, `PingConfiguration`, `Config.Status`, GAL `WifiCredentialsResponse` | OHU, OAA | #45 |

Only in aasdk and unconfirmed by OAA or another project (#32, #45): `MediaMessageId` 0x800a and 0x8010, `ServiceDiscoveryResponse` 12 and 16, `WirelessTcpConfiguration` 4/5, `BluetoothAuthenticationData.pairing_method`, `InputReport.disp_channel_id`.

## Verified with a phone, independent of other projects

Facts found with the Fairphone 4 that corrected a source (not definitions taken from one): the radio field 3 (`is_favorite`), kHz and the band group field number (above), the 16128 byte frame limit (aasdk#37, upstream opencardev/aasdk#98; not a proto), the car control layout (above).
