# Architecture

## Initial direction

KiddOS will use a shared application core with platform-specific adapters.

The desktop build is a development and testing environment. The handheld
build will integrate with the actual EmuELEC environment after inspection.

## Initial technology

- C++20
- CMake
- SDL2
- Git

These are initial development choices, not a commitment to use SDL2 for every
hardware subsystem.

## Planned components

- User interface and rendering
- Physical input abstraction
- Game library and metadata
- Parental controls and PIN handling
- Game launch integration
- Save-state/checkpoint management
- Media management
- Battery and power monitoring
- Desktop platform adapter
- EmuELEC platform adapter

## Constraints

The target has 1 GB of RAM. Background work, media previews, and application
services must be designed with memory and CPU use in mind.

Hardware integration decisions require inspection and testing on the device.
