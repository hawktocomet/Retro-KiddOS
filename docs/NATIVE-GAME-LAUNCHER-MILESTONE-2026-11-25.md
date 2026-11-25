# Native Game Launcher & EmulationStation Independence

**Milestone date:** 2026-11-25
**Device tested:** RPPBLUE
**Status:** Implemented, deployed, and functionally verified on hardware

## Goal

Launch Game Boy and Game Boy Color games directly through RetroArch,
without using the firmware's `runemu.py` game-launch path or relying on
EmulationStation to launch games.

Preserve the existing KiddOS Games, Movies, Music, and TV Shows experience.

## Scope

This milestone intentionally supports:

- Game Boy (GB)
- Game Boy Color (GBC)

GBA, NES, SNES, Genesis/Mega Drive, Master System, and arcade remain
deferred until separately audited and implemented.

## Implementation

`device/working-ui/services/GameLauncher.cpp` launches RetroArch directly:

- Executable: `/usr/bin/retroarch`
- User-supplied core: `/storage/cores/gambatte_libretro.so`
- Base configuration: `/storage/.config/retroarch/retroarch.cfg`
- KiddOS overlays: `/storage/.config/kiddos/retroarch/gb.cfg` and `gbc.cfg`

The launcher passes the selected profile using `--appendconfig` and
loads the selected ROM with the specified core.

The launcher accepts GB and GBC games only. It validates required paths,
starts RetroArch, monitors the controller exit gesture, waits for the
emulator process to finish, and returns to the KiddOS library.

No `runemu.py` or EmulationStation launch invocation remains in
`GameLauncher.cpp`.

## Controller behavior

KiddOS deploys a controller profile override to:

`/storage/joypads/GO-Super Gamepad.cfg`

The firmware profile remains untouched.

The override retains normal gameplay mappings but omits RetroArch
gamepad hotkey assignments. On hardware, this successfully disabled
the firmware shortcuts while preserving normal Select and Start inputs.

**Important OverlayFS deployment note:**

RetroArch reads controller profiles from `/tmp/joypads`, an OverlayFS
mount whose upper directory is `/storage/joypads`.

Creating the override in `/storage/joypads` after that overlay is mounted
does not refresh the mounted view. A device reboot is required for the
new override to appear in `/tmp/joypads`.

After reboot, the active and persistent profile hashes matched:

`6e73bb6f3e5ee5f2eb410fc62bce9d8e5f1a76fc8e0afddad23018680c0d3bbc`

The active profile contained no RetroArch hotkey assignments.

The KiddOS Start + Select exit gesture was tested successfully.

An incidental observation: the device's right volume button triggers
RetroArch fast-forward. This was left unchanged and is not a defined
KiddOS feature.

## Save behavior

- Native in-game saves are enabled.
- Automatic save states are disabled.
- Save-state hotkeys are inaccessible through the Kid Mode controller.
- Existing RetroArch save directories and core options are preserved.
- Native in-game save persistence across a full device reboot was
  confirmed on hardware.

## Deployment

Deployment script:

`device/working-ui/deploy-kiddos-ui.sh`

The deployment includes:

- KiddOS ARM64 executable
- MPV input configuration
- GB and GBC RetroArch profile overlays
- GO-Super Gamepad controller override

The script includes backup and rollback handling for the deployed files.

Deployed executable path on RPPBLUE:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

Build artifact:

`../Build-Workspace/build/arm64/kiddos-ui`

SHA256:

`f2356e0eb9a6366334ef795af25cc6ea27dc0471b303115823baa94479ce0204`

## Hardware verification

User-confirmed tests on RPPBLUE:

- [x] GB/GBC games launch through the native KiddOS launcher.
- [x] RetroArch firmware gamepad hotkeys are disabled.
- [x] Normal Select and Start gameplay inputs work.
- [x] Start + Select exits the game and returns to KiddOS.
- [x] Native in-game saving works.
- [x] Native saves survive a full device reboot.
- [x] Movies section opens and video playback works.
- [x] Music section opens and audio playback works.
- [x] TV Shows section opens, browses, and plays content.
- [x] Games section opens and launches GB/GBC games.

## Final local validation

On 2026-11-25:

- `bash -n device/working-ui/deploy-kiddos-ui.sh` passed.
- `git diff --check` passed.
- The local ARM64 build hash matched the deployed build hash.

## Distribution principles

KiddOS does not bundle ROMs, BIOS files, media, or emulator cores.
The user supplies their own content and compatible emulator cores.
KiddOS owns the launcher experience and core discovery/user workflow,
not distribution of copyrighted game content or emulator cores.

## Deferred work

- Add further systems one at a time, beginning with a separate core and
  launch audit for each platform.
- Continue validating the user-facing core sourcing/install experience.
- Keep multiplayer and kid-friendly library usability ahead of
  unnecessary frontend or firmware complexity.
