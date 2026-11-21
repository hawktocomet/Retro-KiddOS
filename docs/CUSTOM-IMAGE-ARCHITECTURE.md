# Retro KiddOS Custom Image Architecture

**Status:** Architecture baseline and implementation plan
**Last updated:** November 20, 2026
**Purpose:** Document the components, dependencies, build sequence, and remaining work required to produce a reproducible, bootable Retro KiddOS image for the FunnyPlaying Retro Pixel Pocket.

---

## 1. Goal and guiding principle

The goal is to produce our own bootable Retro Pixel Pocket image that starts directly into a reliable, child-friendly Retro KiddOS experience.

The project should reuse the existing AmberELEC-derived firmware, Linux kernel, bootloader, drivers, emulator cores, and system libraries wherever practical. We are not trying to replace the underlying operating system.

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

The image must eventually provide:

- A reliable boot process into KiddOS.
- The maintained SDL2 frontend.
- The native hardware service.
- The local Remote Management server.
- The existing emulator and game-launch infrastructure.
- Persistent game libraries, artwork, and management settings.
- A safe way to update and recover the device.

The result should be reproducible from source rather than dependent on manually copying files onto a previously configured handheld.

## 2. Repositories and their responsibilities

There are four primary workspaces.

| Workspace | Responsibility | Current role |
|---|---|---|
| `Firmware-NinohFOX/` | Operating system, kernel, packages, root filesystem, bootable image | Active integration checkout; changes must be preserved |
| `Firmware-FunnyPlaying/` | Manufacturer firmware source | Clean manufacturer baseline for comparison; not the active integration checkout |
| `Build-Workspace/` | ARM64 dependencies, SDL libraries, cross-compilation support, experiments | Supporting build environment |
| `KiddOS/` | Frontend, hardware service, management server, deployment scripts, documentation | Maintained product source |

### 2.1 Firmware workspaces

The active integration checkout is:

`~/Projects/Retro-Pixel-Pocket/Firmware-NinohFOX`

The manufacturer source baseline is:

`~/Projects/Retro-Pixel-Pocket/Firmware-FunnyPlaying`

The NinohFOX checkout is the working build-system integration target. It contains local KiddOS integration changes and other uncommitted work; preserve that state. The manufacturer checkout is retained for comparison and provenance checks. Do not assume the two checkouts have identical history or that either is the final release source revision.

The active integration checkout is the operating-system and image build system.

It is an AmberELEC/LibreELEC-style build environment containing:

- Host and target toolchain setup.
- Package recipes and dependency resolution.
- Linux kernel build configuration.
- Device-specific Rockchip/RPPOCKET configuration.
- Root filesystem assembly.
- Kernel and system image generation.
- Bootloader configuration.
- Disk image and release packaging.

Important paths:

- `projects/Rockchip/devices/RPPOCKET/` — device-specific configuration and packages.
- `packages/` — shared system packages.
- `packages/amberelec/` — AmberELEC integration and startup behavior.
- `scripts/build_distro` — distribution build entry point.
- `scripts/image` — kernel, system, and release image preparation.
- `scripts/mkimage` — partition and bootable disk image assembly.
- `build.RetroPixel-RPPOCKET.aarch64/` — expected ARM64 build output for the actual RPPOCKET target. It is currently absent.
- `build.AmberELEC-RPPOCKET.aarch64/` — an existing directory, but it is not the expected output path for the RetroPixel distribution selected by `scripts/build_distro`.

The `Firmware-NinohFOX/` checkout contains uncommitted changes, new package files, tests, and backup files. Preserve them. Do not clean, reset, or overwrite this workspace as part of ordinary KiddOS development. Use `Firmware-FunnyPlaying/` as a read-only comparison baseline unless a specific, reviewed task requires otherwise.

### 2.2 Build-Workspace

Location: `~/Projects/Retro-Pixel-Pocket/Build-Workspace`

This workspace contains ARM64 build dependencies and experiments, including SDL-related build artifacts, sysroot libraries, and cross-compilation support.

The maintained frontend build currently depends on:

- `aarch64-linux-gnu-g++`
- ARM64 SDL2 headers and libraries.
- SDL2_image.
- SDL2_ttf.
- ARM64 target sysroot.
- The appropriate SDL configuration headers.

The current maintained frontend build script is in the KiddOS repository, not this workspace.

Some scripts in Build-Workspace refer to older paths and the original single-file prototype. Treat those as historical until they are reconciled.

**Long-term objective:** make the application build consume pinned, documented dependencies that can be recreated automatically. A developer should not need an undocumented pre-existing sysroot to produce the application binary.

### 2.3 KiddOS

Location: `~/Projects/Retro-Pixel-Pocket/KiddOS`

This is the maintained product repository.

Important directories:

| Path | Responsibility |
|---|---|
| `device/working-ui/` | SDL2/C++ child-facing frontend |
| `device/hardware-service/` | Native hardware input and control service |
| `device/management-server/` | Python HTTP server, catalog, and browser interface |
| `firmware/boot/` | Device boot-hook deployment artifacts |
| `docs/` | Architecture, hardware, game library, and migration documentation |

The KiddOS repository should become the source of truth for all custom software included in the final image.

---

## 3. Hardware and base operating system

### 3.1 Target hardware

The target is the FunnyPlaying Retro Pixel Pocket.

Known device characteristics:

- Rockchip RK3326.
- AArch64 CPU architecture.
- Cortex-A35 CPU target.
- 720 × 720 display.
- Approximately 926 MiB reported RAM.
- Linux kernel 4.4.189 on the tested device.
- Existing RetroArch and emulator infrastructure.

The tested device reports RetroPixel 20251127 and an AmberELEC-derived environment.

### 3.2 Boot chain

The expected boot chain is:

1. Device boot ROM and board bootloader.
2. U-Boot.
3. Linux kernel and device tree.
4. AmberELEC-derived root filesystem.
5. systemd and firmware startup services.
6. KiddOS startup integration.
7. KiddOS frontend and supporting services.

The verified build entry point is `make RPPOCKET`, which invokes `scripts/build_distro`. That script selects `DISTRO=RetroPixel`, `PROJECT=Rockchip`, and the requested architecture. For the ARM64 target it proceeds through the image build.

The RPPOCKET device configuration selects U-Boot and `UBOOT_SYSTEM=rg351mp`. The U-Boot package currently reuses the RG351MP `odroidgoa_defconfig` configuration while installing an RPPOCKET-specific U-Boot device tree. This reuse is verified in the build scripts; its correctness should still be validated against the actual hardware and source package.

The Rockchip-specific `projects/Rockchip/bootloader/mkimage` overrides the generic image assembly path. It writes the Rockchip bootloader payload, copies `boot.ini`, stages device trees, generates per-DTB extlinux configurations, and copies those files into the FAT boot partition. The device boot script selects the RPPOCKET v6.0 Linux DTB configuration except for the separately handled `d007` hardware revision.

### 3.3 Kernel and device tree

The RPPOCKET configuration selects:

- ARM64 kernel architecture.
- Cortex-A35 target CPU.
- `Image` kernel output.
- `rockchip/rk3326-rppocket_v6_0-linux.dtb` device tree.
- Mali G31 graphics configuration.

The kernel and device tree are fundamental image components. A working application binary cannot compensate for an incorrect kernel, device tree, display driver, input driver, or storage configuration.

The tested device's kernel version is known, but the exact relationship between that running kernel and the current Firmware source revision must be recorded for reproducible builds.

---

## 4. Firmware image assembly

The `Firmware-NinohFOX/` checkout separates operating-system construction from final disk-image assembly.

### 4.1 Build stages

The high-level sequence is:

1. Load build configuration.
2. Initialize host and target toolchains.
3. Resolve and build required packages.
4. Build the Linux kernel.
5. Assemble the target root filesystem.
6. Produce the compressed system filesystem image.
7. Produce the kernel image.
8. Prepare release files and bootloader configuration.
9. Create the partitioned disk image.
10. Copy boot and system files into the image.
11. Produce checksums and release artifacts.

The main scripts are:

- `scripts/build_distro`
- `scripts/image`
- `scripts/mkimage`

The `scripts/image` process produces separate kernel and system artifacts before the final disk image is assembled.

### 4.2 Partitions

The inspected RPPOCKET image configuration resolves to:

- `KERNEL_NAME=KERNEL`.
- `SYSTEM_SIZE=2048` MiB.
- `SYSTEM_PART_START=8192` sectors.
- A FAT32 boot/system partition.
- An ext4 storage partition, with 32 MiB used as the minimum storage size in the generic image-layout logic.

The Rockchip image path uses an MBR (`msdos`) partition table for this U-Boot target. The system partition begins at sector 8192; the image-layout code derives the initial disk padding from that offset. The storage partition is intended for persistent user data, including ROMs and configuration.

These are source-inspected settings, not yet measurements from a successfully generated image. Confirm the final partition table and usable storage size by inspecting the first built image before flashing it.

The production image must preserve the separation between immutable operating-system files and writable user data.

### 4.3 Image reproducibility

A reproducible image requires recording:

- Firmware Git commit.
- KiddOS Git commit.
- Build configuration and target device.
- Kernel version and configuration.
- Device tree.
- Bootloader version and configuration.
- Package versions and dependency closure.
- Cross-compiler and sysroot versions.
- Application binary hashes.
- Image hash and build date.

A successful build alone is not sufficient. We must be able to identify exactly which source revisions and dependencies produced it.

---

## 5. Firmware packages and legacy components

### 5.1 Device-specific package selection

RPPOCKET has device-specific package recipes under:

`Firmware-NinohFOX/projects/Rockchip/devices/RPPOCKET/packages/`

The device options currently include:

`ADDITIONAL_PACKAGES="dtc amberelec kiddos-ui"`

This selects a `kiddos-ui` package that currently contains an early test implementation.

### 5.2 Important frontend integration gap

The `Firmware-NinohFOX/` package's current `kiddos-ui` source is not the maintained frontend.

It contains hardcoded test paths and a small static game list. It does not represent the current shared catalog, live slot updates, Parent Mode, Remote Management, or current Wi-Fi workflow.

The maintained frontend is:

`KiddOS/device/working-ui/`

The `Firmware-NinohFOX/` package must eventually be replaced or updated to package the maintained frontend and its runtime dependencies.

Do not ship the prototype as the final KiddOS interface.

### 5.3 Legacy RetroPixel daemon

The shared AmberELEC package in `Firmware-NinohFOX/` still copies the legacy `retropixel-server` executable into the root filesystem.

The shared startup script also contains a launch of that daemon.

The current uncommitted startup changes add a KiddOS boot selection, but the inspected diff does not remove the legacy daemon launch.

The new hardware service replaces the old daemon's relevant hardware responsibilities on the tested device. A clean image must not unintentionally start both services.

Before release:

- Decide whether the legacy binary should remain installed for recovery or be omitted.
- Ensure the old daemon is not launched during normal KiddOS operation.
- Confirm that no other firmware service assumes it is running.
- Test the final service ownership and hardware behavior.

### 5.4 Sleep package

RPPOCKET contains an `enable-oga-sleep` package.

Its package recipe installs systemd sleep configuration and a sleep hook.

RPPOCKET's `odroidgoa-utils` package depends on `enable-oga-sleep`, and AmberELEC lists `odroidgoa-utils` among its tools. This is evidence that the sleep package may be included indirectly.

The resolved build package manifest must confirm whether it is actually installed.

This matters because suspend behavior previously interfered with device use and remote access. The new hardware service deliberately does not implement automatic suspend.

Do not remove sleep support blindly. First establish which package installs it, which service or policy triggers suspend, and whether the final image needs suspend at all.

---

## 6. KiddOS frontend

Source:

`KiddOS/device/working-ui/`

The frontend is a native C++17 SDL2 application.

### 6.1 Responsibilities

The frontend provides:

- Child-facing game carousel.
- Selected Game screen.
- Physical controller navigation.
- Game launching and return to KiddOS.
- Battery display.
- Parent Mode and PIN management.
- Remote Management controls.
- Wi-Fi settings and connection workflow.
- Shutdown and power actions.
- Shared catalog integration.

The UI is designed for the 720 × 720 display and the device's physical controls.

### 6.2 Build process

The maintained build script is:

`device/working-ui/build-kiddos-ui.sh`

It:

1. Locates the KiddOS source tree.
2. Locates Build-Workspace dependencies.
3. Checks SDL2 headers and target libraries.
4. Checks the AArch64 cross-compiler.
5. Compiles the frontend for Cortex-A35.
6. Verifies the output architecture and dynamic dependencies.
7. Backs up the previous build.
8. Installs the new build artifact into the ARM64 build directory.

The resulting executable is currently:

`Build-Workspace/build/arm64/kiddos-ui`

The final firmware package should build this executable from maintained source, or consume a verified artifact produced by a reproducible build stage.

### 6.3 Runtime dependencies

The frontend requires:

- SDL2.
- SDL2_image.
- SDL2_ttf.
- Cabin Bold font at the expected device path.
- Firmware game-launch infrastructure.
- RetroArch and compatible emulator cores.
- The shared game catalog.
- The expected device filesystem layout.

The font and all required shared libraries must be included in the image or installed through explicit package dependencies.

### 6.4 Current validation

The maintained frontend has been deployed and tested on the blue device.

It is not yet validated as a clean-image installation across all three devices.

---

## 7. Native hardware service

Source:

`KiddOS/device/hardware-service/`

The hardware service is a native ARM64 C++ process.

It handles device-level responsibilities previously associated with the legacy server, including:

- Physical brightness controls.
- Physical volume controls.
- Joystick input handling.

It is intentionally separate from the SDL frontend.

### 7.1 Deployment state

The service has been deployed and reboot-verified on the blue device.

It currently resides in writable storage and is launched through the device's custom startup integration.

The read-only system partition was not modified during that migration.

### 7.2 Image integration required

The service is not yet a proper Firmware package.

It needs a device-specific package that:

- Compiles the maintained source for ARM64.
- Installs the binary into a stable system path.
- Installs a systemd service unit.
- Declares its runtime dependencies.
- Enables or starts the service through the intended systemd target.
- Supports clean upgrades and rollback.

The package must not silently reintroduce legacy suspend or power-management behavior.

---

## 8. Remote Management server

Source:

`KiddOS/device/management-server/`

The management server is a Python HTTP application, currently version 0.3.0, using port 8081.

It is separate from the SDL frontend.

### 8.1 Components

- `server.py` — HTTP server and API.
- `catalog.py` — ROM discovery, artwork association, stable IDs, and slot validation.
- `web/index.html` — browser management interface.
- `control.sh` — service control helper.
- `tests/` — catalog and API tests.

### 8.2 Persistent data

| Purpose | Device path |
|---|---|
| ROM library | `/storage/roms` |
| Management state | `/storage/.kiddos-management` |
| Slot assignments | `/storage/.kiddos-management/slots.json` |
| Shared catalog module | `/storage/.kiddos-management/catalog.py` |

The slot file is persistent user data. Routine software updates must not delete or overwrite it.

### 8.3 Game and artwork rules

The catalog can discover multiple supported systems, but the current four active game slots accept only raw `.gb` and `.gbc` ROMs.

Every assigned game must have artwork.

Artwork may be obtained through the browser's Libretro thumbnail search workflow or uploaded by a parent.

The server validates uploads and uses temporary files and atomic replacement for artwork and slot data.

### 8.4 Live updates

The frontend polls the shared slot file every second.

When a parent changes a slot through Remote Management, the running carousel should reflect the change without a reboot, manual refresh, or frontend restart.

### 8.5 Image integration required

The management server needs a proper package or installation stage that:

- Installs the Python source and browser assets.
- Installs a systemd service.
- Creates or preserves writable state directories.
- Starts independently of the SDL frontend.
- Provides controlled access to the local network.
- Supports service health checks and restart behavior.

The ROM library and management state must remain on writable persistent storage.

---

## 9. Boot integration

The firmware startup script invokes:

`/storage/.config/custom_start.sh before`

It also invokes the hook with `after` later in the startup sequence.

This is the supported integration point identified in the inspected AmberELEC startup script.

### 9.1 Current repository hook

`KiddOS/firmware/boot/custom_start.sh` currently contains an older test implementation.

It points to:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

It masks EmulationStation for the current boot and launches the test executable through a transient systemd service.

This file does not yet represent the complete current device deployment.

### 9.2 Current device deployment

The newer hardware-service migration notes describe a different startup arrangement using:

- `/storage/.config/custom_start.sh`
- `/storage/.config/kiddos/retropixel-server`
- A transient systemd unit named `kiddos-hardware.service`
- The maintained KiddOS frontend deployment

The repository hook and the current device deployment need to be reconciled.

### 9.3 Preferred final boot architecture

The clean image should use explicit systemd services rather than relying on an increasingly complicated shell hook.

The intended service responsibilities are:

- Hardware service: starts early and independently.
- Management server: starts independently and remains available if the UI exits.
- KiddOS frontend: starts as the primary child-facing interface.
- EmulationStation: remains available as a recovery/fallback path, not as the normal child-facing interface.

The exact service ordering, dependencies, restart policies, and fallback behavior must be tested before release.

---

## 10. Emulator and game-launch infrastructure

KiddOS should reuse the firmware's existing emulator infrastructure.

The current frontend uses the firmware's `runemu.py` launcher and installed RetroArch cores.

Known relevant cores include:

- Gambatte / SameBoy for GB and GBC.
- mGBA / gpSP for GBA.
- MAME 2003 Plus / FinalBurn Neo for arcade.
- Nestopia / FCEUmm for NES.
- Genesis Plus GX / PicoDrive for Genesis.
- Gearsystem for Master System.
- Snes9x for SNES.

The current four active slots are restricted to GB and GBC.

The patched Gambatte link core is required for the intended link-play workflow. Its build, installation, and version must be part of the reproducible image plan.

Do not replace the firmware's emulator infrastructure without a concrete functional reason.

---

## 11. Persistent storage and upgrades

The final image must distinguish immutable system software from writable user data.

Persistent data includes:

- ROM library.
- Game artwork.
- Slot assignments.
- Parent PIN data.
- Wi-Fi configuration managed by the firmware.
- Management server state.
- Logs needed for troubleshooting.

Routine application and firmware updates must not erase these items.

The image and update process should have a clear policy for:

- First boot initialization.
- Storage partition creation and expansion.
- Existing user data migration.
- Application upgrades.
- Configuration schema changes.
- Recovery after failed updates.
- Factory reset behavior.

The management slot file and ROM library must never be treated as disposable build artifacts.

---

## 12. Current implementation status

| Component | Tested or present | Clean-image status |
|---|---|---|
| RPPOCKET firmware build system | Present in `Firmware-NinohFOX/` | Existing image builder; full image not yet validated |
| Kernel and device tree | Existing device configuration | Exact source revision needs recording |
| U-Boot image assembly | Present in build scripts | Final settings need recording |
| SDL2 KiddOS frontend | Tested on blue device | Current Firmware package is only a prototype |
| Hardware service | Deployed and reboot-verified on blue | Needs Firmware package |
| Management server | Implemented and tested | Needs Firmware service/package integration |
| Game catalog and artwork workflow | Implemented | Needs packaging and persistent-data setup |
| Live slot updates | Implemented and tested | Needs clean-image validation |
| Legacy daemon | Still copied by Firmware package | Startup must be reconciled |
| Sleep support | Package dependency path exists | Actual installed package must be confirmed |
| Boot hook | Older test version in KiddOS repo | Must be reconciled with deployed hook |
| Emulator cores | Supplied by firmware | Required versions and patched core need pinning |
| Three-device validation | Not complete | Required before release |

---

## 13. Build integration roadmap

The work should proceed in dependency order.

### Phase 1 — Freeze the architecture baseline

- Preserve current Firmware working-tree changes.
- Identify the exact firmware source revision.
- Record resolved RPPOCKET build configuration.
- Identify the actual installed package dependency closure.
- Reconcile deployed and repository startup hooks.
- Record required application and emulator dependencies.

### Phase 2 — Package custom software

Create proper RPPOCKET Firmware packages for:

1. Maintained KiddOS frontend.
2. Native hardware service.
3. Remote Management server.

Each package must declare dependencies, install files into stable paths, and define appropriate systemd integration.

Remove or disable the prototype UI package once the maintained frontend replaces it.

### Phase 3 — Integrate boot behavior

- Define the final systemd services.
- Ensure the hardware service starts reliably.
- Ensure Remote Management is independent of the UI.
- Start the maintained frontend as the normal interface.
- Prevent the legacy daemon from running alongside the replacement service.
- Preserve a recovery path to EmulationStation or another known-good environment.

### Phase 4 — Make builds reproducible

- Pin compiler and dependency versions.
- Document SDL2, SDL2_image, and SDL2_ttf build inputs.
- Document the patched Gambatte link core.
- Remove dependencies on stale absolute paths.
- Ensure the image builder can build all required custom software from maintained source.

### Phase 5 — Build and inspect the first custom image

Verify:

- Correct U-Boot and device tree.
- Correct kernel and root filesystem.
- All required packages installed.
- Correct systemd service files and enablement.
- Correct font and SDL runtime libraries.
- Correct emulator cores and launcher.
- No unintended legacy daemon startup.
- Writable storage and persistent configuration behavior.

### Phase 6 — Physical validation

Test on the blue, purple, and green devices.

At minimum:

- Cold boot.
- Normal restart.
- Frontend crash and restart.
- Hardware buttons and brightness.
- Volume controls.
- Wi-Fi connection and reconnection.
- Remote Management availability.
- ROM upload and artwork upload.
- Slot assignment and live carousel refresh.
- Game launch and return.
- Pokémon link-play workflow.
- Shutdown and recovery behavior.
- Persistence across reboot and software update.

### Phase 7 — Release and recovery

Produce:

- Versioned image.
- SHA-256 checksum.
- Build manifest.
- Installation instructions.
- Recovery instructions.
- Known limitations.
- Release notes.

Keep a known-good image available until the new image has passed physical validation.

---

## 14. Open questions and evidence still needed

The following items remain unresolved. Source-code presence alone is not proof that the final image contains or correctly runs a component.

### 14.1 Build and bootloader

1. Which exact U-Boot binary, source revision, and configuration are included in a successful image?
2. Does the RG351MP `odroidgoa_defconfig` reuse produce a bootloader that is correct for all target RPPOCKET hardware revisions?
3. Does the resolved package manifest include `enable-oga-sleep` through `odroidgoa-utils`?
4. What exact kernel, device-tree, and package revisions are present in the generated image?

The build scripts currently identify the selected distribution, device, architecture, partition start, system size, and U-Boot system label. The actual generated artifacts have not yet been inspected.

### 14.2 KiddOS packaging and runtime

5. Which exact packages and shared libraries are required by the maintained frontend?
6. What is the final installation path for the frontend executable and its font/assets?
7. What are the final systemd service definitions, dependencies, restart policies, and startup ordering for the hardware service and management server?
8. What is the final policy for retaining, disabling, or removing the legacy daemon?
9. How will the patched Gambatte link core be built, versioned, and installed reproducibly?

### 14.3 Storage, updates, and recovery

10. How will first boot initialize and expand persistent storage?
11. How will software and firmware updates preserve ROMs, artwork, slot assignments, PIN data, and network configuration?
12. What is the tested recovery procedure if KiddOS or one of its services fails to start?

These questions must be answered through resolved build manifests, generated-image inspection, and physical-device tests—not assumptions.

---

## 15. Definition of done

The custom image project is complete when a clean build from documented source revisions produces an image that:

- Boots on all three Retro Pixel Pocket devices.
- Starts directly into the maintained KiddOS frontend.
- Runs the hardware service without the legacy daemon conflict.
- Provides independent Remote Management.
- Preserves ROMs, artwork, slot assignments, and parent settings.
- Launches games through the existing emulator infrastructure.
- Supports the intended multiplayer workflow.
- Can be updated and recovered without losing user data.
- Can be rebuilt by following the documented process.

The final measure is not how much of the firmware we replaced. It is whether the family can turn on the devices and play together reliably with minimal adult intervention.


---

## 16. Verified Firmware build findings and current blockers

This section records findings from direct inspection of the `Firmware-NinohFOX/` checkout. It supersedes earlier statements in this document where the build path or boot-image details were still described as unknown.

### 16.1 Actual distribution and build path

**Verified:**

- `scripts/build_distro` explicitly selects `DISTRO=RetroPixel` and `PROJECT=Rockchip`.
- `distributions/RetroPixel/` is a real distribution directory, with `DISTRONAME="RetroPixel"` and the `20251127` distribution version.
- The RPPOCKET ARM64 build path is `build.RetroPixel-RPPOCKET.aarch64/`.
- The existing `build.AmberELEC-RPPOCKET.aarch64/` directory is not the expected output directory for this selected distribution.
- `scripts/image` deletes and recreates its generated install tree and clears its install stamps before assembling the image root filesystem.

**Current state:** the expected RetroPixel build directory is absent. No complete image build has been run as part of this investigation. Do not run a build merely to test the path until the missing source inputs and release blockers below have been addressed.

### 16.2 External source package provisioning

**Blocker — required local sources are missing.**

The `Firmware-NinohFOX/` checkout's own build instructions require a separately downloaded `extpackage` archive to be copied into the repository. This is a manual provisioning step documented by the repository; the sources are not populated through Git submodules.

The current checkout does not contain the required `extpackage/` directory. Inspected recipes reference local sources including:

- `extpackage/u-boot-rppocket`
- `extpackage/linux-kernel-rppocket`
- `extpackage/rkbin-0bb1c512492386a72a3a0b5a0e18e49c636577b9`
- `extpackage/AmberELEC-Server-C`
- `extpackage/Pic`
- `extpackage/AmberELEC-Config`
- `extpackage/AmberELEC-themes`

This means the current checkout is not yet provisioned for a complete build. Obtain the source archive using the repository's documented process, inspect its contents and provenance, and preserve the existing `Firmware-NinohFOX/` working tree when adding it.

### 16.3 Rockchip release logo dependency

**Release risk — hard-coded source path is absent.**

`projects/Rockchip/bootloader/release` contains a copy operation referencing:

`/home/li/图片/logo-boot.bmp`

That file is absent on this machine. The inspected search found this as the only reference to that path.

The release script conditionally enters the logo/battery staging block when the installed bootloader logo exists, but then attempts to copy the hard-coded source image. The copy operation therefore cannot succeed as written when that condition is met. Inspect the script's error handling and correct the dependency before relying on a release build. Prefer a repository-owned asset or a deliberate optional-asset check rather than a developer-specific absolute path.

The Rockchip `mkimage` stage subsequently attempts to copy staged logo and battery assets into the FAT partition. Ensure the release and image stages agree on whether those assets are mandatory or optional.

### 16.4 Custom software integration conflicts

The current `Firmware-NinohFOX/` integration is not yet aligned with the maintained KiddOS implementation:

1. The `Firmware-NinohFOX/` `kiddos-ui` package contains an early static prototype, not `KiddOS/device/working-ui/`. Its hardcoded test paths and game list make it unsuitable for the production image.
2. The `amberelec` package copies the legacy `retropixel-server` from `extpackage/AmberELEC-Server-C`.
3. The inspected startup script still launches that legacy daemon. The current local startup changes add KiddOS boot selection but do not remove that launch.
4. The maintained native hardware service and management server in the KiddOS repository still need proper Firmware package/service integration.

Before producing a production image, package the maintained frontend, hardware service, and management server from their maintained sources. Explicitly decide whether the legacy daemon is omitted or retained but disabled, and ensure it cannot run alongside the replacement hardware service during normal KiddOS operation.

### 16.5 Sleep behavior

The package graph contains a possible indirect path to `enable-oga-sleep` through `odroidgoa-utils`. Its presence in source recipes does not, by itself, prove that it is installed in the final root filesystem.

Confirm the resolved package manifest and identify the actual suspend trigger before changing sleep packages or policies. The previous device suspend issue makes this a required validation item, but not grounds for blindly removing unrelated firmware functionality.

### 16.6 Firmware working-tree protection

The `Firmware-NinohFOX/` checkout contains pre-existing modified files, new package files, test files, and backups. Treat these as user work.

Do not use `git clean`, `git reset`, broad checkout/restore operations, or `distclean` as routine investigation steps. In particular, the build instructions' suggested clean operations must not be run without first understanding their effects on this existing workspace.

### 16.7 Readiness summary

| Item | Finding | Status |
|---|---|---|
| Distribution selection | RetroPixel / Rockchip / RPPOCKET | Verified |
| Expected build directory | `build.RetroPixel-RPPOCKET.aarch64/` | Absent |
| Existing AmberELEC-named build directory | Not the selected RetroPixel output | Verified |
| RPPOCKET partition and image settings | 2048 MiB system, start sector 32768 | Source-inspected |
| Rockchip bootloader and boot-file staging | Device-specific image override exists | Verified in source; image not tested |
| External `extpackage` inputs | Required sources absent | Blocker |
| Hard-coded logo source | Referenced file absent | Release risk |
| Maintained frontend package | Firmware package is a prototype | Integration required |
| Legacy daemon | Still copied and launched by inspected integration | Conflict to resolve |
| Hardware service package | Not integrated into Firmware | Work required |
| Management server package | Not integrated into Firmware | Work required |
| Complete custom image | Not built in this investigation | Not yet validated |

### 16.8 Recommended order before the first image build

1. Preserve and review the existing Firmware changes; do not clean the checkout.
2. Provision and inspect the documented external source archive.
3. Resolve the hard-coded logo dependency and verify release-script error handling.
4. Replace the prototype UI package integration with the maintained frontend build.
5. Add proper packages and service definitions for the hardware service and management server.
6. Remove or explicitly disable legacy daemon startup in the production boot path.
7. Confirm the sleep package dependency closure and suspend policy.
8. Review the resolved configuration and package manifest.
9. Only then run the image build, with a backup and an understanding that `scripts/image` recreates generated install state.
10. Inspect the generated image's partition table, boot files, root filesystem, services, and persistent-storage behavior before flashing a device.
