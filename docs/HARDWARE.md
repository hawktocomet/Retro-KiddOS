# Hardware Target

## Device

FunnyPlaying Retro Pixel Pocket.

Inspected device:
- Firmware: v1.1.3
- OS release: RetroPixel 20251127
- Platform: CoreELEC / AmberELEC-derived firmware
- Architecture: AArch64
- Kernel: Linux 4.4.189
- CPU: Rockchip RK3326, quad-core
- Reported RAM: approximately 926 MiB
- Display: 720 × 720

## Existing firmware infrastructure

KiddOS reuses the device's existing:
- Linux firmware and startup process
- SDL libraries
- RetroArch and emulator cores
- ROM storage and game files
- Physical controller input

The current frontend is a standalone SDL application. It is launched
through a firmware startup hook that temporarily masks EmulationStation.

## Validation

The device-first frontend was tested successfully on the blue device
on November 16, 2026.

Verified:
- Direct startup into KiddOS
- Game navigation and artwork
- Game launching through the firmware
- In-game controls
- Select + Start return to KiddOS
- System menu and shutdown confirmation
- Device shutdown

This validation applies to the tested blue device and its current
firmware configuration. It does not establish that installation is
portable to other devices.

## Safety

Do not flash firmware, repartition storage, or modify the original
startup configuration without an explicit backup and recovery plan.
