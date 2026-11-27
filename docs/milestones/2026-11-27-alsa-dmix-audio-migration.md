# ALSA dmix Audio Migration

Date: 2026-11-27

## Objective

Move KiddOS music, movie, and TV-show playback onto a shared ALSA `dmix`
device so the media launchers use a consistent audio configuration.

## Implementation

- Added `device/working-ui/alsa-dmix.conf`, which includes the system ALSA
  configuration and sets the dmix format to `S16_LE`.
- Updated the MPV launch arguments in the music, movie, and TV-show launchers
  to use `alsa/dmix`, signed 16-bit audio, 48 kHz, and stereo output.
- Set `ALSA_CONFIG_PATH` in each MPV child process to
  `/storage/.config/kiddos/alsa-dmix.conf`.
- Preserved the TV-show square video aspect override.

## Music process replacement

Music replacement now starts the new MPV child in a stopped state. The parent
waits until the child confirms that it is stopped, terminates and reaps the
previous music process, and then resumes the replacement process.

This ordering prevents the replacement player from acquiring the audio device
before the previous player has been stopped.

The launcher retains pause/resume behavior and reaps completed music children.

## Deployment support

Added an `audio-deploy` command to `device/working-ui/deploy-kiddos-ui.sh`.

The audio deployment:
- Transfers the built ARM64 executable and ALSA configuration.
- Verifies both staged files using SHA-256 checksums.
- Backs up the existing executable and ALSA configuration.
- Stops KiddOS before replacing either file.
- Installs the files and restarts KiddOS.
- Attempts to restore the previous files and relaunch KiddOS if installation
  or startup fails.

Staging files are placed beside their respective final destinations so each
final `mv` can use a same-filesystem rename.

The deployment command was reviewed and its shell syntax was checked. It was
not run as part of this checkpoint.

## Validation

- ARM64 build completed successfully using
  `device/working-ui/build-kiddos-ui.sh`.
- The build emitted two existing `-Wfree-nonheap-object` warnings in
  `services/LibraryData.cpp`; no new ALSA-related compiler errors occurred.
- `device/working-ui/tests/run-library-data-tests.sh`: all 24 tests passed.
- `bash -n device/working-ui/deploy-kiddos-ui.sh`: passed.
- `git diff --check`: passed.

## Deployment status

This checkpoint records the implementation and local validation only.
The audio deployment has not been run, and runtime behavior with the new
ALSA configuration has not been verified on the Retro Pixel Pocket as part
of this checkpoint.

Do not treat the successful cross-build as proof of device-side audio
operation. Device verification remains a separate step requiring explicit
deployment approval.
