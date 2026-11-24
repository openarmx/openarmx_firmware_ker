# OpenArmX KER Firmware

English | [简体中文](README_CN.md)

Firmware for the OpenArmX KER bimanual leader device. The encoder firmware reads joint sensors over RS-485, while the M5 firmware aggregates 16 channels and streams joint data to a host through USB Vendor or WiFi.

## Contents

- `M5/` - M5Stack CoreS3 firmware, display UI, calibration, USB/WiFi transport, and USB CDC configuration.
- `encoder/` - ATtiny1616 and TLE5012B encoder firmware with IDs 1-16.
- `test/` - Encoder ID scanning, RS-485 diagnostics, and WiFi stream tests.
- `firmware_docs/` - Detailed M5 and encoder firmware guides in English and Chinese.

## Requirements

- M5Stack CoreS3-SE.
- ATtiny1616 encoder boards and an RS-485 bus.
- Python 3 and PlatformIO Core.
- A USB data cable for M5 flashing.
- A SerialUPDI programmer for encoder flashing.

## Repository Setup

```bash
git clone https://github.com/openarmx/openarmx_firmware_ker.git
cd openarmx_firmware_ker
python3 -m pip install --user platformio pyserial
```

## Build And Flash M5

The product uses one M5 firmware. It starts in USB mode and can switch between USB and WiFi at runtime through KER Studio.

```bash
cd M5
pio run
pio run --target upload
```

The complete image for flashing at address `0x0` is generated at:

```text
M5/.pio/build/ker/firmware_merged.bin
```

WiFi defaults to `openarm-ker.local:19090`. SSID, password, transport selection, sensor status, and zero calibration are managed through the USB CDC interface in KER Studio.

## Build And Flash Encoders

Each encoder must have a unique ID from 1 to 16. The example below flashes ID 5:

```bash
cd encoder
PLATFORMIO_BUILD_FLAGS="-DDEVICE_ID=5" \
  pio run --target upload --upload-port /dev/ttyUSB0
```

Do not place duplicate IDs on the same RS-485 bus.

## Test

```bash
# Scan encoder IDs through a USB-RS485 adapter
python3 test/scan_id.py

# Test M5 WiFi schema and streaming
python3 test/wifi_stream_test.py openarm-ker.local
```

Only one WiFi client can connect to the M5 at a time. Stop the ROS driver before running the standalone WiFi test.

## Documentation

- [M5 firmware guide](firmware_docs/M5_FIRMWARE.md) | [简体中文](firmware_docs/M5_FIRMWARE_CN.md)
- [Encoder firmware guide](firmware_docs/ENCODER_FIRMWARE.md) | [简体中文](firmware_docs/ENCODER_FIRMWARE_CN.md)

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 原初智能机器人有限公司

For details, see the [LICENSE](LICENSE.txt) file or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **Wei Lindong** (魏林栋)
- **Zhao Tianhe** (赵天赫)
- Company: 原初智能机器人有限公司
- Website: https://openarmx.com/

## Version

**Current M5 Firmware Version**: 3.3.3

- Release notes will be generated for v3.3.3 by the release workflow.

## GitHub and Gitee Releases

Pushing a version tag runs **Build and Release Firmware**. After creating the
GitHub Release, it calls **Sync to Gitee** to push branches and tags, then copy
the release notes and all assets to
[Gitee](https://gitee.com/chengdu-changshu-robot/openarmx_firmware_ker/releases).
The tag must match `FW_VERSION` in `M5/include/Meta.h`.

M5 builds pin Espressif32 7.1.1 (Arduino ESP32 2.0.17) and validated library
versions. To retry a failed release for an existing tag, first commit and push
the build fix, then open GitHub Actions → **Build and Release Firmware** →
**Run workflow**, select the updated `main` branch, and enter `v3.3.3` as `tag`.
Firmware sources come from the tag; the M5 build configuration comes from the
selected workflow commit. Both commit IDs are recorded in `manifest.json`.
There is no need to recreate the tag. Re-running an old job uses its old workflow.

Configure these GitHub repository secrets under **Settings → Secrets and
variables → Actions**:

- `GITEE_SSH_PRIVATE_KEY`: SSH private key whose public key has write access to the Gitee repository.
- `GITEE_ACCESS_TOKEN`: Gitee access token with permission to create releases and upload attachments in that repository.

Credentials are stored in GitHub Secrets, not in workflow files. Missing secrets
cause synchronization to fail with an explicit error. To copy an existing
GitHub Release, manually run **Sync to Gitee** and set `release_tag` to its tag,
for example `v3.3.3`. Re-running replaces the Gitee Release for that tag and
uploads the GitHub assets again.
Leave `release_tag` empty to synchronize only code and tags. Existing Gitee tags
are preserved when they point to the same commit, including lightweight versus
annotated tag differences. Tags pointing to different commits fail explicitly.

## Acknowledgments

This firmware is part of the OpenArmX robotic platform ecosystem and includes work derived from the OpenArm KER project.

---

## Contact Us

### 原初智能机器人有限公司

| Contact | Information |
|---|---|
| Email | [openarmrobot@gmail.com](mailto:openarmrobot@gmail.com) |
| Phone / WeChat | +86-17746530375 |
| Website | [https://openarmx.com/](https://openarmx.com/) |
| Documentation | [http://docs.openarmx.com/](http://docs.openarmx.com/) |
| Address | Huacheng Machinery Plant, No.11 Xinye 8th Street, West Area, Tianjin Economic-Technological Development Area |
| Contact Person | Mr. Wang |
