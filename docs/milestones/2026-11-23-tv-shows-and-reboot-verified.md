# Retro KiddOS Milestone — November 23, 2026

## Summary

Completed and verified the first TV Shows browsing and playback implementation for Retro KiddOS. The latest tested build was deployed to the blue Retro Pixel Pocket, configured to launch automatically at boot, and verified after a real reboot.

The implementation and supporting changes were committed and pushed to GitHub.

This milestone advances the Christmas 2026 goal: a reliable, child-friendly device experience for browsing and playing curated games and media.

## Guiding principle

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

The Foundation remains a longer-term reusable platform. Completing it is not a prerequisite for delivering the Christmas KiddOS experience.

## TV Shows implementation

### Browsing

- Added a dedicated TV Shows destination to the KiddOS navigation.
- Added a 2×2 TV Shows grid, following the visual approach used by Movies.
- The first configured show is Teenage Mutant Ninja Turtles (1987).
- The screen supports empty slots/placeholders for additional shows.
- Only immediate child directories of `/storage/roms/tvshows` are treated as shows.
- Episode discovery is recursive within each show directory.
- Supported video extensions:
  - `.mp4`
  - `.mkv`
  - `.avi`
  - `.mov`
  - `.m4v`
  - `.webm`
- Episodes are naturally sorted.
- Empty show directories are ignored.
- Show artwork can be discovered from `poster.jpg`, `poster.png`, `cover.jpg`, or `cover.png`.

### Playback and resume

- Selecting a show starts its first episode when no saved progress exists.
- If progress exists, playback resumes from the saved episode and position.
- MPV's watch-later mechanism preserves playback position.
- Each show has its own saved episode index.
- Episodes advance automatically through the playlist.
- The launcher waits for the selected episode to be loaded and seekable before saving its index and shutting down MPV. This addresses a race encountered when switching episodes and exiting quickly.

### TV playback controls

- A: Pause/resume.
- D-pad left/right: Seek backward/forward by 10 seconds.
- L shoulder: Previous episode.
- R shoulder: Next episode.
- Start + Select twice within 1500 ms: Save progress and return to the TV Shows grid.

The L/R shoulder behavior is specific to TV playback. Menu music navigation remains unchanged.

### MPV behavior

- Added the media-title OSD message for four seconds when a media item loads.
- Playback uses the shared `mpv-input.conf`.
- The down-button action is ignored rather than quitting playback.
- The TV launcher uses MPV IPC to inspect playlist position and wait for the selected episode to become ready.
- Shutdown first requests a graceful MPV quit, then escalates to SIGTERM and SIGKILL only if necessary.

## Device content used for testing

The device contained:

`/storage/roms/tvshows/Teenage Mutant Ninja Turtles (1987)/Season 01/`

Five MKV episodes were present in that season directory.

## Playback and resume testing

The following behavior was tested on the device:

- TV Shows grid navigation.
- Starting a show.
- Episode playback.
- Pause/resume.
- Seeking.
- Previous/next episode controls.
- Automatic episode advancement.
- Saving and restoring playback progress.
- Returning to the TV Shows grid using the exit chord.

A resume race was identified when switching backward to an episode, watching briefly, and exiting. The launcher was updated to wait until MPV reports the selected episode path and seekable state before saving the episode index and quitting.

The user subsequently confirmed that the behavior worked correctly.

## Build and deployment

Build script:

`device/working-ui/build-kiddos-ui.sh`

Build output:

`Build-Workspace/build/arm64/kiddos-ui`

Deployment script:

`device/working-ui/deploy-kiddos-ui.sh`

The deployment script transfers the executable and MPV configuration, verifies the checksum, backs up the existing target and configuration, restarts the service, and verifies that the service is active. It includes rollback handling for deployment failures.

### Tested candidate

Device:

`root@192.168.0.89` — RPPBLUE

Deployed executable:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

Build ID:

`cbba28c092f591216554b1528192e948565349a1`

SHA-256:

`f5ffd2a3207f943d4c4952348c078e00ddbbeed2588e9cb199f3379fd3cb68e2`

The user deployed and tested this build and reported that it worked perfectly.

## Permanent boot configuration

The device's two startup scripts previously referenced an older executable path:

- `/storage/.config/custom_start.sh`
- `/storage/.config/kiddos-early-start.sh`

Both were updated to launch the tested candidate:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

The changes were explicitly authorized.

Backups were retained on the device:

- `/storage/.config/kiddos-early-start.sh.pre-latest-ui-20261123-110325`
- `/storage/.config/custom_start.sh.pre-latest-ui-20261123-110325`

Verification performed:

- Confirmed the candidate executable existed.
- Confirmed its SHA-256 matched the tested build.
- Ran `sh -n` against both modified startup scripts.
- Confirmed both scripts referenced the new path.
- Confirmed the old path was no longer referenced.

### Reboot verification

The device was rebooted manually.

After reboot, the running executable was verified as:

`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

The running process executable's SHA-256 matched:

`f5ffd2a3207f943d4c4952348c078e00ddbbeed2588e9cb199f3379fd3cb68e2`

The startup logs confirmed:

- The early launcher ran.
- The KiddOS systemd service launch was requested.
- The hardware service was launched.
- The regular boot hook detected that KiddOS was already running and skipped a duplicate launch.

**Result:** The latest tested KiddOS build starts automatically after reboot. This was verified on the physical device.

## Source files included in the implementation commit

The following 15 files were committed:

- `device/working-ui/Application.cpp`
- `device/working-ui/Application.hpp`
- `device/working-ui/Navigation.hpp`
- `device/working-ui/UI.cpp`
- `device/working-ui/UI.hpp`
- `device/working-ui/build-kiddos-ui.sh`
- `device/working-ui/models/Models.hpp`
- `device/working-ui/mpv-input.conf`
- `device/working-ui/screens/ScreenState.hpp`
- `device/working-ui/screens/TVShowScreens.cpp`
- `device/working-ui/services/MovieLauncher.cpp`
- `device/working-ui/services/TVShowCatalog.cpp`
- `device/working-ui/services/TVShowCatalog.hpp`
- `device/working-ui/services/TVShowLauncher.cpp`
- `device/working-ui/services/TVShowLauncher.hpp`

The implementation adds 780 lines and removes 5 lines across these 15 files.

## Git milestone

Repository:

`https://github.com/hawktocomet/Retro-KiddOS`

Branch: `main`

Commit:

`3b13c7e` — `Add TV show browsing and playback`

The commit was pushed successfully to `origin/main`. At verification, local `HEAD` and `origin/main` both pointed to `3b13c7e`.

The following unrelated, untracked directories were deliberately left untouched and excluded from the commit:

- `build-working-ui/`
- `firmware/boot/stock-retropixel-v1.1.3/`
- `firmware/kernel/`

The device startup-script changes were made directly on the device and are not part of the 15-file Git commit.

## Result and next direction

This milestone establishes:

1. A working TV Shows browsing and playback path.
2. Per-show episode progress and playback-position resume.
3. Working playback controls and episode navigation.
4. A tested deployment on the physical device.
5. Automatic startup of the tested build after reboot.
6. A reviewed implementation committed and pushed to GitHub.

The next work should continue toward the Christmas experience: curated games, music, movies, and TV shows that children can browse and use with minimal adult assistance.

Avoid expanding infrastructure or custom software unless it directly improves that experience.
