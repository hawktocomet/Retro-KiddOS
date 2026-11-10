# Architecture

## Product constraints

KiddOS is a handheld product for the FunnyPlaying Retro Pixel Pocket.

The interface targets a 720 × 720 logical display and physical controls.
The reported device has 1 GB of RAM, so memory use, background work, media
previews, and concurrent services must be tightly controlled.

The Arch Linux environment is a temporary development and testing host.
It is not a supported desktop product or a second deployment target.

## Architectural direction

Separate shared application logic from device-specific integration.

Shared logic may include:

- UI state and navigation.
- Game library and metadata.
- Parent access and PIN workflow.
- Game visibility and feature configuration.
- Game action and launch requests.
- Save/checkpoint policy.
- Media selection and metadata.
- Battery-protection policy.

Device-specific integration is responsible for connecting the application
to the actual Retro Pixel Pocket environment, including:

- Display and graphics.
- Physical input.
- Audio.
- Game and emulator launching.
- Save-state mechanisms.
- Battery and charger reporting.
- Video playback and decoding.
- Filesystem and storage behavior.

The exact boundaries must be informed by inspection of the target firmware.

## Initial technology

- C++20
- CMake
- SDL2
- Git

These are initial development choices, not a commitment to use SDL2 for
every hardware subsystem. Availability and compatibility on the target
device must be verified.

## Development harness

The initial Arch Linux build provides a temporary environment for:

- Building shared application code.
- Testing basic UI behavior.
- Simulating physical input.
- Running bounded development tests.

The harness is not a general desktop frontend. Desktop success does not
prove that a feature works on the handheld.

## Planned components

- UI and rendering.
- Physical input abstraction.
- Game library and metadata.
- Parental controls and PIN handling.
- Game launch integration.
- Save-state/checkpoint management.
- Media management.
- Battery and power monitoring.
- Retro Pixel Pocket / EmuELEC platform integration.

## Hardware verification gate

Before selecting device-specific implementation details, inspect and verify
the actual device's firmware, libraries, graphics stack, input mappings,
emulator integration, power reporting, video support, and storage layout.

See `HARDWARE.md` for the complete checklist.

No firmware flashing, repartitioning, or destructive storage operations are
authorized without explicit approval.
