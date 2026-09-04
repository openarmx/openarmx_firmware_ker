# KER M5 Firmware Guide

English | [简体中文](M5_FIRMWARE_CN.md)

This guide covers configuration, building, flashing, calibration, display operation, and communication testing for the KER M5 controller firmware.

## 1. Overview

The M5 reads 16 magnetic encoders over RS-485, converts raw angles into KER joint angles, and streams the result to a host over USB Vendor or WiFi.

```text
16 encoders
  -> RS-485
  -> angle processing, zero offsets, mechanical offsets
  -> USB Vendor / WiFi
  -> test tool or ROS 2 driver
```

The product uses one firmware image. It starts in USB mode and can switch between USB and WiFi at runtime through KER Studio without rebuilding or rebooting. The USB CDC management port remains available for WiFi setup, transport switching, status queries, calibration, and debugging. It is not the ROS data transport.

USB and WiFi do not stream joint data at the same time. Both transports use the same schema:

| Field | Type | Count | Description |
|---|---|---:|---|
| `timestamp` | `uint32` | 1 | M5 timestamp in microseconds. |
| `angles` | `float` | 16 | Joint angles in degrees. |
| `errors` | `bool` | 16 | Per-channel read error flags. |

The acquisition task runs at approximately 1 ms per cycle. Actual stream rate also depends on RS-485, the active transport, and host processing.

## 2. Configuration

The project is located at `firmware/M5/`.

| File | Purpose |
|---|---|
| `platformio.ini` | PlatformIO configuration for the single `ker` product firmware. |
| `include/Common.h` | Pins, encoder direction, mechanical limits, and offsets. |
| `include/Meta.h` | Firmware version, hardware version, USB VID/PID, hostname, and port. |
| `include/RuntimeStream.h` | Runtime USB/WiFi transport interface. |
| `src/main.cpp` | Acquisition, management commands, GUI handling, and zero persistence. |
| `src/GUIHandler.cpp` | Display bars and touch controls. |
| `src/USBStream.cpp` | USB Vendor transport. |
| `src/WiFiStream.cpp` | WiFi TCP service. |
| `src/RuntimeStream.cpp` | Active transport and reconnect handling. |

### 2.1 WiFi

WiFi credentials are not compiled into the firmware. Connect the M5 over USB and start KER Studio:

```bash
cd /path/to/m_ker
python3 tool/m5_configurator/main.py
```

Select the M5 management port on the WiFi page, scan for or enter a network, and save it. Credentials and the selected transport are stored in NVS. After saving valid credentials, the firmware can switch to WiFi immediately without rebooting.

- Only 2.4 GHz WiFi is supported.
- The default hostname is `openarm-ker`, normally available as `openarm-ker.local`.
- The default TCP port is `19090`.
- The USB CDC management baud rate is `115200`.
- WiFi accepts one TCP client at a time.
- The protocol has no encryption or authentication; use a trusted LAN.

### 2.2 Encoder Channels

Each entry in `include/Common.h` has this form:

```cpp
{invert, mech_min, mech_max, mech_joint_offset}
```

| Parameter | Description |
|---|---|
| `invert` | Reverses the encoder direction. |
| `mech_min` | Mechanical minimum in degrees. |
| `mech_max` | Mechanical maximum in degrees. |
| `mech_joint_offset` | Robot joint angle represented by the calibrated zero pose. |

## 3. Build and Flash

### 3.1 Install PlatformIO

```bash
python3 -m pip install --user platformio
pio --version
cd /path/to/m_ker/firmware/M5
```

If `pio` is not on `PATH`, replace it with `python3 -m platformio`.

### 3.2 Build

```bash
pio run
```

The main outputs are:

```text
.pio/build/ker/firmware.bin
.pio/build/ker/firmware_merged.bin
```

`firmware.bin` contains only the application and must not be flashed by itself at address `0x0`. Use `firmware_merged.bin` for one-file flashing from `0x0`; it contains the bootloader, partition table, boot application, and main application.

### 3.3 Flash

Connect a USB data cable and hold RESET for about three seconds to enter download mode:

```bash
pio run --target upload
```

To select a port explicitly:

```bash
pio device list
pio run --target upload --upload-port /dev/ttyACM0
```

Press RESET once after flashing.

## 4. Calibration

### 4.1 Preparation

- Verify that encoder IDs match their joint positions.
- Keep every joint within its mechanical range.
- Stop the teleoperation bridge and all commands to a real robot.
- Place the KER in the defined calibration pose.
- Confirm that the display does not show `ERR` on required channels.

| Channel | Joint |
|---:|---|
| 1-7 | Right arm J1-J7 |
| 8 | Right gripper |
| 9-15 | Left arm J1-J7 |
| 16 | Left gripper |

### 4.2 Zero All Channels

1. Place the KER in the calibration pose.
2. Check all channel bars for communication errors.
3. Tap `Zero Reset (All)`.
4. Confirm the operation.
5. Move each joint slowly and verify direction and continuity.

Zero offsets are stored in NVS and survive power cycles.

### 4.3 Zero Selected Channels

Tap the required bars to select them, tap `Confirm Zero`, and confirm. Use this after replacing or adjusting only part of the encoder chain.

### 4.4 Verification

- Reboot and verify that calibration is retained.
- Move every joint slowly and check direction and continuity.
- Check for unexpected jumps near 360 degrees.
- Verify values near configured mechanical limits.
- For a complete 16-channel system, the ROS `error_mask` should be `0`.

If a direction is reversed, correct the channel `invert` setting and rebuild. Do not reverse the same joint in both firmware and ROS.

## 5. Display

| Display or control | Purpose |
|---|---|
| Channel bars 1-16 | Live joint values; `ERR` indicates a read failure. |
| `Zero Reset (All)` | Calibrates all channels. |
| `Confirm Zero` | Calibrates selected channels. |
| `START` | Enters STREAM and starts data transmission. |
| `STOP` | Returns to STANDBY and stops transmission. |
| WiFi status | Shows connection state, IP address, and client state. |
| USB status | Shows USB client state. |

The bars remain visible while streaming; only the lower-right button changes from `START` to `STOP`.

## 6. Tests

### 6.1 Standalone WiFi Test

```bash
cd /path/to/m_ker
python3 firmware/test/wifi_stream_test.py openarm-ker.local
```

An IPv4 address may be used instead of mDNS. The script checks TCP connection, PING/schema, STREAM, checksum, angle data, error bits, and receive rate. Stop the ROS driver first because the M5 accepts only one WiFi client.

### 6.2 ROS 2 Test

After starting the KER driver:

```bash
ros2 service call /ker/ping_wifi std_srvs/srv/Trigger
ros2 topic echo /ker/metadata --once
ros2 topic hz /ker/joint_states
ros2 topic echo /ker/error_mask --once
```

## 7. Troubleshooting

### WiFi Is Not Configured

Open KER Studio over USB and save a 2.4 GHz network. An empty WiFi state after first flashing is expected because NVS does not contain credentials yet.

### `openarm-ker.local` Does Not Resolve

Use the IPv4 address shown by the M5 display and verify that the host and M5 are on the same LAN.

### WiFi Connects but No Data Arrives

- Ensure that only one client is connected.
- Ensure that the M5 is in STREAM mode.
- Use TCP port `19090`.
- Stop standalone tests before starting ROS, or stop ROS before running a standalone test.

### USB Permission Denied on Linux

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="303a", MODE="0666"' | \
  sudo tee /etc/udev/rules.d/99-m5stack.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Reconnect the M5 after applying the rule.

### Reversed or Discontinuous Angles

Check encoder installation, the zero pose, `invert`, mechanical limits, and offsets. Filtering can reduce the effect of a transient but cannot replace correct wiring and calibration.
