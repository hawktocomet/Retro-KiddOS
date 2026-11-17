# Retro KiddOS

A simple, child-friendly entertainment frontend for the FunnyPlaying Retro Pixel Pocket, built around games, music, and movies.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling.

The goal is for each child to be able to:
- Play favorite games.
- Listen to favorite music.
- Watch favorite movies.

The experience should be straightforward, reliable, and usable without Wi-Fi or constant adult help. Family multiplayer is an important part of the gaming goal.

Our guiding principle is:

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

More broadly, every hour should make it easier for the kids to enjoy their own games, music, and movies. Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

## Project direction

Retro KiddOS uses a device-first approach. Rather than replacing the handheld's firmware or rebuilding its emulator and media infrastructure, it reuses the existing system and provides a simpler interface for children.

The current priority is a dependable family experience, not a large custom operating system. Keep the interface focused and the implementation as small as practical. Validate real use on the hardware before expanding the scope.

## Current working implementation

The confirmed SDL frontend is in [`device/working-ui/`](device/working-ui/).

Core frontend verified November 16, 2026; battery monitoring verified November 17, 2026, on the blue Retro Pixel Pocket:

- Boots directly into the KiddOS game carousel.
- Displays four selected games with artwork.
- Navigates with the D-pad.
- Launches games through the existing firmware.
- Supports normal in-game controls.
- Select + Start exits a game and returns to KiddOS.
- Provides a system menu with shutdown confirmation.
- Shuts down the device.
- Displays battery percentage and charging status.
- Shows a low-battery warning at 15% or below.
- Provides hidden, PIN-protected Parent Mode.
- Parent Mode can show and toggle the LAN management server without disconnecting Wi-Fi.
- Parents can change the Parent Mode PIN after verifying the current PIN.

The firmware startup hook is in [`firmware/boot/`](firmware/boot/).

## Media playback exploration

The Pocket's existing MPV player has been confirmed to play video with sound. A 720×720 H.264/AAC test clip played smoothly and filled the square display after encoding the source to square pixels. Full-length movie playback and a simple music playback workflow still need to be validated.

The direction is to use existing firmware tools for local, offline media wherever practical. Do not build a custom video or audio engine unless testing shows the built-in tools cannot provide the needed experience.

## Current game set

- Mario Tennis
- Pokemon Red
- Super Mario Bros. Deluxe
- Tetris

The current game list and runtime paths are configured directly in the prototype. A parent-managed game library is a future goal, not an implemented feature.

## Build

See [`device/working-ui/README.md`](device/working-ui/README.md).

The build script cross-compiles for ARM64 and uses a local `Build-Workspace` directory alongside this repository. That workspace contains the required SDL sources, device sysroot, and libraries; it is not included in this repository.

## Current limitations

This is a working prototype validated on one blue device.

- Runtime paths and game configuration are device-specific.
- The firmware hook is not a general-purpose installer.
- Installation and rollback need a repeatable procedure.
- Other devices have not yet been validated.
- Wi-Fi configuration controls in Parent Mode are not implemented yet.
- The game library is still hardcoded; parent-managed library editing is future work.
- Multiplayer workflows remain future work.
- Full-length movie playback and music-library usability are not yet validated.

Do not assume the current firmware hook is safe to install on another device without reviewing its paths and recovery procedure.

## Repository layout

- `device/working-ui/` — working SDL frontend and build instructions.
- `firmware/boot/` — tested device startup hook.
- `docs/` — current hardware, controls, and game-library references.
- `CONTRIBUTING.md` — project development principles.

The original desktop SDL implementation and its research materials are preserved locally in the project's Archives directory. Earlier committed history remains available in Git.

## Hardware

- FunnyPlaying Retro Pixel Pocket
- 720 × 720 display
- ARM64 Linux firmware
- Existing RetroArch, emulator, and MPV infrastructure

See [`docs/HARDWARE.md`](docs/HARDWARE.md) for verified device details.

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE).
