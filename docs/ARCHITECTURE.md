# Retro KiddOS frontend architecture

This document describes the device-first SDL frontend after the November 2026
architecture refactor. It records responsibility boundaries, not a promise that
future media features are already implemented.

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
| `screens/GameScreens.cpp` | Games carousel, selected-game screen, and game information rendering |
| `screens/SystemScreens.cpp` | System Menu, shutdown confirmation, and PIN rendering |
| `screens/ParentScreen.cpp` | Parent Center rendering |
| `screens/WifiScreen.cpp` | Wi-Fi settings/password rendering and on-screen keyboard interaction |
| `screens/ScreenState.hpp` | Wi-Fi screen state and shared status-bar state |
| `services/*` | Existing device operations: game launch, battery, Wi-Fi, parent management, and power |

## Input contract

The handheld's physical controls are represented as semantic actions:

- D-pad: `Up`, `Down`, `Left`, `Right`
- A: `Confirm`
- B: `Back`
- Start: `Start`

`Input.hpp` contains the physical joystick-button translation. The SDL keyboard
event path remains as a desktop/development compatibility path; it is not part
of the handheld's physical input abstraction. The Wi-Fi password keyboard is an
on-screen UI and remains operated with the D-pad and A. `SDL_QUIT` remains a
runtime/window safety event. The existing PIN joystick-hat path is preserved.

The currently verified button IDs are B=0, A=1, Up=8, Down=9, Left=10,
Right=11, and Start=13. No L/R shoulder IDs are assumed.

## Navigation and library boundaries

`Navigation` owns the current screen and remembers the screen that opened the
System Menu. `Application` still owns screen-specific actions and decides when
to call navigation transitions.

`LibraryGrid` owns only selection behavior. `LibraryNavigator` owns the order
Games → Music → Movies and resets the grid selection when changing sections.
Each future section is expected to own its own content model and rendering; no
generic media model is introduced. Only Games is currently implemented. The
section model is ready, but physical L/R switching is not wired until the
handheld's shoulder-button IDs are identified and verified.

## State and resource ownership

Wi-Fi interaction state is grouped in `KiddosScreens::WifiState`; battery and
connection indicator values are grouped in `StatusBarState`. Device operations
remain in the existing service layer. Screens render and coordinate those
services without taking ownership of their process or system lifecycle.

SDL windows, renderers, joystick handles, fonts, textures, and text textures
remain owned by `UI`. `openUI()` and `closeUI()` remain the single lifecycle
boundary for those resources. The screen renderer modules do not initialize or
shut down SDL independently.

## PIN workflow

`PinWorkflow` owns the four-digit entry state, cursor, error state, and the
Unlock → Current PIN → New PIN → Confirm PIN flow. It reports workflow outcomes
to `Application`, which performs screen navigation and refreshes presentation.
`pin_store.h` remains the persistence boundary and was not rewritten.

## Verification status and remaining work

Host-side architecture tests cover physical-button translation, navigation
return behavior, grid bounds, library section cycling/reset, and PIN workflow
transitions. The existing PIN-store test also passes. The complete frontend
and service translation units pass a strict C++17 syntax-only check using SDL
API declaration stubs.

The ARM64 device build and on-device regression pass are still required. The
ARM64 cross-compiler and external `Build-Workspace` sysroot/dependencies were
not included in the uploaded project archive, so no device binary was built or
deployed as part of this archive refactor. The blue device remains the only
approved deployment target.
