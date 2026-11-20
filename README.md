# Retro KiddOS

A simple, child-friendly entertainment frontend for the FunnyPlaying Retro Pixel Pocket.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling. Family multiplayer is an important part of the gaming goal.

Our guiding principle:

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

## Project direction

Retro KiddOS is a device-first frontend, not a replacement operating system. It reuses the handheld's existing Linux firmware, SDL libraries, emulator stack, game files, controller, networking, and power-management facilities.

The current priority is a dependable family gaming experience. Keep the interface focused and the implementation as small as practical. Validate real use on hardware before expanding scope.

## Current implementation

The active SDL frontend is in [`device/working-ui/`](device/working-ui/). Its application architecture is documented in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

The frontend currently provides:

- A four-game carousel with artwork and D-pad navigation.
- Game launching through the existing firmware and return to KiddOS after exiting.
- A System Menu with shutdown confirmation.
- Battery percentage, charging status, and low-battery warning.
- Wi-Fi connection status.
- Hidden, PIN-protected Parent Mode.
- Parent Center controls for the existing Remote Management service, Wi-Fi radio, network scanning and selection, secured-network password entry, and Parent PIN changes.

The architecture separates the application coordinator, semantic input, navigation, screen rendering and state, SDL resource ownership, PIN workflow, and device services. This organizes the existing frontend; it does not replace firmware services or introduce a generic media framework.

## Verification status

The refactored frontend was built for ARM64 and deployed to the **blue device (RPPBLUE)** on November 19, 2026.

The following were verified on that device:

- Startup and Games screen rendering.
- D-pad navigation, A/Confirm, and B/Back.
- Start/System Menu navigation and correct return destination.
- Game launch and return to KiddOS.
- Shutdown confirmation.
- Parent Mode and PIN unlock, change, cancellation, and error handling.
- Wi-Fi settings and the on-screen password keyboard.
- Battery and charging indicator.

The deployed executable SHA-256 was:

`1238648b823a2f09342bb646d5aee6f135a8d426647ea2b7bd6c7b5825bb3508`

This validation applies to RPPBLUE and its current firmware/configuration. Purple and green devices have not been approved or validated as deployment targets.

## Build and deployment

See [`device/working-ui/README.md`](device/working-ui/README.md) for prerequisites, the ARM64 build, deployment target, backup, and rollback procedure.

The build uses an external `Build-Workspace` alongside the repository, or the path supplied through `KIDDOS_BUILD_WORKSPACE`. The workspace contains the ARM64 compiler dependencies, SDL sources, and device sysroot; it is not included in this repository.

Deployment is device-specific and defaults to RPPBLUE. Verify the target and local build path before running it. The deployment script backs up the existing executable and verifies the staged checksum. Its automated startup check only verifies that the service remains active; visual and input regression testing on the handheld is still required.

The tested firmware startup hook is in [`firmware/boot/`](firmware/boot/). It is not a general-purpose installer. Do not install it on another device without reviewing its paths and recovery procedure.

## Existing media playback

The handheld's existing MPV player has been confirmed to play a 720×720
H.264/AAC test clip with sound. The test clip filled the square display after
the source was encoded to square pixels. This is a playback experiment, not a
completed Movies or Music interface. Continue to prefer existing firmware
playback tools over building custom media infrastructure.

## Current scope and limitations

- The game catalog remains hardcoded; parent-managed library editing is not implemented.
- Games is the only implemented library section. Music and Movies exist only as prepared section-navigation structure; their browsing and playback workflows are not implemented.
- Physical L/R library switching is not wired because shoulder-button IDs have not been verified.
- Multiplayer workflows, including Pokémon trading and head-to-head play, remain future work.
- Full-length movie playback and a simple music-library workflow have not been validated.
- Runtime paths and the firmware hook are specific to the tested device setup.

Keep future work focused on making it easier for the kids to play and enjoy games together.

## Repository layout

- `device/working-ui/` — active SDL frontend, build/deploy scripts, and device-specific instructions.
- `firmware/boot/` — tested device startup hook.
- `docs/` — architecture, hardware, and game-library references.
- `CONTRIBUTING.md` — development principles.

Earlier committed history, including the pre-refactor baseline, remains available in Git.

## Hardware

- FunnyPlaying Retro Pixel Pocket
- 720 × 720 display
- ARM64 Linux firmware
- Existing RetroArch, emulator, SDL, and MPV infrastructure

See [`docs/HARDWARE.md`](docs/HARDWARE.md) for the inspected device configuration.

## License

GNU General Public License v3.0 or later. See [`LICENSE`](LICENSE).
