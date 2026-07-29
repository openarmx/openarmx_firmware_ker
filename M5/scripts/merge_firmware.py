Import("env")

from pathlib import Path
import subprocess


def merge_firmware(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    framework_dir = Path(
        env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    )
    esptool_dir = Path(env.PioPlatform().get_package_dir("tool-esptoolpy"))
    output_path = build_dir / "firmware_merged.bin"

    command = [
        env.subst("$PYTHONEXE"),
        str(esptool_dir / "esptool.py"),
        "--chip", "esp32s3",
        "merge_bin",
        "--flash_mode", "keep",
        "--flash_freq", "keep",
        "--flash_size", "keep",
        "-o", str(output_path),
        "0x0", str(build_dir / "bootloader.bin"),
        "0x8000", str(build_dir / "partitions.bin"),
        "0xe000", str(framework_dir / "tools" / "partitions" / "boot_app0.bin"),
        "0x10000", str(build_dir / "firmware.bin"),
    ]
    subprocess.run(command, check=True)
    print(f"Merged flash image: {output_path}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_firmware)
