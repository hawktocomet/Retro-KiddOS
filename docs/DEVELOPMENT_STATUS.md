# Development Status

Last updated: 2026-11-11

This document distinguishes working development infrastructure from product
features that are still planned. A successful desktop build does not establish
compatibility with the Retro Pixel Pocket hardware.

## Current development baseline

### Project and build

- C++20 application using SDL2, SDL_ttf, and SDL_image.
- CMake-based build and CTest test suite.
- Arch Linux is the temporary development and testing environment.
- The target remains the FunnyPlaying Retro Pixel Pocket; desktop support is
  not a product target.

### Application prototype

- SDL2 application lifecycle and platform adapter are in place.
- A 720 × 720 Kid Mode carousel prototype exists, with placeholder game
  artwork, directional selection, and sound/battery indicators.
- A game action screen demonstrates Play, Resume, and Start Over selections.
- The action screen uses demonstration state only: real checkpoint discovery,
  save-state integration, and emulator launching are not implemented.

### Reusable SDL UI library

The `kiddos_ui` static library is implemented as a reusable application UI
foundation. It currently includes:

- Core elements, containers, geometry, layout, renderer, theme, fonts, text,
  and textures.
- Button, Image, Label, Panel, ProgressBar, Toggle, Slider, Checkbox,
  RadioGroup, Select, TextInput, and ListView widgets.
- Focus management and directional controller-action dispatch.
- Text-input interaction support.
- Screen lifecycle and a stack-based ScreenManager.
- Public umbrella headers, the `KiddOS::UI` CMake alias, and install rules.
- Automated tests covering library components, image opacity, rounded-panel
  rendering, and screen-manager lifecycle/navigation.

See [KiddOS UI Library](KIDDOS_UI_LIBRARY.md) for build instructions and
integration examples.

## Not yet implemented

- Integrating the reusable UI library into the actual application screens.
- Final child-facing carousel and game-action experience using production
  artwork and final interaction behavior.
- PIN-protected Parent Center and parent-managed game visibility.
- Real emulator discovery, game launching, and save/checkpoint integration.
- Parent-managed artwork and gameplay previews.
- Device battery/charging integration and low-battery protection.
- Verification and implementation of device-specific controls, display,
  audio, suspend/resume, and firmware integration on the actual handheld.

## Next milestone

Integrate the reusable UI library into the application and use it to build
the Kid Mode home/carousel screen. Keep game catalog and application policy
outside the generic widget library. Preserve the existing prototype as a
behavioral reference while replacing its hard-coded UI with reusable screens
and widgets.

## Verification expectations

For code changes, configure and build with CMake, run the complete CTest suite,
and perform a desktop runtime smoke test when application behavior changes.
Hardware-specific claims require testing on the actual Retro Pixel Pocket.
