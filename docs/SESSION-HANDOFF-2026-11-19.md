# Retro KiddOS — Session Handoff

**Date:** November 19, 2026

## Product goal and guardrail

Prepare the FunnyPlaying Retro Pixel Pocket devices for easy family gaming by
Christmas. The 3-year-old and 6-year-old should be able to turn on their
handhelds, choose a game, and play with minimal adult help.

> Every hour we spend developing should make it easier for you and your kids
> to play games together. If it doesn't, we probably shouldn't be building it.

Keep the existing firmware and its emulator/networking capabilities wherever
possible. Avoid broad OS work and speculative infrastructure.

## Hardware and deployment boundary

- The blue Retro Pixel Pocket is the only device used for current development
  and deployment.
- Device hostname: `RPPBLUE`.
- Purple and green devices have not been validated; do not deploy to them
  without explicit direction and a separate review of paths and rollback steps.
- The device uses ARM64 Linux firmware, SDL2, ConnMan, and the existing
  RetroArch/emulator infrastructure.

## SDL frontend source refactor

The working SDL frontend has been reorganized so `main.cpp` focuses on UI
screens, input, and UI state while device operations live behind small service
interfaces.

### Shared models

`device/working-ui/models/`

- `Models.hpp` defines the shared `Game`, `WifiStatus`, and `WifiNetwork`
  data types.
- `Games.hpp` contains the current four-game catalog: Mario Tennis, Pokemon
  Red, Super Mario Bros. Deluxe, and Tetris.

### Services

`device/working-ui/services/`

- `BatteryService` reads battery capacity and external-power state.
- `GameLauncher` launches a selected game through the existing firmware.
- `WifiService` reads Wi-Fi state, controls radio power, loads scan results,
  starts and polls asynchronous connection attempts, and supports cancellation.
- `ParentManagementService` delegates management-server actions to the
  existing device-side management control mechanism.
- `PowerService` requests system shutdown.

`CMakeLists.txt` and `build-kiddos-ui.sh` both include the extracted service
source files in the executable. The refactor is an internal separation of
responsibilities; it does not introduce a new UI framework or replace firmware
services.

The game catalog is still hardcoded. Parent-managed game libraries and slot
assignment are future work.

## Wi-Fi configuration and saved-network switching

The Parent Mode Wi-Fi workflow includes:

- Wi-Fi radio power and connection status.
- Scanning and displaying available networks.
- Selecting open or secured networks.
- On-screen QWERTY password entry, including letter-case and symbol layouts.
- Connection progress and result feedback.

The workflow and keyboard were deployed and physically tested on the blue
device. A later saved-network switching test exposed a ConnMan/D-Bus timing
case: `org.freedesktop.DBus.Error.NoReply` could be returned while the
wireless connection was still completing.

The helper in `device/working-ui/wifi_helper.py` now handles that specific
case by polling the requested ConnMan service state for a limited period. It
reports success only if the service reaches `ready` or `online`; otherwise it
preserves the failure. `NoReply` is not classified as an authentication or
wrong-password error.

The user re-tested switching between the two saved home networks on November
19, 2026 and confirmed that it works.

This closes the current Wi-Fi milestone. Do not continue polishing Wi-Fi
without a new reproducible issue.

## Existing verified Parent Mode behavior

On the blue device:

- Parent Mode is hidden behind PIN entry.
- Parents can change the PIN after verifying the current PIN.
- Remote Management reports and controls the LAN management server.
- Wi-Fi Settings reports connection/radio state and controls Wi-Fi power.
- Remote Management remains independent of Wi-Fi radio power.
- The Wi-Fi indicator appears beside the battery indicator.

The PIN is a practical child-proofing measure, not a security boundary against
root or physical filesystem access.

## Management server

Management server v0.3.0 provides a read-only local inventory of games and
movies. It is independently controlled from Parent Mode. Uploads, deletion,
active-slot changes, and browser-side configuration writes are not implemented.

Do not duplicate the management server's lifecycle logic in the SDL frontend;
continue to use the installed control script.

## Current product scope

Round-one game systems:

- Game Boy
- Game Boy Color
- Game Boy Advance
- Arcade
- SNES
- Genesis/Mega Drive
- Master System
- NES

Curate independent-play lists separately for the 3-year-old and 6-year-old,
plus shared parent-supported multiplayer. Favor simple controls, forgiving
gameplay, and minimal reading. The 6-year-old enjoys Pokémon and is learning
to read; distinguish low-reading options from reading-heavy titles.

## Deferred work

- Browser-based media upload, library organization, and slot assignment.
- Browser-based theme customization.
- Automatic network priority/fallback and phone-hotspot fallback.
- IBSS/ad-hoc networking.
- Multiplayer workflows such as Pokémon trading and Mario Kart head-to-head.
- Full-length movie playback and a complete music experience.
- Validation on the purple and green handhelds.
- Installer/distribution work and any full-firmware redistribution audit.

## Development workflow and safety

- Work incrementally and provide one concise terminal-ready command at a time.
- The user runs commands locally and shares the output; do not imply direct
  access to their repository or device.
- Do not edit, build, deploy, stage, commit, push, delete, or clean anything
  unless the user has authorized that action.
- Keep timestamped `.pre-*` backups and Python `__pycache__` directories out
  of Git; do not delete them as part of routine cleanup.
- Stage explicit intended files rather than using `git add -A`.
- Do not print or expose Wi-Fi passwords or other credentials.
- Public copyright holder in project notices: `hawktocomet`; do not publish
  the user's legal name.

## Suggested next step

With this refactor and Wi-Fi milestone documented, review the intended source
and documentation changes, run appropriate local checks, then commit and push
the reviewed files. Keep the next development slice tied to making it easier
for the children to play games together.

## SDL frontend architecture refactor — archive work in progress

The source-level architecture refactor was performed from the uploaded project
archive at baseline `ff1b344cd962bc23af1413fd31e01f5338d1a27a`. The SDL frontend
is now divided into:

- `main.cpp` as a minimal entry point and `Application` as the application
  coordinator/state owner.
- `Input.hpp` for semantic physical joystick actions. Desktop SDL keyboard
  compatibility remains separate; the on-screen Wi-Fi keyboard remains intact.
- `Navigation.hpp` for active-screen state and System Menu return behavior.
- `UI.hpp/.cpp` for SDL resources, initialization/cleanup, shared UI state, and
  coordination with existing device services.
- Individual renderer/interaction modules under `screens/` for Games,
  System Menu/PIN, Parent Center, and Wi-Fi.
- `PinWorkflow.hpp` for PIN entry/change behavior; `pin_store.h` remains the
  unchanged persistence boundary.
- `LibraryGrid.hpp` and `LibraryNavigator.hpp` for grid selection and the
  Games/Music/Movies section order without a generalized media-item model.
- `screens/ScreenState.hpp` for grouped Wi-Fi and status-bar state.

The existing device services and firmware/emulator lifecycle are retained.
The section navigator resets selection when changing categories, but physical
L/R shoulder-button mapping is not wired because their SDL button IDs have not
been verified. Games remains the only implemented library section.

Host architecture tests and the existing PIN-store tests pass. All frontend
and service translation units pass a strict C++17 syntax-only check against
SDL API declaration stubs. The uploaded archive did not contain the external
ARM64 `Build-Workspace`, SDL sysroot, or cross-compiler, so no ARM64 binary was
built and no device was changed. Before deployment, build with the user's
existing ARM64 workspace, then run the complete regression checklist on
RPPBLUE only. Purple and green remain out of scope.
