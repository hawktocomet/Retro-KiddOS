# Retro KiddOS frontend architecture

This document describes the device-first SDL frontend after the November 2026
architecture refactor. It records current responsibility boundaries and
distinguishes implemented behavior from prepared extension points.

This is the **KiddOS application architecture**, not the architecture of the
entire device platform. Project ownership and the intended separation from
Retro Pixel Foundation are documented in
[PROJECT-BOUNDARIES.md](PROJECT-BOUNDARIES.md).

The current frontend still contains stock-firmware-specific service
integrations. Their presence is part of the compatibility baseline; this
document does not claim that the platform interfaces have already been
fully separated or implemented by Foundation.

## Application flow

`main.cpp` is intentionally a small entry point. `Application` owns the
application-level state and runs the established lifecycle:

1. Initialize the PIN store.
2. Create SDL/UI resources.
3. Process input, navigation, screen actions, and asynchronous service results.
4. Destroy UI resources before launching a selected game.
5. Return to the Games library after the emulator exits.

The existing firmware, emulator launcher, and service processes remain in
place. This refactor does not replace the operating system or emulator stack.

## Source responsibilities

| Source | Responsibility |
|---|---|
| `main.cpp` | Process entry point only |
| `Application.hpp/.cpp` | Application state, main loop, high-level event orchestration, game launch/return lifecycle |
| `Input.hpp` | Translate physical joystick button IDs into semantic actions |
| `Navigation.hpp` | Current screen and System Menu return destination |
| `LibraryGrid.hpp` | Two-column grid selection and bounds handling |
| `LibraryNavigator.hpp` | Games/Music/Movies section order and selection reset; no shared media-item model |
| `PinWorkflow.hpp` | Parent PIN unlock/change state machine |
| `pin_store.h` | Existing PIN persistence and hash verification; retained unchanged |
| `UI.hpp/.cpp` | SDL resource ownership, UI initialization/cleanup, shared status, and device-service/UI coordination |
| `screens/GameScreens.cpp` | Games carousel, selected-game screen, game information, and artwork refresh |
| `services/GameCatalog.cpp/.hpp` | Loads the shared device-side Python catalog and maps its four slot assignments into frontend game models |
| `screens/SystemScreens.cpp` | System Menu, shutdown confirmation, and PIN rendering |
| `screens/ParentScreen.cpp` | Parent Center rendering |
| `screens/WifiScreen.cpp` | Wi-Fi settings/password rendering and on-screen keyboard interaction |
| `screens/ScreenState.hpp` | Wi-Fi screen state and shared status-bar state |
| `services/*` | Device operations: game launch, battery, Wi-Fi, parent management, and power |

## Input contract

The handheld's physical controls are represented as semantic actions:

- D-pad: `Up`, `Down`, `Left`, `Right`
- A: `Confirm`
- B: `Back`
- Start: `Start`

`Input.hpp` contains the physical joystick-button translation. The SDL keyboard
event path remains a desktop/development compatibility path; it is not part
of the handheld's physical input abstraction. The Wi-Fi password keyboard is an
on-screen UI operated with the D-pad and A. `SDL_QUIT` remains a runtime/window
safety event. The existing PIN joystick-hat path is preserved.

The verified physical button IDs are B=0, A=1, Up=8, Down=9, Left=10,
Right=11, and Start=13. No L/R shoulder IDs are assumed.

## Navigation and library boundaries

`Navigation` owns the current screen and remembers the screen that opened
the System Menu. `Application` owns screen-specific actions and decides when
to call navigation transitions.

`LibraryGrid` owns selection behavior. `LibraryNavigator` owns the order
Games → Music → Movies and resets the grid selection when changing sections.
Each future section is expected to own its own content model and rendering; no
generic media model is introduced. Only Games is currently implemented. The
section model is ready, but physical L/R switching is not wired until the
handheld's shoulder-button IDs are identified and verified.

## Device-side game catalog integration

The native `KiddosLibrary` service in
`device/working-ui/services/LibraryData.cpp` discovers eligible content
directly beneath `/storage/roms`. It validates resolved paths against the ROM
root, ignores hidden paths, and requires the category's artwork and content
requirements before an item appears in Parent Center.

The current native game reader offers raw `.gb` and `.gbc` files with
matching artwork. Movies are individual files in `movies/`; music playlists
and TV shows are immediate child directories in `music/` and `tvshows/`.
Playlist tracks and show episodes are discovered recursively inside their
respective containers.

Four stable slot references per category are persisted in
`/storage/.config/kiddos/slots.conf`. The file begins with
`KIDDOS_SLOTS_V1`; references are relative to the ROM root. The native slot
manager loads and saves these references without maintaining a second
catalog database. Parent Center assignment operations do not move or delete
media files.

The legacy browser Remote Management service and Python catalog are not
part of the current architecture. Wi-Fi remains a separate device service.

## State and resource ownership

Wi-Fi interaction state is grouped in `KiddosScreens::WifiState`; battery and
connection indicator values are grouped in `StatusBarState`. Device operations
remain in the existing service layer. Screens render and coordinate those
services without taking ownership of their process or system lifecycle.

SDL windows, renderers, joystick handles, fonts, textures, and text textures
remain owned by `UI`. `openUI()` and `closeUI()` remain the single lifecycle
boundary for those resources. Screen renderer modules do not initialize or
shut down SDL independently.

## PIN workflow

`PinWorkflow` owns the four-digit entry state, cursor, error state, and the
Unlock → Current PIN → New PIN → Confirm PIN flow. It reports workflow outcomes
to `Application`, which performs screen navigation and refreshes presentation.
`pin_store.h` remains the persistence boundary and was not rewritten.

## Build and verification

The refactored frontend was built successfully for ARM64 using the project's
external build workspace and the `aarch64-linux-gnu-g++` cross-compiler. The
build output was an AArch64 ELF executable with the expected SDL2, SDL2_image,
and SDL2_ttf runtime dependencies.

Host-side architecture tests passed for:
- Semantic physical-button mappings.
- Navigation and System Menu return destination.
- Grid movement and bounds handling.
- Library section switching and selection reset.
- PIN unlock, error, change, mismatch, and save transitions.

The refactored executable was deployed to RPPBLUE on November 19, 2026. The
device checksum matched the local ARM64 build:

`1238648b823a2f09342bb646d5aee6f135a8d426647ea2b7bd6c7b5825bb3508`

On-device regression checks covered startup/rendering, navigation, System Menu
and return behavior, game launch/return, shutdown confirmation, Parent Mode
and PIN workflows, Wi-Fi settings/password keyboard, and battery/charging
indication. All reported checks passed.

This validates the blue device's current configuration, not portability to
other devices or firmware versions. The deployment script's automated health
check only verifies that the systemd service remains active after launch; it
does not replace visual, controller, or workflow testing on the handheld.

## Deployment boundaries and remaining work

The deployment target is the blue device RPPBLUE. The deployment script backs
up the existing executable before replacement and verifies the staged SHA-256.
Keep the printed backup path for manual rollback if a visual or input
regression is found.

The following remain outside the current implementation:
- Music or Movies browsing/playback screens.
- Physical L/R library section switching; shoulder-button IDs remain unverified.
- Multiplayer workflows, including Pokémon trading and head-to-head play.

Do not add generic frameworks or media abstractions ahead of a concrete
user-facing need. Preserve existing firmware services and prioritize changes
that make it easier for the children to play games together.
