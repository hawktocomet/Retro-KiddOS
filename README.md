# Retro KiddOS

A simple, child-friendly entertainment frontend for the FunnyPlaying Retro Pixel Pocket.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling. Family multiplayer is an important part of the gaming goal.

Our guiding principle:

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

## Project direction

Retro KiddOS is a device-first frontend, not a replacement operating system. It reuses the handheld's existing Linux firmware, SDL libraries, emulator stack, game files, controller, networking, and power-management facilities.

The current milestone is a reproducible, flashable KiddOS firmware image based on the existing RetroPixel/AmberELEC firmware. Preserve its proven boot, kernel, drivers, emulator stack, and hardware support; integrate the maintained KiddOS software as proper firmware packages and services. The image must be buildable from documented source inputs and installable on a stock Retro Pixel Pocket without manual SSH setup.

Feature development is frozen while this foundation is completed. Work should address image-build, packaging, reproducibility, boot, persistence, and release-validation blockers—not add unrelated UI or infrastructure features.

## Current firmware-image milestone

The target deliverable is one reproducible, flashable image built on the existing RetroPixel/AmberELEC firmware—not a Linux distribution built from scratch and not a manual installer layered onto a previously configured device.

The active firmware integration checkout is `Firmware-NinohFOX/`; `Firmware-FunnyPlaying/` is retained as the manufacturer's source baseline for comparison. The final source revision and build provenance still require audit and documentation.

The maintained frontend, native hardware service, and management server already exist in this repository. The remaining work is to package and integrate them into the firmware, resolve external build inputs, verify boot and service behavior, preserve writable user data, and produce and inspect a complete image. A flashable image has not yet been validated.

## Current implementation

The active SDL frontend is in [`device/working-ui/`](device/working-ui/). Its application architecture is documented in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

The frontend currently provides:

- A four-slot game carousel populated from the device-side game catalog.
- Game launching through the existing firmware and return to KiddOS after exiting.
- Live carousel updates when slots change in Remote Management, without restarting KiddOS.
- Artwork-aware game assignment, browser-side cover-art discovery, preview, and upload, plus manual artwork upload.
- A System Menu with shutdown confirmation.
- Battery percentage, charging status, and low-battery warning.
- Wi-Fi connection status.
- Hidden, PIN-protected Parent Mode.
- Parent Center controls for Remote Management, Wi-Fi radio, network scanning and selection, secured-network password entry, and Parent PIN changes.

The Python catalog module is the shared authority for game discovery, slot assignments, and artwork matching. The SDL frontend reads that catalog locally; it does not maintain a separate hardcoded game list. The architecture separates the application coordinator, semantic input, navigation, screen rendering and state, SDL resource ownership, PIN workflow, and device services.

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

The November 19 architecture-refactor executable had SHA-256:

`1238648b823a2f09342bb646d5aee6f135a8d426647ea2b7bd6c7b5825bb3508`

The subsequent live-catalog frontend build had SHA-256:

`209edf4c07e2f16227d41c5969650182b410f9ef26eb8a3bf05bc9d6353ab10c`

That live-catalog build was deployed to RPPBLUE and verified on-device. The later artwork workflow update changed the management web page, not the frontend executable.

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

- The four-slot carousel currently accepts only raw `.gb` and `.gbc` ROMs. Other discovered systems are not currently slot-assignable.
- Browser-side automatic artwork search supports Game Boy and Game Boy Color; other artwork can be supplied manually.
- Games is the only implemented library section. Music and Movies exist only as prepared section-navigation structure; their browsing and playback workflows are not implemented.
- Physical L/R library switching is not wired because shoulder-button IDs have not been verified.
- Multiplayer workflows, including Pokémon trading and head-to-head play, remain future work.
- Full-length movie playback and a simple music-library workflow have not been validated.
- Runtime paths and the firmware hook are specific to the tested device setup.

Until the reproducible image is built and validated, keep work limited to firmware integration and release blockers. Resume feature development only when the image's acceptance criteria or a real hardware problem requires it.

## Repository layout

- `device/working-ui/` — active SDL frontend, build/deploy scripts, and device-specific instructions.
- `firmware/boot/` — tested device startup hook.
- `device/management-server/` — shared game catalog, Remote Management API, browser interface, and tests.
- `docs/` — architecture, hardware, game-library, and management references.
- `docs/CUSTOM-IMAGE-ARCHITECTURE.md` — current firmware-image integration plan, verified build findings, and blockers.
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
