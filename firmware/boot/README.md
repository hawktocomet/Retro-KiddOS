# KiddOS Boot Integration

## Early startup

KiddOS uses an early systemd launcher to start the native UI as soon as the
firmware's hardware services and required devices are ready.

Repository files:

- `kiddos-early-start.sh` — waits for hardware readiness and starts KiddOS.
- `kiddos-early.service` — starts the asynchronous early-launch waiter during
  boot.

Installed device paths:

- `/storage/.config/kiddos-early-start.sh`
- `/storage/.config/system.d/kiddos-early.service`

The early launcher waits for:

- The `retropixel-serv` process
- `/dev/input/js0`
- `/dev/dri/card0`
- The KiddOS executable

It waits for up to 45 seconds. If the required conditions are not met, it
exits and leaves the later firmware startup hook to handle the launch.

Before launching KiddOS, the early launcher checks whether `kiddos-ui.service`
is already active. It masks `emustation.service` at runtime and starts KiddOS
as a transient systemd service with automatic restart on failure.

## Later startup hook

The firmware's later startup hook remains a fallback:

`/storage/.config/custom_start.sh`

It also starts the hardware service and avoids launching a duplicate KiddOS
instance if the early launcher has already started it.

**Do not overwrite the installed `custom_start.sh` with the repository copy
without first reconciling its device-specific hardware-service logic.**

The installed hook contains device-specific changes that are not necessarily
present in the repository version.

## Deployment

Running:

`device/working-ui/deploy-kiddos-ui.sh deploy`

stages and checksum-verifies both early-start files, backs up any existing
copies, installs them, and reloads systemd's unit definitions.

Deployment preserves the early service's existing enabled/disabled state. It
does not enable, disable, or restart the early service. The updated launcher
and unit take effect on the next boot.

If a later deployment step fails, the deployment transaction restores the
previous early-start files. If those files did not previously exist, rollback
removes the newly installed copies.

The deployment does not modify the installed `custom_start.sh` or the
early-service enablement symlink:

`/storage/.config/system.d/multi-user.target.wants/kiddos-early.service`

## Manual rollback

The deployment script accepts optional backup arguments for the early-start
script and systemd unit.

Use `-` to restore a file to its previously absent state. Use
`__UNCHANGED__` to leave an optional component untouched.

To restore the early-start files while leaving the GB, GBC, controller profiles, and Wi-Fi helper unchanged, use the following command. Note that it also restores the executable and MPV config and then relaunches KiddOS.

```sh
deploy-kiddos-ui.sh rollback EXECUTABLE_BACKUP MPV_CONFIG_BACKUP __UNCHANGED__ __UNCHANGED__ __UNCHANGED__ __UNCHANGED__ EARLY_SCRIPT_BACKUP EARLY_UNIT_BACKUP
```

Replace each backup placeholder with its exact device backup path.

The complete argument order is documented in the deployment script's usage
text. Run rollback only when you intend to restore the corresponding backups.

## Device scope and recovery

The early-start integration has been validated on the blue device only.

Do not assume it is ready to install on another device without checking that
device's firmware paths, hardware readiness signals, and startup configuration.

Keep the legacy KiddOS runtime and the installed startup hook intact as
recovery paths.

## Logging

Early-launch and later-hook messages are appended to:

`/storage/.config/kiddos-boot.log`
