# Development Log

## 2026-11-09 — Project foundation

### Project setup

- Established KiddOS as an independent project, separate from DualOS.
- Selected the FunnyPlaying Retro Pixel Pocket as the first hardware target.
- Selected Arch Linux as the initial desktop development environment.
- Confirmed GCC 16.2, CMake 4.4.4, Ninja 1.13.2, Git 2.56.0, SDL2 2.32.74,
  OpenGL pkg-config metadata, and Vulkan tooling are present.

### Initial implementation direction

- C++20 and SDL2 desktop prototype.
- CMake build system.
- Shared core with desktop and EmuELEC platform adapters.
- Documentation-first workflow.

### Initial prototype

- Added a 720 × 720 SDL2 window.
- Added a dark navy background.
- Added Escape and window-close handling.

### Not yet implemented

- Game carousel and artwork.
- Controller navigation.
- Parent Center and PIN.
- Game launching.
- Save-state integration.
- Media Manager.
- Battery monitoring.
- EmuELEC integration.

### Known limitations

The target device has not yet been inspected. Hardware specifications and
software compatibility still require verification.


## 2026-11-09 — Product model clarification

### Decision

KiddOS has one child-facing gaming experience and a separate parent
management area. There is no separate Adult Mode or adult gaming frontend.

### Parent management responsibilities

- Choose which installed games are visible to the child.
- Manage game artwork and gameplay previews.
- Organize games and configure child-facing features.
- Potentially provide future controls such as playtime limits and schedules.

The parent area configures the child's experience; it is not an alternative
gaming environment.

### Documentation updated

- README product description and goals.
- Project vision and product model.
- Game library and parental visibility.
- Media Manager parent workflow.

## 2026-11-10 — Consolidated product baseline

### Product scope

- Confirmed the FunnyPlaying Retro Pixel Pocket as the sole initial target.
- Clarified that KiddOS is a handheld product, not a general desktop OS.
- Clarified that the Arch Linux build is a temporary development and testing
  harness, not a supported desktop edition.
- Confirmed the 720 × 720 logical interface and physical-control-first design.
- Reaffirmed that KiddOS is separate from DualOS.

### Product specification

- Added `docs/PRODUCT_SPECIFICATION.md` as the consolidated product baseline.
- Documented Kid Mode and the PIN-protected Parent Center.
- Consolidated game carousel, game actions, previews, and navigation behavior.
- Consolidated game visibility, save/checkpoint, and media-management rules.
- Recorded low-battery thresholds as provisional and hardware-dependent.
- Recorded the hardware inspection gate and non-destructive development rules.

### Architecture and documentation

- Clarified the separation between shared application logic and
  device-specific integration.
- Clarified that desktop simulation does not establish handheld compatibility.
- Updated README, project vision, architecture, and contribution guidance.
- Retained the detailed UI, input, game library, save, media, battery, and
  hardware documents as supporting references.

### Not yet implemented

The product specification records intended behavior, not completed features.
The existing SDL2 prototype remains a basic development harness. Hardware
integration and the planned product components remain future work.

## 2026-11-10 — Manufacturer firmware source audit

- Cloned the public `FRetroPixel/RetroPixel_Pocket` repository for local,
  read-only source inspection.
- Audited revision
  `13e1c48d86c95ae2b8a99d6baf02e4e394629cb3` (2025-12-02).
- Confirmed the `RPPOCKET` build target selects an RK3326 v6.0 device tree,
  ARM64/Cortex-A35 settings, Mali G31/libmali, and the inherited RG351MP
  Linux/U-Boot platform identifiers.
- Identified configured Linux graphics, input, RK817 battery/charger, and
  Rockchip thermal support.
- Inspected inherited battery, ADC key, volume, headphone, joystick LED,
  and suspend/resume utilities.
- Confirmed the inspected headphone script only handles D007 Plus and
  PowKiddy XU10, while the joystick LED script only handles D007 Plus.
  Neither contains an RPPOCKET-specific implementation.
- Confirmed the kernel and U-Boot package definitions refer to external
  source directories not present in the inspected checkout.
- No firmware was built or flashed. No device storage or boot configuration
  was changed.
- Added `docs/FIRMWARE_AUDIT.md` with findings, confidence levels, and the
  runtime verification plan.
- Next: continue source audit of boot flow, package startup, frontend/game
  launching, and external kernel/U-Boot source availability. After the
  handheld arrives, verify actual hardware interfaces before implementing
  device-specific adapters.

## 2026-11-10 — Application foundation restored

- Restored the application lifecycle and SDL platform implementation.
- Corrected malformed project include directives.
- Configured and compiled the C++20 SDL2 application.
- Ran a headless SDL runtime smoke test using SDL's dummy video driver.
- No device firmware, storage, or boot configuration was changed.

## 2026-11-10 — First visible Kid Mode carousel

- Added the first three-game Kid Mode carousel to the SDL2 desktop harness.
- Implemented a centered selected game with smaller neighboring previews.
- Added procedural placeholder artwork and the approved navy/blue visual
  treatment.
- Added left/right keyboard and controller navigation with wrapping
  selection.
- Added visual sound-status and battery-status indicators.
- Kept rendering behind the platform interface and selection state in the
  application layer.
- Replaced an SDL-specific delay call in application logic with a
  platform-independent C++ thread sleep.
- Built the application successfully and completed a desktop preview run.
- Added `docs/KID_MODE_CAROUSEL_PROTOTYPE.md` documenting scope and limits.
- No device firmware, storage, or boot configuration was changed.

## 2026-11-10 — Game selection and action screen

- Added a separate game action screen to the application state.
- Added large horizontal Play, Resume, and Start Over selections.
- Added static selected-game artwork to the action screen.
- Implemented Confirm to enter and Back to return to the carousel.
- Implemented Up/Down action selection for the checkpoint demonstration.
- Preserved the selected game when returning to the carousel.
- Used a temporary sample checkpoint fixture; no real save-state detection
  or emulator launch is implemented.
- Built the SDL2 application and ran a headless runtime smoke test.
- No device firmware, storage, or boot configuration was changed.
