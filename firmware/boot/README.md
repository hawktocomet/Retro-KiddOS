# Retro KiddOS Firmware Boot Integration

This directory contains the startup hook tested on the blue FunnyPlaying
Retro Pixel Pocket on November 16, 2026.

## How it works

The firmware invokes `custom_start.sh` with the `before` argument.

The hook:

1. Loads the firmware environment from `/etc/profile`.
2. Logs startup activity to `/storage/.config/kiddos-boot.log`.
3. Checks for the KiddOS executable.
4. Masks `emustation.service` for the current boot.
5. Starts KiddOS as a transient systemd service named `kiddos-ui`.

The service uses `Restart=on-failure` and a two-second restart delay.

If the executable is missing or EmulationStation cannot be masked, the hook
leaves normal startup unchanged. If the initial service launch fails, it
unmasks EmulationStation.

## Current device-specific configuration

The tested executable path is:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

The hook is currently specific to the tested blue device configuration.
Do not assume it is ready to install on another device.

## Logging

Startup messages are appended to:

`/storage/.config/kiddos-boot.log`

## Recovery and limitations

This hook has been validated on the blue device only.

The current failure handling covers a missing executable, a failed service
launch, and failure to mask EmulationStation. It does not automatically
restore EmulationStation if KiddOS later exits or repeatedly crashes.

Before installing an updated hook, preserve the device's original startup
script. Recovery requires restoring that original script and removing any
runtime service mask as appropriate.

No general installation or rollback procedure is provided yet.

## Verification

The tested hook's SHA-256 is:

`0433c8f4a9a2bcccc1d60f7995a3c316305e2f7bdadb6bf8718a8a2270bd4980`
