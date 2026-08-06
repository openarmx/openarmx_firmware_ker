# KER Encoder Firmware Guide

English | [简体中文](ENCODER_FIRMWARE_CN.md)

This guide covers encoder IDs, wiring, firmware flashing, and RS-485 communication tests for the KER magnetic encoder board.

## 1. Overview

Each board uses an ATtiny1616 to read a TLE5012B magnetic sensor over SSC and return its angle to the M5 over RS-485.

```text
magnet rotation
  -> TLE5012B
  -> ATtiny1616
  -> RS-485 at 2 Mbps
  -> M5 controller
```

| Item | Configuration |
|---|---|
| MCU | ATtiny1616 at 20 MHz |
| Sensor | TLE5012B |
| Sensor interface | SSC |
| Bus | RS-485 at 2 Mbps |
| Packet | 4 bytes |
| Device ID | Selected at build time |

The encoder returns raw angle data. Joint direction, mechanical range, joint offset, and zero calibration are handled by the M5 firmware.

## 2. ID Assignment

The protocol supports IDs 0-31. KER currently uses IDs 1-16:

| Encoder ID | Joint |
|---:|---|
| 1-7 | Right arm J1-J7 |
| 8 | Right gripper |
| 9-15 | Left arm J1-J7 |
| 16 | Left gripper |

Do not connect duplicate IDs to one RS-485 bus. Multiple boards replying to the same request cause collisions and invalid angle data. A replacement board must be flashed with the ID assigned to its physical joint.

## 3. Wiring and Programmer

Use a USB SerialUPDI programmer, such as an Adafruit UPDI Friend or a compatible adapter.

For flashing, connect:

- UPDI signal.
- Encoder board power and GND.
- Common ground between the programmer and encoder board.

For operation, connect RS-485 A, B, and GND. Reversed A/B wiring normally results in no detected IDs or no responses.

## 4. Build and Flash

The encoder project is located at `firmware/encoder/`.

### 4.1 Install PlatformIO

```bash
python3 -m pip install --user platformio
pio --version
```

### 4.2 Select an ID and Flash

```bash
cd /path/to/m_ker/firmware/encoder
PLATFORMIO_BUILD_FLAGS="-DDEVICE_ID=5" \
  pio run --target upload --upload-port /dev/ttyUSB0
```

The example flashes ID 5. Change only `DEVICE_ID` for another joint:

```bash
# Right arm J1
PLATFORMIO_BUILD_FLAGS="-DDEVICE_ID=1" \
  pio run --target upload --upload-port /dev/ttyUSB0

# Left gripper
PLATFORMIO_BUILD_FLAGS="-DDEVICE_ID=16" \
  pio run --target upload --upload-port /dev/ttyUSB0
```

List serial ports with:

```bash
pio device list
```

Prebuilt `firmware_IDxx.hex` files are stored under `firmware/encoder/firmware/` for production and recovery. For source builds, always provide `DEVICE_ID` explicitly so a stale build configuration cannot assign the wrong ID.

## 5. Communication Tests

Testing requires a USB-RS485 adapter, or a USB-UART adapter connected to an automatic-direction TTL-RS485 module. The complete path must support 2 Mbps.

```bash
python3 -m pip install pyserial
```

Test scripts are located in `firmware/test/`.

### 5.1 Scan IDs

Set the port in `firmware/test/scan_id.py`:

```python
PORT = '/dev/ttyUSB1'
BAUD = 2000000
```

Then run:

```bash
cd /path/to/m_ker
python3 firmware/test/scan_id.py
```

Each connected board should appear under exactly one expected ID. If responses are unstable, disconnect other boards and test one board at a time.

### 5.2 Read One Angle

Set the port and ID in `firmware/test/test.py`:

```python
PORT = '/dev/ttyUSB1'
BAUD = 2000000
DEVICE_ID = 5
```

Run:

```bash
python3 firmware/test/test.py
```

Rotate the magnet or joint slowly and verify that the angle changes continuously. Press `Ctrl+C` to stop.

## 6. Troubleshooting

### UPDI Programmer Not Found

- Use a USB data cable.
- Check the port with `pio device list`.
- Verify serial-port permissions.
- Typical ports are `/dev/ttyUSB0` on Linux and `COMx` on Windows.

### Flashing Fails

- Check UPDI, power, and common ground.
- Verify that the selected port is the UPDI programmer, not the RS-485 test adapter.
- Avoid conflicting external power supplies.

### No IDs Are Detected

- Check encoder power and common ground.
- Swap RS-485 A/B if their labeling may be reversed.
- Verify 2 Mbps support across the USB and RS-485 adapters.
- Use an automatic-direction RS-485 module, or provide correct DE/RE control.
- Test one encoder board at a time.
- Confirm the flashed `DEVICE_ID`.

### Angle Is Static or Jumps

Check magnet distance, alignment, mounting, sensor power, bus noise, and duplicate IDs. Validate a single board before reconnecting the complete chain.

### M5 Shows `ERR`

First verify the encoder with `scan_id.py` and `test.py`. Then check M5-side A/B, GND, power, and ID assignment before running M5 zero calibration.
