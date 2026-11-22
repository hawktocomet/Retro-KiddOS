# Retro KiddOS

A simple, child-friendly entertainment frontend for the FunnyPlaying Retro Pixel Pocket.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling. Family multiplayer is an important part of the gaming goal.

Our guiding principle:

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

## Project direction

Retro KiddOS and Retro Pixel Foundation are two independent projects with
separate responsibilities.

- **Retro KiddOS** is the child-friendly gaming and entertainment experience.
  It owns the frontend, content library, child-facing navigation, parent
  management, and product-specific behavior.
- **Retro Pixel Foundation** is the reusable hardware and software platform.
  It owns the operating system, hardware support, system services, gaming
  runtime, reproducible image build, and documented application interfaces.

The Foundation must be independently buildable and useful without KiddOS.
KiddOS must remain usable on the existing stock RetroPixel firmware while
the Foundation is developed.

See [Project Boundaries](docs/PROJECT-BOUNDARIES.md) for the ownership rules,
transition responsibilities, and intended migration sequence.

### Immediate KiddOS priority

The immediate product priority is a reliable, easy-to-use experience on the
existing devices. Preserve the working stock firmware integration and avoid
changes that put the family gaming experience at risk.

Foundation research and development should proceed independently. It must
not become a prerequisite for KiddOS progress or require premature migration
of working components.

### Foundation direction

The longer-term Foundation goal is a transparent, reproducible platform that
another developer can clone, build, and use with a frontend of their choice.
Build inputs, dependencies, source provenance, licenses, and redistribution
restrictions must be documented.

The existing firmware-image research remains valuable technical evidence.
It is not a claim that a complete, reproducible Foundation image has already
been built or validated.

## Current implementation

The active SDL frontend is in [`device/working-ui/`](device/working-ui/). Its application architecture is documented in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

The frontend currently provides:

- A four-slot game carousel populated from the device-side game catalog.
- A Movies browser populated from `/storage/roms/movies`, with filename-based titles and MPV playback.
- L/R shoulder-button library switching between Games and Movies; D-pad navigation and A/B selection/back controls.
- Game launching through the existing firmware and return to KiddOS after exiting.
- Live carousel updates when slots change in Remote Management, without restarting KiddOS.
- Artwork-aware game assignment, browser-side cover-art discovery, preview, and upload, plus manual artwork upload.
- A System Menu with shutdown confirmation.
- Battery percentage, charging status, and low-battery warning.
- Wi-Fi connection status.
- Hidden, PIN-protected Parent Mode.
- Parent Center controls for Remote Management, Wi-Fi radio, network scanning and selection, secured-network password entry, and Parent PIN changes.

The Python catalog module is the shared authority for game discovery, slot assignments, and artwork matching. The SDL frontend reads that catalog locally; it does not maintain a separate hardcoded game list. The Movies browser independently discovers `.mp4`, `.mkv`, `.avi`, `.mov`, and `.m4v` files in `/storage/roms/movies` using Python 3. Movie playback is delegated to the firmware's `/usr/bin/mpv`. The architecture separates the application coordinator, semantic input, navigation, screen rendering and state, SDL resource ownership, PIN workflow, and device services.

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

The Movies browser and MPV launcher are integrated into the SDL frontend.
On RPPBLUE, MPV playback was verified with DRM video output and ALSA stereo
audio. The physical controls were also verified: A pauses/resumes, B exits
playback and returns to Movies, and D-pad left/right seeks by 10 seconds.
Continue to prefer existing firmware playback tools over building custom
media infrastructure.

## Current scope and limitations

- The four-slot carousel currently accepts only raw `.gb` and `.gbc` ROMs. Other discovered systems are not currently slot-assignable.
- Browser-side automatic artwork search supports Game Boy and Game Boy Color; other artwork can be supplied manually.
- Games and Movies are implemented as child-facing library sections. Music browsing and parent-side media preparation workflows are not implemented.
- The Movies browser requires Python 3 on the device; playback requires `/usr/bin/mpv` with DRM video output and ALSA audio support.
- Movie playback controls are configured by `device/working-ui/mpv-input.conf`: A pauses/resumes, B exits to Movies, and D-pad left/right seeks by 10 seconds. Start and L/R are unbound during playback.
- The Movies browser currently scans `/storage/roms/movies` and supports `.mp4`, `.mkv`, `.avi`, `.mov`, and `.m4v` filename extensions; codec compatibility depends on the installed MPV build.
- Multiplayer workflows, including Pokémon trading and head-to-head play, remain future work.
- Full-length movie playback and a simple music-library workflow have not been validated.
- Runtime paths and the firmware hook are specific to the tested device setup.

Prioritize practical improvements to the KiddOS experience while preserving the working stock-firmware path. Defer broad Foundation integration and speculative abstractions until they have a concrete purpose and can be validated independently.

## Repository layout

- `device/working-ui/` — active SDL frontend, build/deploy scripts, and device-specific instructions.
- `firmware/boot/` — tested device startup hook.
- `device/management-server/` — shared game catalog, Remote Management API, browser interface, and tests.
- `docs/` — architecture, hardware, game-library, and management references.
- `docs/CUSTOM-IMAGE-ARCHITECTURE.md` — retained firmware-image research, verified build findings, and historical integration blockers.
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
