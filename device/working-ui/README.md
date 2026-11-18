# Retro KiddOS — Working Device UI

Confirmed running on the FunnyPlaying Retro Pixel Pocket on November 16, 2026.

## Working features

- Four-game carousel with D-pad navigation.
- Launches games through the firmware's runemu.py.
- Stages the patched Gambatte link core.
- Returns to KiddOS after exiting a game.
- System menu, shutdown confirmation, and system shutdown.
- Battery percentage display.
- Charging indicator when AC or USB external power is connected.
- Low-battery warning styling at 15% or below.
- Hidden, PIN-protected Parent Mode.
- Remote Management server status and local start/stop control.
- Wi-Fi connection indicator beside the battery indicator.
- Parent Center with Remote Management, Wi-Fi Settings, and Change Parent PIN.
- Wi-Fi Settings reports radio power and connection state, and can turn Wi-Fi on/off through ConnMan.
- Change PIN flow requiring the current PIN.

Battery monitoring reads the device's RK817 power-supply sysfs interfaces:

- Battery capacity: `/sys/devices/platform/ff180000.i2c/i2c-0/0-0020/rk817-battery/power_supply/battery/capacity`
- External power: the `online` files under the RK817 charger's `ac` and `usb` power-supply directories.

The values are polled every five seconds. The charging indicator appears when
either AC or USB reports online. Battery percentage is displayed whenever a
valid capacity reading is available.

The battery indicator was tested on the blue Retro Pixel Pocket on November
17, 2026, including connecting and disconnecting external power. Its current
size and visual prominence are intentionally left for the next UI redesign.

Confirmed installed executable SHA-256:

`d0942a6e2ebf0bfe336cdafde64aa839c71eb67de87a5bac05bdf0372068a53d`

## Runtime requirements

The current application expects these files under:

`/storage/roms/.kiddos-ui-test-20261116/`

- Mario Tennis.gbc
- Pokemon Red Version.gb
- Super Mario Bros. Deluxe.gbc
- Tetris.gb
- Matching PNG artwork
- gambatte-link_libretro.so

It also requires the firmware's runemu.py, systemctl, SDL2 libraries, and Cabin Bold font at:

`/storage/.config/emulationstation/themes/rp-pocket/art/fonts/Cabin-Bold.ttf`

## Building

Run `./build-kiddos-ui.sh` from this directory.

The script expects the local Build-Workspace directory alongside the KiddOS repository, with the ARM64 SDL2 dependencies and device sysroot. It requires aarch64-linux-gnu-g++ and readelf.

The output is written to `Build-Workspace/build/arm64/kiddos-ui`. Existing output is backed up before replacement.

## Firmware integration

The tested boot hook is at `../../firmware/boot/custom_start.sh`.

It starts KiddOS as a systemd service and masks EmulationStation. It currently uses the dated runtime directory above and should not be installed on another device without reviewing its paths and rollback procedure.

## Status

This is the confirmed working November 18 device implementation on the blue Retro Pixel Pocket. Parent Mode, Remote Management, Change PIN, Wi-Fi power controls, and the Wi-Fi indicator have been deployed and verified. Wi-Fi power control uses ConnMan and is tested locally on the handheld. Remote Management remains independent of Wi-Fi power and was re-tested after Wi-Fi was turned off and restored.

Wi-Fi network scanning, joining networks, and credential entry are not implemented. The original desktop SDL application has been moved out of the active repository and preserved in the local Archives directory. This standalone frontend is the active development path.

Deployed ARM64 executable SHA-256:

`2531fa4d3d1376943fec0fbe4185f28450948dee060db1a93a0aef0fbceeb882`
