# Retro Pixel Pocket Hardware Service Migration

**Status:** Deployed and reboot-verified on the blue Retro Pixel Pocket (RPPBLUE)
**Date:** 2026-11-20
**Scope:** Replace the proprietary `retropixel-server` runtime with the KiddOS-owned ARM64 hardware service.

## Summary

The device now runs `/storage/.config/kiddos/retropixel-server --apply` as the transient systemd unit `kiddos-hardware.service`. The replacement is launched by the writable `/storage/.config/custom_start.sh` boot hook. The read-only system partition and `/usr/bin/autostart.sh` were not modified.

The existing KiddOS UI startup logic in `custom_start.sh` was preserved. The hook starts the hardware service before checking whether the UI is already running, and avoids launching a duplicate UI instance.

## Source layout

- `main.cpp` — joystick event loop and brightness control
- `VolumeController.cpp/.hpp` — GPIO volume-button handling and ALSA mixer control
- `BrightnessPolicy.hpp` — brightness step/clamping policy
- `VolumePolicy.hpp` — logical volume and mixer conversion policy
- `BrightnessPolicy_test.cpp` — brightness policy tests
- `VolumePolicy_test.cpp` — volume policy and mixer conversion tests
- `Makefile` — reproducible native build and policy tests

## Build and test

Requires a C++17 compiler and GNU Make.

From the repository root:

```sh
make -C device/hardware-service
make -C device/hardware-service test
```

The default build creates `device/hardware-service/build/retropixel-server`. The test target builds and runs both policy test executables. `make clean` removes generated build output.

The default executable mode is **observe-only**. It reports proposed brightness and volume changes without writing them. Pass `--apply` only when intentionally controlling device hardware.

## Runtime behavior

The service:

- Reads joystick button events from `/dev/input/js0`.
- Adjusts brightness through `/sys/class/backlight/backlight/brightness`, in steps of 10, bounded from 1 through the lesser of the device maximum and 99.
- Reads volume buttons from GPIO 123 (up) and GPIO 116 (down), with four-sample input stabilization.
- Reads and writes ALSA mixer control `numid=3` using `/usr/bin/amixer -c 0`.
- Maps logical volume 0–100 to mixer values 119–237, in logical steps of 5.
- Handles SIGINT/SIGTERM and closes its open input descriptors on shutdown.
- Exports GPIOs 116 and 123 when they are not already exported; it does not unexport them at shutdown.

The service defaults to observe-only mode. The device boot hook explicitly supplies `--apply`.

## Intentional differences from the legacy daemon

The replacement intentionally **does not automatically suspend** after inactivity. Automatic suspend in the legacy daemon was disruptive during use and diagnosis.

The replacement also **does not force poweroff when battery capacity reaches 0%**. This avoids an abrupt shutdown based on a potentially inaccurate battery reading.

Battery capacity/status polling and the legacy low-battery LED behavior are not implemented in this replacement. The new service focuses on brightness, physical volume controls, and joystick input. Do not assume battery-management behavior was migrated.

## Device deployment record

Device: `RPPBLUE`
Address used during deployment: `192.168.0.89`

Installed executable:

`/storage/.config/kiddos/retropixel-server`

Boot hook:

`/storage/.config/custom_start.sh`

Systemd transient unit:

`kiddos-hardware.service`

The boot hook identifies the legacy process by its `/proc/<pid>/exe` target and sends it SIGTERM before starting the replacement. It verifies that the old process has stopped; if it remains, the replacement is not started. The new unit is launched with `Restart=always` and `RestartSec=2`.

The original daemon remains on the read-only system at `/usr/bin/retropixel-server` and was also backed up to:

`/storage/.config/retropixel-server.original`

The original custom startup hook was backed up to:

`/storage/.config/custom_start.sh.pre-hardware-migration`

These backups were retained for rollback. Neither the original daemon nor the original startup script was deleted.

## Validation performed

On the blue device:

- Confirmed the replacement executable is ARM64 and installed at the path above.
- Ran the service in observe-only mode and verified it could open the joystick, initialize the volume controller, and report brightness and mixer state without writing hardware settings.
- Tested physical brightness controls in apply mode; brightness changed in the expected 10-point steps and remained within bounds.
- Tested physical volume up/down; logical volume changed by 5 and the mixer values changed accordingly.
- Confirmed GPIO 116 and GPIO 123 could be read, and checked battery/LED state during migration.
- Rebooted and confirmed both `kiddos-hardware.service` and `kiddos-ui.service` were active.
- Confirmed the hardware service had zero systemd restarts after reboot.
- Checked `/proc` and found no running process whose executable was the legacy `/usr/bin/retropixel-server`.
- Confirmed the immutable `/usr/bin/autostart.sh` and read-only system partition were left untouched.

The device-side replacement binary SHA-256 at deployment:

`a1103d923b952f3a87e9aae54a0faee6f0c7ee8b6d98cd5af6d1e6d6fb565f04`

The original legacy binary SHA-256:

`efc547020d97787133bd4fbe1c0f80558bb7413e04206b71e5653a54873c8d3c`

## Rollback

Rollback is a device operation and should be performed deliberately.

1. Stop the replacement unit: `systemctl stop kiddos-hardware.service`.
2. Restore the saved startup hook: copy `/storage/.config/custom_start.sh.pre-hardware-migration` back to `/storage/.config/custom_start.sh`.
3. Reboot the device so the stock autostart path can launch the original daemon again.

The original daemon was not removed from `/usr/bin`. Do not delete the retained backups unless a later, separately verified cleanup is intended.

## Current deployment boundary

This migration was deployed and verified on the blue device only. A second device was not available for validation. The repository source and build are portable C++17, but GPIO numbering, mixer control IDs, input paths, and sysfs paths are hardware/firmware-specific and should be verified before deployment to another model or firmware.
