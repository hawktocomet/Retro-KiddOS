# Retro KiddOS — Working Device UI

Refactored ARM64 frontend built and verified on the blue FunnyPlaying Retro Pixel Pocket (RPPBLUE) on November 19, 2026.

## Working features

- Four-slot carousel populated from the device-side game catalog.
- Detects slot configuration changes while running and refreshes game names, platforms, and artwork without restarting KiddOS.
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
- Scans for available Wi-Fi networks and displays selectable results.
- Supports joining both open and secured networks through ConnMan.
- Secured network passwords are entered with an on-screen QWERTY keyboard, including letter-case and symbol layouts.
- Provides keyboard navigation, password editing, connection progress, and connection-result feedback.
- Change PIN flow requiring the current PIN.

## Source organization

The SDL frontend is split into an application coordinator, screen renderers,
UI/resource ownership, and focused device services:

- `main.cpp` — process entry point only.
- `Application.hpp/.cpp` — application state, event loop, navigation orchestration, and game launch/return lifecycle.
- `Input.hpp` — semantic physical-button actions; desktop keyboard events remain a compatibility path.
- `Navigation.hpp` — active screen and System Menu return destination.
- `LibraryGrid.hpp` / `LibraryNavigator.hpp` — reusable grid selection and Games/Music/Movies section order without a generic media model.
- `PinWorkflow.hpp` — Parent PIN entry/change state machine; `pin_store.h` remains the persistence implementation.
- `UI.hpp/.cpp` — SDL resource lifecycle, shared UI state, and service/UI coordination.
- `screens/` — screen-specific rendering and Wi-Fi on-screen keyboard behavior.
- `models/Models.hpp` — shared `Game`, `WifiStatus`, and `WifiNetwork` types.
- `models/Games.hpp` — frontend game models for the four catalog-backed slots.
- `services/GameCatalog.cpp/.hpp` — loads the shared device-side Python catalog and maps its slot assignments into frontend models.
- `services/GameLauncher.*` — launches a selected game through the firmware.
- `services/BatteryService.*` — reads battery capacity and external-power state.
- `services/WifiService.*` — reads Wi-Fi state, controls radio power, loads scan
  results, and manages asynchronous connection attempts.
- `services/ParentManagementService.*` — delegates management-server controls
  to the existing device-side control mechanism.
- `services/PowerService.*` — requests system shutdown.

See [`../../docs/ARCHITECTURE.md`](../../docs/ARCHITECTURE.md) for the full
responsibility map and remaining integration boundaries. Run
`./run-architecture-tests.sh` for the SDL-independent architecture tests.

Both `CMakeLists.txt` and `build-kiddos-ui.sh` include the extracted service
implementations in the executable. The build still targets the existing ARM64
device environment and its SDL dependencies.

The frontend does not maintain a separate game catalog. `GameCatalog`
invokes the shared device-side Python catalog, which discovers ROMs under
`/storage/roms` and reads persistent assignments from
`/storage/.kiddos-management/slots.json`.

While the carousel is running, the application checks the slot file once per
second. When the file changes, it reloads the catalog and refreshes game
names, platform labels, and artwork. If the selected game is no longer
assigned, the application returns to the Games carousel. Failed reloads are
retried on a later poll.

The four slots currently accept only raw `.gb` and `.gbc` ROMs, and new
assignments require artwork. Other discovered systems remain visible in the
management library but are not currently slot-assignable.

Battery monitoring reads the device's RK817 power-supply sysfs interfaces:

- Battery capacity: `/sys/devices/platform/ff180000.i2c/i2c-0/0-0020/rk817-battery/power_supply/battery/capacity`
- External power: the `online` files under the RK817 charger's `ac` and `usb` power-supply directories.

The values are polled every five seconds. The charging indicator appears when
either AC or USB reports online. Battery percentage is displayed whenever a
valid capacity reading is available.

The battery indicator was tested on the blue Retro Pixel Pocket on November
17, 2026, including connecting and disconnecting external power. Its current
size and visual prominence are intentionally left for the next UI redesign.

Historical installed executable SHA-256 (November 17, 2026; superseded by the November 19 refactor build):

`d0942a6e2ebf0bfe336cdafde64aa839c71eb67de87a5bac05bdf0372068a53d`

## Runtime requirements

The ROM library is rooted at `/storage/roms`. The shared catalog and
persistent slot assignments are stored under
`/storage/.kiddos-management/`. Each assigned ROM requires matching sidecar
artwork (`.png`, `.jpg`, or `.webp`).

The patched `gambatte-link_libretro.so` remains required for the existing
Game Boy link-launch path.

It also requires the firmware's runemu.py, systemctl, SDL2 libraries, and Cabin Bold font at:

`/storage/.config/emulationstation/themes/rp-pocket/art/fonts/Cabin-Bold.ttf`

## Building

Run `./build-kiddos-ui.sh` from this directory.

The script expects the local Build-Workspace directory alongside the KiddOS repository, with the ARM64 SDL2 dependencies and device sysroot. It requires aarch64-linux-gnu-g++ and readelf.

The output is written to `Build-Workspace/build/arm64/kiddos-ui` by default. Set `KIDDOS_BUILD_WORKSPACE` to use a separate workspace, as was done for the November 19 refactor build. Existing output is backed up before replacement.

## Firmware integration

The tested boot hook is at `../../firmware/boot/custom_start.sh`.

It starts KiddOS as a systemd service and masks EmulationStation. Review the hook's device-specific paths and rollback procedure before installing it on another device.

## Current architecture and scope

The application coordinator is `Application`; `main.cpp` is only the process
entry point. Semantic physical-button translation, navigation, screen modules,
library navigation, PIN workflow, and SDL resource ownership are separated
into focused components. See `../../docs/ARCHITECTURE.md` for the responsibility
map and verification record.

Games is the only implemented library section. Music and Movies are prepared
navigation extension points, not implemented browsing or playback screens.
Physical L/R section switching is not wired because shoulder-button IDs have
not been verified. The four-slot catalog and parent-managed game assignments
are implemented; see `../../docs/GAME_LIBRARY.md` and
`../management-server/README.md`.

## Status

The November 19 architecture-refactor build and subsequent live-catalog frontend build are deployed and running on RPPBLUE. The application architecture, input handling, navigation, screen modules, SDL resource ownership, and PIN workflow have been refactored while preserving the existing device features. Wi-Fi power control uses ConnMan and is tested locally on the handheld. Remote Management remains independent of Wi-Fi power and was re-tested after Wi-Fi was turned off and restored.

Wi-Fi configuration is complete for the current milestone. Network scanning, network selection, open-network connections, secured-network password entry, and connection handling were tested on the blue Retro Pixel Pocket. The on-screen QWERTY keyboard was also tested and confirmed working.

A subsequent saved-network switching test exposed a ConnMan/D-Bus timing case: a connection request could return `org.freedesktop.DBus.Error.NoReply` even though the wireless association was completing. The helper now treats that specific result as a possible delayed completion and polls the requested ConnMan service state; it reports success only after the service reaches `ready` or `online`. Other failures remain failures, and `NoReply` is not treated as a password error. Switching between the two saved home networks was re-tested successfully on the blue device on November 19, 2026.

The current Wi-Fi workflow is considered complete and frozen. Defer further edge-case work unless real-world use exposes another problem.

The original desktop SDL application is archived project history. The device-first SDL frontend documented here is the active development path.

Deployed refactored ARM64 executable SHA-256 (November 19, 2026):

`1238648b823a2f09342bb646d5aee6f135a8d426647ea2b7bd6c7b5825bb3508`

The subsequent live-catalog frontend build deployed to RPPBLUE had SHA-256:

`209edf4c07e2f16227d41c5969650182b410f9ef26eb8a3bf05bc9d6353ab10c`

The previous November 18 build checksum is historical and is not the currently deployed executable.

## Deployment and rollback

The deployment script defaults to the approved blue device (`root@192.168.0.89`) and expects the executable at `Build-Workspace/build/arm64/kiddos-ui` alongside the repository. If using a separate build workspace, make its output available at that expected path before deploying.

Deploy with `KIDDOS_DEVICE=root@192.168.0.89 ./deploy-kiddos-ui.sh deploy`.

The script stages the executable, verifies its SHA-256, creates a timestamped backup of the installed executable, replaces it, and starts the `kiddos-ui` service. If the new service fails its four-second active check, it attempts automatic rollback.

Keep the backup path printed by deployment. Manual rollback uses `KIDDOS_DEVICE=root@192.168.0.89 ./deploy-kiddos-ui.sh rollback BACKUP_PATH`, replacing `BACKUP_PATH` with the exact printed path.

The automated service check does not verify screen rendering, controller input, game launching, or other UI workflows. Perform the on-device regression checks after deployment. Do not deploy to the purple or green devices without explicit approval.
