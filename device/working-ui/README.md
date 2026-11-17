# Retro KiddOS — Working Device UI

Confirmed running on the FunnyPlaying Retro Pixel Pocket on November 16, 2026.

## Working features

- Four-game carousel with D-pad navigation.
- Launches games through the firmware's runemu.py.
- Stages the patched Gambatte link core.
- Returns to KiddOS after exiting a game.
- System menu, shutdown confirmation, and system shutdown.

Confirmed executable SHA-256:

`f2d4f0dad7be197fc927f43cd26f8b76b1858a18a15dc93183f7906e15713f18`

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

This is the confirmed working November 16 device implementation. The original desktop SDL application has been moved out of the active repository and preserved in the local Archives directory. This standalone frontend is the active development path.
