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

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd. (成都长数机器人有限公司)

For details, see the [LICENSE](LICENSE.txt) file or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **Zhang Li** (张力)
- Company: Chengdu Changshu Robot Co., Ltd. (成都长数机器人有限公司)
- Website: https://openarmx.com/

## Version

**Current M5 Firmware Version**: 3.3.0

- [v3.3.0 Release Notes](release_notes/v3.3.0.md)

## Acknowledgments

This firmware is part of the OpenArmX robotic platform ecosystem and includes work derived from the OpenArm KER project.

---

## Contact Us

### Chengdu Changshu Robot Co., Ltd.

| Contact | Information |
|---|---|
| Email | [openarmrobot@gmail.com](mailto:openarmrobot@gmail.com) |
| Phone / WeChat | +86-17746530375 |
| Website | [https://openarmx.com/](https://openarmx.com/) |
| Documentation | [http://docs.openarmx.com/](http://docs.openarmx.com/) |
| Address | Huacheng Machinery Plant, No.11 Xinye 8th Street, West Area, Tianjin Economic-Technological Development Area |
| Contact Person | Mr. Wang |
