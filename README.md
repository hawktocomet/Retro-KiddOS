# Retro KiddOS

A simple, child-friendly gaming frontend for the FunnyPlaying Retro Pixel Pocket.

## Project direction

Retro KiddOS uses a device-first approach. Rather than replacing the
handheld's firmware or rebuilding its emulator infrastructure, it reuses
the existing system and provides a simpler interface for children.

The guiding principle is:

> Every hour we spend developing should make it easier for you and your
> kids to play games together. If it doesn't, we probably shouldn't be
> building it.

## Current working implementation

The confirmed SDL frontend is in [`device/working-ui/`](device/working-ui/).

Verified on the blue Retro Pixel Pocket on November 16, 2026:

- Boots directly into the KiddOS game carousel.
- Displays four selected games with artwork.
- Navigates with the D-pad.
- Launches games through the existing firmware.
- Supports normal in-game controls.
- Select + Start exits a game and returns to KiddOS.
- Provides a system menu with shutdown confirmation.
- Shuts down the device.

The firmware startup hook is in [`firmware/boot/`](firmware/boot/).

## Current game set

- Mario Tennis
- Pokemon Red
- Super Mario Bros. Deluxe
- Tetris

The current game list and runtime paths are configured directly in the
prototype. A parent-managed game library is a future goal, not an
implemented feature.

## Build

See [`device/working-ui/README.md`](device/working-ui/README.md).

The build script cross-compiles for ARM64 and uses a local
`Build-Workspace` directory alongside this repository. That workspace
contains the required SDL sources, device sysroot, and libraries; it is
not included in this repository.

## Current limitations

This is a working prototype validated on one blue device.

- Runtime paths and game configuration are device-specific.
- The firmware hook is not a general-purpose installer.
- Installation and rollback need a repeatable procedure.
- Other devices have not yet been validated.
- Parent controls and an editable game library are not implemented.
- Multiplayer workflows remain future work.

Do not assume the current firmware hook is safe to install on another
device without reviewing its paths and recovery procedure.

## Repository layout

- `device/working-ui/` — working SDL frontend and build instructions.
- `firmware/boot/` — tested device startup hook.
- `docs/` — current hardware, controls, and game-library references.
- `CONTRIBUTING.md` — project development principles.

The original desktop SDL implementation and its research materials
are preserved locally in the project's Archives directory. Earlier
committed history remains available in Git.

## Hardware

- FunnyPlaying Retro Pixel Pocket
- 720 × 720 display
- ARM64 Linux firmware
- Existing RetroArch and emulator infrastructure

See [`docs/HARDWARE.md`](docs/HARDWARE.md) for verified device details.

## License

Not yet selected.
