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
