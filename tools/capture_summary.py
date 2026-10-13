#!/usr/bin/env python3
"""Summary of a protocol capture written with AASDK_CAPTURE=<file> (average-web-dev/aasdk#14).

    tools/capture_summary.py capture.jsonl              messages per channel, direction and id
    tools/capture_summary.py capture.jsonl --unknown    also decode one sample of every unknown id
                                                        (needs protoc: protoc --decode_raw)

Names come from the message id enums in protobuf/aap_protobuf. A message whose id no enum of its
channel knows is marked "UNKNOWN": these are the ids to look at (new protocol versions, #14).
"""

import argparse
import collections
import json
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent / "protobuf" / "aap_protobuf"

# message id enums per channel (ChannelId names as in aasdk)
MEDIA = ["service/media/sink/MediaMessageId.proto"]
CHANNEL_ENUMS = {
    "CONTROL": ["service/control/ControlMessageType.proto"],
    "SENSOR": ["service/sensorsource/SensorMessageId.proto"],
    "MEDIA_SINK": MEDIA,
    "MEDIA_SINK_VIDEO": MEDIA,
    "MEDIA_SINK_MEDIA_AUDIO": MEDIA,
    "MEDIA_SINK_GUIDANCE_AUDIO": MEDIA,
    "MEDIA_SINK_SYSTEM_AUDIO": MEDIA,
    "MEDIA_SINK_TELEPHONY_AUDIO": MEDIA,
    "MEDIA_SOURCE_MICROPHONE": MEDIA,
    "INPUT_SOURCE": ["service/inputsource/InputMessageId.proto"],
    "BLUETOOTH": ["service/bluetooth/BluetoothMessageId.proto"],
    "RADIO": ["service/radio/RadioMessageId.proto"],
    "NAVIGATION_STATUS": ["service/navigationstatus/NavigationStatusMessageId.proto"],
    "MEDIA_PLAYBACK_STATUS": ["service/mediaplayback/MediaPlaybackStatusMessageId.proto"],
    "PHONE_STATUS": ["service/phonestatus/PhoneStatusMessageId.proto"],
    "MEDIA_BROWSER": ["service/mediabrowser/MediaBrowserMessageId.proto"],
    "VENDOR_EXTENSION": ["channel/control/GalVerificationVendorExtensionMessageId.proto"],
    "GENERIC_NOTIFICATION": ["service/genericnotification/GenericNotificationMessageId.proto"],
    "WIFI_PROJECTION": ["service/wifiprojection/WifiProjectionMessageId.proto"],
}
CONTROL = CHANNEL_ENUMS["CONTROL"]


def enum_values(files):
    names = {}
    for name in files:
        text = re.sub(r"//[^\n]*", "", (ROOT / name).read_text())
        for value, number in re.findall(r"^\s*(\w+)\s*=\s*(-?\w+)", text, re.M):
            try:
                names.setdefault(int(number, 0), value)
            except ValueError:
                pass
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("capture")
    parser.add_argument("--unknown", action="store_true", help="decode one sample of every unknown id")
    args = parser.parse_args()

    stats = collections.OrderedDict()
    for number, line in enumerate(open(args.capture), 1):
        try:
            entry = json.loads(line)
        except json.JSONDecodeError:
            print(f"line {number}: not JSON, skipped", file=sys.stderr)
            continue
        channel = entry["chName"]
        # control message type on a service channel: channel open/close, use the control ids
        files = CONTROL if entry["control"] or channel == "CONTROL" else CHANNEL_ENUMS.get(channel, [])
        name = enum_values(files).get(entry["id"]) or "UNKNOWN"
        key = (channel, entry["dir"], entry["id"], name)
        item = stats.setdefault(key, {"count": 0, "bytes": 0, "first": entry["t"], "sample": None})
        item["count"] += 1
        item["bytes"] += entry["len"]
        if item["sample"] is None and "hex" in entry:
            item["sample"] = entry["hex"]

    print(f"{'channel':<28} {'dir':<4} {'id':>6}  {'name':<56} {'count':>6} {'bytes':>9} {'first ms':>9}")
    for (channel, direction, message_id, name), item in sorted(stats.items(), key=lambda kv: (kv[0][0], kv[0][1], kv[0][2])):
        print(f"{channel:<28} {direction:<4} {message_id:>6}  {name:<56} {item['count']:>6} {item['bytes']:>9} {item['first']:>9}")

    unknown = [(key, item) for key, item in stats.items() if key[3] == "UNKNOWN"]
    print(f"\n{len(unknown)} unknown id(s)")
    if args.unknown and unknown:
        protoc = shutil.which("protoc")
        for (channel, direction, message_id, _), item in unknown:
            print(f"\n== {channel} {direction} {message_id} (0x{message_id:04x})")
            if not item["sample"]:
                print("   no payload in the capture")
                continue
            payload = bytes.fromhex(item["sample"])[2:]
            print(f"   {payload.hex()}")
            if protoc:
                result = subprocess.run([protoc, "--decode_raw"], input=payload, capture_output=True)
                print("   " + (result.stdout.decode(errors="replace") or result.stderr.decode(errors="replace")).strip().replace("\n", "\n   "))


if __name__ == "__main__":
    main()
