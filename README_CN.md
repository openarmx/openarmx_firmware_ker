# OpenArmX KER 固件

[English](README.md) | 简体中文

OpenArmX KER 双臂主手固件。编码器固件通过 RS-485 返回关节传感器数据，M5 固件汇总 16 路数据，并通过 USB Vendor 或 WiFi 发送到主机。

## 目录内容

- `M5/` - M5Stack CoreS3 固件、屏幕界面、标定、USB/WiFi 传输和 USB CDC 配置。
- `encoder/` - ATtiny1616 与 TLE5012B 编码器固件，支持 ID 1-16。
- `test/` - 编码器 ID 扫描、RS-485 通信诊断和 WiFi 数据流测试。
- `firmware_docs/` - M5 与编码器固件的中英文详细手册。

## 环境要求

- M5Stack CoreS3-SE。
- ATtiny1616 编码器板和 RS-485 总线。
- Python 3 与 PlatformIO Core。
- 用于 M5 烧录的 USB 数据线。
- 用于编码器烧录的 SerialUPDI 烧录器。

## 获取仓库

```bash
git clone https://github.com/openarmx/openarmx_firmware_ker.git
cd openarmx_firmware_ker
python3 -m pip install --user platformio pyserial
```

## M5 编译与烧录

产品只使用一个 M5 固件。固件默认进入 USB 模式，并可通过 KER Studio 在运行时切换 USB 和 WiFi。

```bash
cd M5
pio run
pio run --target upload
```

可从地址 `0x0` 直接烧录的完整镜像位于：

```text
M5/.pio/build/ker/firmware_merged.bin
```

WiFi 默认端点为 `openarm-ker.local:19090`。SSID、密码、传输模式、传感器状态和零位标定均通过 KER Studio 的 USB CDC 接口管理。

## 编码器编译与烧录

每个编码器必须使用 1-16 中唯一的 ID。下面示例烧录 ID 5：

```bash
cd encoder
PLATFORMIO_BUILD_FLAGS="-DDEVICE_ID=5" \
  pio run --target upload --upload-port /dev/ttyUSB0
```

同一条 RS-485 总线上不能存在重复 ID。

## 通信测试

```bash
# 通过 USB-RS485 模块扫描编码器 ID
python3 test/scan_id.py

# 测试 M5 WiFi schema 和数据流
python3 test/wifi_stream_test.py openarm-ker.local
```

M5 同时只接受一个 WiFi 客户端。运行独立 WiFi 测试前应先停止 ROS driver。

## 详细文档

- [M5 固件手册](firmware_docs/M5_FIRMWARE_CN.md) | [English](firmware_docs/M5_FIRMWARE.md)
- [编码器固件手册](firmware_docs/ENCODER_FIRMWARE_CN.md) | [English](firmware_docs/ENCODER_FIRMWARE.md)

## 许可证

本项目采用知识共享署名-非商业性使用-相同方式共享 4.0 国际许可证（CC BY-NC-SA 4.0）。

版权所有 (c) 2026 原初智能机器人有限公司。

详细信息请查看 [LICENSE](LICENSE.txt)，或访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

## 作者

- **魏林栋** (Wei Lindong)
- **赵天赫** (Zhao Tianhe)
- 公司：原初智能机器人有限公司
- 网站：https://openarmx.com/

## 版本

**当前 M5 固件版本**：3.3.3

- v3.3.3 版本说明将由 Release 工作流自动生成。

## GitHub 与 Gitee 发布同步

推送版本标签后，**Build and Release Firmware** 会编译固件并创建 GitHub Release，
随后直接调用 **Sync to Gitee**，推送分支和标签，并将发布说明及所有附件同步到
[Gitee](https://gitee.com/chengdu-changshu-robot/openarmx_firmware_ker/releases)。
标签必须与 `M5/include/Meta.h` 中的 `FW_VERSION` 一致。

请在 GitHub 仓库的 **Settings → Secrets and variables → Actions** 中配置：

- `GITEE_SSH_PRIVATE_KEY`：SSH 私钥，对应公钥需有 Gitee 仓库写权限。
- `GITEE_ACCESS_TOKEN`：Gitee 访问令牌，需有该仓库的 Release 创建和附件上传权限。

凭据保存在 GitHub Secrets 中，工作流文件只引用名称。缺少凭据时，同步会明确报错。
如需补同步已有 GitHub Release，可手动运行 **Sync to Gitee**，将 `release_tag` 填为对应标签，
例如 `v3.3.3`。重复运行会替换 Gitee 中同标签的 Release，并重新上传 GitHub 附件。
仅同步代码和标签时，留空 `release_tag`。如果 Gitee 已有同名标签且指向相同提交，
即使轻量标签与附注标签的类型不同，也会保留并继续同步；指向不同提交时会明确报错。

## 致谢

本固件属于 OpenArmX 机器人平台生态，并包含基于 OpenArm KER 项目的衍生工作。

---

## 联系我们

### 原初智能机器人有限公司

| 联系方式 | 信息 |
|---|---|
| 邮箱 | [openarmrobot@gmail.com](mailto:openarmrobot@gmail.com) |
| 电话 / 微信 | +86-17746530375 |
| 官方网站 | [https://openarmx.com/](https://openarmx.com/) |
| 在线文档 | [http://docs.openarmx.com/](http://docs.openarmx.com/) |
| 地址 | 天津经济技术开发区西区新业八街 11 号华城机械厂 |
| 联系人 | 王先生 |
