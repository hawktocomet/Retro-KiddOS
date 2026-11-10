# Retro KiddOS — Product Specification

**Status:** Working product baseline
**Last updated:** 2026-11-10
**Primary target:** FunnyPlaying Retro Pixel Pocket

This document consolidates the product decisions and requirements established
for Retro KiddOS. Supporting documents contain detailed behavior and
implementation requirements. If a supporting document is more specific, it
should be read together with this specification.

## 1. Product definition

Retro KiddOS is a child-friendly gaming environment designed to make a
handheld gaming device simple and safe for young children, with adult
management controls.

The product has exactly one target device for its initial development:
the FunnyPlaying Retro Pixel Pocket.

KiddOS is a handheld product, not a general-purpose desktop operating system
or a desktop gaming frontend.

The Arch Linux development machine is a temporary engineering host. Its
desktop build exists only to develop and test shared application logic and
simulate device input. It is not a supported product edition.

### Core product goals

- A simple, predictable, physical-control-first child experience.
- Parent-controlled access to installed games and features.
- An artwork-driven game carousel.
- Straightforward game launching, resuming, and restarting.
- Automatic gameplay checkpoints where emulator support permits.
- A PIN-protected Parent Center.
- Parent-managed game artwork and gameplay previews.
- Child-friendly battery and power protection.
- Offline operation for the core experience.

## 2. Product scope and target

### Primary hardware

FunnyPlaying Retro Pixel Pocket.

Current reported hardware baseline:

- Rockchip RK3326
- Quad-core ARM processor
- 1 GB RAM
- 3.1-inch IPS display
- 720 × 720 resolution
- 3000 mAh battery
- Wi-Fi
- microSD/TF storage
- Customized EmuELEC Linux

These specifications remain subject to verification on the actual device.

### Display and input

- Logical UI resolution: 720 × 720.
- Primary interaction: physical handheld controls.
- The interface must be designed for the device's square display.
- Mouse-first interaction is not a product requirement.
- Keyboard or controller input on the development host may simulate
  physical controls during development.

### Explicit exclusions

- No general desktop edition.
- No separate adult gaming mode or parallel adult frontend.
- No requirement to support other handhelds in the initial release.
- No assumption that desktop behavior proves handheld compatibility.
- No destructive firmware or storage operations without explicit approval.

KiddOS is a separate project from DualOS.

## 3. Product modes and access

### Kid Mode

Kid Mode is the default experience after startup.

It presents only the games and features approved by a parent. A child should
be able to pick up the device, select a game, play, and return to the game
selection screen without needing an adult to explain the interface.

### Parent Center

Parent Center is a PIN-protected management area.

It exists to configure the child's KiddOS experience. It is not a second
gaming frontend and does not expose an alternative adult gaming environment.

Parent Center responsibilities include:

- Enabling and hiding individual games.
- Managing game artwork and previews.
- Managing game saves and automatic checkpoints.
- Organizing games into collections.
- Configuring display and volume settings.
- Configuring which child-facing features are available.
- Potential future playtime limits, schedules, and additional parental
  controls.

## 4. Kid Mode interface

### Visual direction

- Dark navy background.
- Bright blue highlights.
- Large, clear selection areas.
- Minimal visual clutter.
- Symbols are primary; text is secondary.
- Physical controls are the primary navigation method.

### Game carousel

- The selected game appears large and centered.
- Neighboring games appear smaller and somewhat transparent.
- Neighboring games do not have prominent text labels.
- Left and Right browse between games.
- A small sound-status indicator appears in the upper-left.
- A battery indicator appears in the upper-right.
- Battery appearance changes as charge falls.
- Do not display battery percentages or words in the normal indicator.
- A warning is reserved for the low-battery protection threshold.

### Game selection and actions

Selecting a game opens its game detail/action screen.

- Show static game artwork.
- Present available actions as large, stacked, full-width horizontal boxes.
- Each action has an oversized symbol on the left and its label on the right.
- Do not replace these with small square cards or tiny icons.

Available actions depend on save state:

- First launch: Play.
- Valid automatic checkpoint: Resume and Start Over.

### Gameplay previews

- Static artwork appears first.
- If a preview video exists, a muted preview may begin after approximately
  2.5 seconds.
- The preview uses the same display area as the artwork.
- The preview loops.
- Browsing to another game restarts the delay.
- Launching a game stops preview playback immediately.
- If no preview exists, static artwork remains visible.
- Do not run an emulator in the background to generate previews.

## 5. Parent Center

Parent Center is a practical management interface. Its screens should provide
useful contextual information alongside their controls, not just isolated
settings.

The designed areas include:

### Game Management

Parents choose which installed games are available in Kid Mode. Hiding a
game must not uninstall it or delete its files, native saves, automatic
checkpoints, or cached media.

### Save Manager

The parent selects a game before managing its save/checkpoint data.

Automatic checkpoints and native in-game saves are distinct. Destructive
operations, restore behavior, and confirmation steps must be defined before
implementation.

### Media Manager

Parents can retrieve, preview, select, crop, position, and import game media.
A one-click acceptance of automatically selected media should be available.

### Display and Volume

These screens should explain the effect of their settings with useful
contextual information, consistent with the approved Parent Center designs.

### Features

Parent-managed controls for which child-facing features are available.
Potential future controls include playtime limits and schedules.

The approved Parent Center screen designs are the visual baseline. Do not
redesign them without a specific reason or a new decision.

## 6. Physical controls and navigation

### Kid Mode

- Left / Right: browse games.
- Up / Down: choose a game action.
- A: confirm.
- B: normal in-game B button.
- X / Y / L / R / START / SELECT: normal in-game controls, except for the
  universal START + SELECT shortcut.

### Universal shortcut

START + SELECT held continuously for approximately two seconds:

- During gameplay: request the child-friendly exit confirmation.
- On the game carousel: open the Parent Center PIN screen.

A short press of either button individually must not trigger the shortcut.

The shortcut must ultimately work across supported emulators. Its final
implementation depends on auditing the device's input handling and emulator
integration. Desktop simulation alone does not establish compatibility.

### Exit confirmation

The child sees two clear choices:

- Yes, return to games.
- No, keep playing.

On confirmation, the game should close gracefully and return to the carousel.
The previously selected game should remain selected.

See `INPUT_CONTROLS.md` for the detailed control requirements.

## 7. Game library and parental visibility

- Parents selectively enable individual installed games for Kid Mode.
- Only enabled games appear in the child's carousel.
- Hiding a game is a visibility/access change, not an uninstall operation.
- Hiding must preserve ROMs/game files, native saves, automatic checkpoints,
  and cached media.
- Collections are organizational aids only; they do not independently
  determine child access.
- Only games being enabled for Kid Mode enter the media-scraping workflow.

See `GAME_LIBRARY.md` for details.

## 8. Save and resume behavior

### Automatic checkpoints

On child-requested exit, KiddOS should attempt to create an automatic
checkpoint before returning to the carousel.

Automatic checkpoints are separate from native in-game saves.

### Launch behavior

- No valid automatic checkpoint: show Play.
- Valid automatic checkpoint: show Resume and Start Over.

Start Over clears only the automatic resume checkpoint. It must never erase
native in-game saved progress.

Save-state creation and restoration depend on emulator support. The system
must handle failures safely and must not claim that a save succeeded when it
did not.

Compatibility must be validated per emulator.

See `SAVE_STATE_SYSTEM.md` for details.

## 9. Media management

The Media Manager is part of the parent workflow for enabling a game.

The workflow identifies the game and platform, retrieves available media,
allows the parent to preview and select it, and then makes the game visible
to the child.

Supported media workflows include:

- Automatically selected artwork with one-click acceptance.
- Manual selection of artwork and screenshots.
- Cropping and positioning.
- Local artwork and video import.
- Optional muted gameplay previews.

Preserve parent-selected media. Re-scraping must not silently overwrite it.
Partial media availability must not prevent a game from being enabled.

Media source terms, attribution, API access, and licensing must be researched
before integrating a scraper.

See `MEDIA_MANAGER.md` for details.

## 10. Battery and power protection

KiddOS should monitor battery state during gameplay.

The current design proposes:

- Approximately 15%: attempt an automatic checkpoint, pause gameplay, and
  display a child-friendly full-screen charging message.
- Prevent normal Kid Mode input from dismissing the charging screen.
- Detect external power and restore/resume where supported.
- Approximately 5%: consider graceful shutdown to protect the device.

These thresholds are provisional. Battery reporting, charger detection,
checkpoint reliability, and safe shutdown behavior must be tested on actual
hardware before release.

See `LOW_BATTERY_PROTECTION.md` for details.

## 11. Technical direction

Initial development uses:

- C++20
- CMake
- SDL2
- Git

The intended architecture separates shared application logic from
device-specific integration.

The temporary Arch Linux development harness is used to build and test
shared logic and simulate input. It is not a supported desktop product.

The handheld implementation must integrate with the actual Retro Pixel
Pocket firmware and EmuELEC environment after inspection.

Do not assume SDL2, graphics acceleration, audio, video decoding, battery
reporting, input mappings, or emulator launch mechanisms are available or
compatible until verified on the device.

The 1 GB RAM constraint requires careful control of memory use, background
services, media previews, and other concurrent work.

## 12. Hardware verification gate

Before committing to device-specific implementation, inspect the actual
handheld and verify:

- CPU architecture and userspace ABI.
- EmuELEC version and modifications.
- Kernel version and device tree.
- Available SDL version and libraries.
- GPU driver and hardware acceleration.
- Display orientation and scaling.
- Physical button mappings.
- Battery reporting and charger detection.
- Video decoding support.
- Existing frontend and emulator launch mechanisms.
- Available storage and filesystem layout.

No firmware flashing, repartitioning, or destructive storage operation is
authorized without explicit approval.

See `HARDWARE.md` for the verification checklist.

## 13. Development principles

- Handheld-first design.
- 720 × 720 logical interface.
- Physical controls before mouse or keyboard interaction.
- Preserve approved UI decisions.
- Keep the child experience simple and predictable.
- Keep parent management separate from child gameplay.
- Preserve installed games and saved progress.
- Prefer bounded, failure-tolerant operations.
- Verify hardware behavior rather than relying on assumptions.
- Document meaningful decisions and test results.
- No destructive device operations without explicit approval.

## 14. Decision status

The following are established product decisions:

- Retro Pixel Pocket is the sole initial target.
- Kid Mode is the default and only child-facing gaming experience.
- Parent Center is for management, not adult gameplay.
- The interface is designed around a 720 × 720 display and physical controls.
- The game carousel and large horizontal game-action design are established.
- Hiding games preserves their files and saved data.
- Automatic checkpoints are distinct from native in-game saves.
- Media retrieval is part of enabling a game, not a full-library background
  scrape.
- The Arch Linux desktop build is only a temporary development harness.

The following remain provisional or hardware-dependent:

- Exact device specifications until inspected.
- EmuELEC integration and available system libraries.
- Emulator launch and input interception mechanisms.
- Save-state support for each emulator.
- Battery thresholds and charging behavior.
- Video decoding and preview performance.
- Media provider selection, licensing, and API access.
- Future playtime limits and scheduling.

This specification should be updated when product decisions change.
Supporting documents should be kept consistent with it.
