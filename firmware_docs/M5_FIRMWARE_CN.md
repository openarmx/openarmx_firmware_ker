# KER M5 固件使用手册

[English](M5_FIRMWARE.md) | 简体中文

本文说明 KER M5 主控固件的配置、编译、烧录、标定、屏幕操作和通信测试。

## 1. 固件简介

M5 固件从 RS-485 总线读取 16 路磁编码器，将原始角度转换为 KER 关节角度，再通过
USB Vendor 或 WiFi 发送给主机。

```text
16 路编码器
  -> RS-485
  -> M5 角度处理、Zero 和机械偏移
  -> USB Vendor / WiFi
  -> 测试脚本或 ROS 2 driver
```

产品只使用一个固件。固件默认进入 USB 模式，可通过 KER Studio 在运行时切换 USB 和
WiFi，不需要重新编译或重启。USB CDC 管理串口始终用于 WiFi 配置、传输切换、状态读取、
标定和调试；它不是 ROS 数据传输通道。

USB 与 WiFi 不会同时发送关节数据，当前激活的传输方式使用相同的数据字段：

| 字段 | 类型 | 数量 | 说明 |
|---|---|---:|---|
| `timestamp` | `uint32` | 1 | M5 时间戳，单位为微秒。 |
| `angles` | `float` | 16 | 16 路关节角度，单位为度。 |
| `errors` | `bool` | 16 | 各通道读取错误状态。 |

固件采集任务以约 1 ms 周期运行。实际传输频率还会受到 RS-485、USB/WiFi 和主机处理
频率影响。

## 2. 配置说明

M5 工程位于：

```text
firmware/M5/
```

主要文件：

| 文件 | 作用 |
|---|---|
| `platformio.ini` | 单一 `ker` 产品固件的 PlatformIO 配置。 |
| `include/Common.h` | 引脚、16 路编码器方向、机械范围和偏移。 |
| `include/Meta.h` | 固件版本、硬件版本、USB VID/PID。 |
| `include/RuntimeStream.h` | USB/WiFi 运行时切换接口。 |
| `src/main.cpp` | 采集、管理命令、GUI 和 Zero 保存。 |
| `src/GUIHandler.cpp` | 屏幕柱状图和触摸按钮。 |
| `src/USBStream.cpp` | USB vendor 通信。 |
| `src/WiFiStream.cpp` | WiFi TCP 服务。 |
| `src/RuntimeStream.cpp` | 当前传输管理与断线恢复。 |

### 2.1 WiFi 配置

WiFi 名称和密码不写入源码。烧录统一的 `ker` 固件后，通过 USB 连接 M5 并启动 KER
Studio：

```bash
cd /home/openflex/openflex_all/openflex_ws/src/m_ker
python3 tool/m5_configurator/main.py
```

在“WiFi 配置”页面选择 M5 管理串口，扫描或手动填写网络并保存。配置保存在 M5 的 NVS
中，断电重启后仍然有效。保存 WiFi 后可直接将传输方式切换为 WiFi，无需重启。

- M5 使用 2.4 GHz WiFi。
- 主机名 `openarm-ker` 通常可通过 `openarm-ker.local` 访问。
- TCP 端口必须与测试脚本和 ROS driver 一致。
- USB CDC 管理串口波特率为 `115200`，用于配置、标定和调试。
- 当前协议没有加密和身份认证，应在可信局域网中使用。
- WiFi 同时只接受一个 TCP 客户端。

### 2.2 编码器配置

`include/Common.h` 中每个通道的格式为：

```cpp
{invert, mech_min, mech_max, mech_joint_offset}
```

| 参数 | 说明 |
|---|---|
| `invert` | 是否反转编码器方向。 |
| `mech_min` | 关节机械最小角度，单位为度。 |
| `mech_max` | 关节机械最大角度，单位为度。 |
| `mech_joint_offset` | 执行 Zero 后，该姿态对应的机器人关节角度。 |


## 3. 编译与烧录

### 3.1 安装 PlatformIO

```bash
python3 -m pip install --user platformio
pio --version
```

如果终端找不到 `pio`，可将下面命令中的 `pio` 替换为 `python3 -m platformio`。

进入工程：

```bash
cd /home/openflex/openflex_all/openflex_ws/src/m_ker/firmware/M5
```

### 3.2 编译

```bash
pio run
```

主要输出文件位于：

```text
.pio/build/ker/firmware.bin
.pio/build/ker/firmware_merged.bin
```

`firmware.bin` 只包含应用程序，不能从地址 `0x0` 单独烧录。用于上位机或 `esptool` 单文件
烧录时必须选择 `firmware_merged.bin`，其中包含 bootloader、分区表、boot application 和
主应用程序。

### 3.3 烧录

用支持数据传输的 USB 线连接 M5，长按复位键约 3 秒进入烧录模式，然后运行：

```bash
pio run --target upload
```

指定烧录端口：

```bash
pio device list
pio run --target upload --upload-port /dev/ttyACM0
```

烧录完成后按一次复位键启动固件。

## 4. 标定

### 4.1 标定前准备

- 确认 16 个编码器 ID 与关节顺序正确。
- 确认各关节在机械范围内，并且编码器安装牢固。
- 标定时不要运行遥操 bridge，不要向真实机械臂发送控制命令。
- 将 KER 放到预定的标准标定姿态。

通道顺序：

| 通道 | 对应关节 |
|---:|---|
| 1-7 | 右臂 J1-J7 |
| 8 | 右夹爪 |
| 9-15 | 左臂 J1-J7 |
| 16 | 左夹爪 |

### 4.2 Zero All

1. 将 KER 调整到标准标定姿态。
2. 确认屏幕 16 路柱状图没有显示 `ERR`。
3. 点击左下角 `Zero Reset (All)`。
4. 在确认框中点击 `YES`。
5. 等待角度刷新，然后缓慢移动各关节检查方向。

Zero 数据保存在 M5 的 NVS 中，正常断电重启后不会丢失。

### 4.3 单通道 Zero

如果只重新安装了一个编码器，无需重新标定全部通道：

1. 点击对应柱状图，使其变为亮蓝色。
2. 点击左下角 `Confirm Zero`。
3. 在确认框中点击 `YES`。

### 4.4 标定后检查

- 重启 M5，确认 Zero 仍然有效。
- 缓慢移动每个关节，确认角度方向正确且连续。
- 确认角度不会无故变化约 360 度。
- 确认机械极限附近的数值符合 `mech_min` 和 `mech_max`。
- 连接测试脚本或 ROS 后检查 `error_mask`；完整 16 路设备应为 `0`。

如果方向相反，修改对应通道的 `invert` 后重新编译烧录。

## 5. 屏幕操作

| 显示或按钮 | 作用 |
|---|---|
| 1-16 柱状图 | 显示各通道角度；`ERR` 表示读取异常。 |
| `Zero Reset (All)` | 标定全部通道。 |
| `Confirm Zero` | 标定已选中的橙色通道。 |
| `START` | 进入 STREAM，开始发送数据。 |
| `STOP` | 返回 STANDBY，停止发送数据。 |
| `WIFI <IP> C` | WiFi 已连接，并有 TCP 客户端。 |
| `USB CONNECTED` | USB 客户端已连接。 |

进入 STREAM 后柱状图继续实时显示，只有右下角按钮由 `START` 变为 `STOP`。

## 6. 数据测试

### 6.1 WiFi 独立测试

WiFi 测试脚本不依赖 ROS：

```bash
cd /home/openflex/openflex_all/openflex_ws/src/m_ker
python3 firmware/test/wifi_stream_test.py 192.168.3.114
```

也可以使用 mDNS：

```bash
python3 firmware/test/wifi_stream_test.py openarm-ker.local
```

脚本会执行 TCP 连接、PING/schema、STREAM、checksum 校验，并打印角度、错误位和接收
频率。测试时不要同时启动 ROS driver，因为 WiFi 模式只允许一个客户端。

### 6.2 ROS 测试

启动 KER driver 后检查：

```bash
ros2 service call /ker/ping_wifi std_srvs/srv/Trigger
ros2 topic echo /ker/metadata --once
ros2 topic hz /ker/joint_states
ros2 topic echo /ker/error_mask --once
```

默认 ROS driver 以最高约 100 Hz 发布 `/ker/joint_states`，bridge 默认以 50 Hz 输出控制
命令。

## 7. 常见问题

### M5 一直显示 `WIFI --` 且尚未配置

通过 USB 启动 `tool/m5_configurator/main.py`，在“WiFi 配置”页面写入网络。首次烧录后
NVS 中没有 WiFi 凭据，显示 `WIFI --` 属于正常状态。

### M5 一直显示 `WIFI --`

- 确认使用 2.4 GHz WiFi。
- 检查 SSID、密码和信号强度。
- 确认路由器没有启用客户端隔离。

### `openarm-ker.local` 无法访问

直接使用 M5 屏幕显示的 IPv4 地址。

### WiFi 已连接但没有数据

- 确认只运行了一个客户端。
- 确认 M5 已进入 STREAM。
- 确认客户端完成 PING/schema 后发送了 STREAM 命令。
- 检查端口是否为 `19090`。

### USB 提示权限不足

临时测试可以使用管理员权限确认问题，但正常使用应配置 udev 规则：

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="303a", MODE="0666"' | \
  sudo tee /etc/udev/rules.d/99-m5stack.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

重新插拔 M5 后再启动 driver。

### 角度方向相反

检查对应通道的 `invert`。如果只是 KER 与 OpenArmX 坐标系定义不同，则应在 ROS
`joint_scales` 中修正，不要修改两次。

### 角度跳变约 360 度

检查 Zero 姿态、`mech_min`、`mech_max` 和编码器安装方向。滤波只能减小突变影响，不能
代替正确标定。
