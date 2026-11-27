# Retro KiddOS Milestone — November 27, 2026

## Summary

Completed improvements to shared media-library slots, music browsing and playback, and TV-show fullscreen presentation. The separate ALSA `dmix` audio migration remains outside this checkpoint for its own review and commit.

These changes support the Christmas 2026 goal: children should be able to browse and enjoy curated games and media with minimal adult help.

## Guiding principle

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

## Shared library slot resolution

- Added shared `KiddosLibrary::resolveSlots` logic for resolving four configured slots against discovered items.
- Updated Game, Movie, Music, and TV Shows catalog adapters to use the common resolver.
- Preserved the existing slot configuration representation and relative-path matching behavior.
- Empty and missing slot entries remain empty.
- Added tests covering all four catalog types, including empty and missing entries.

## Music browsing and playback

- Music browsing now presents configured playlist slots, followed by tracks within the selected playlist.
- Track navigation and automatic advancement stay within the active playlist and wrap to its first track after the last track.
- Now Playing presents playlist and track metadata.
- Playlist artwork is preferred when available, with track media as the fallback.
- Music can be paused and resumed across other media playback handoffs.
- Now Playing metadata is restored by matching the active music file path, avoiding stale playlist or track indices after catalog reloads.

## TV-show fullscreen presentation

- Added MPV's `--video-aspect-override=1:1` option to the TV-show launcher.
- The user tested the change on-device and confirmed TV playback now fits the full screen.

## Verification

- `device/working-ui/tests/run-library-data-tests.sh` passed all 24 tests.
- `device/working-ui/build-kiddos-ui.sh` completed successfully.
- TV fullscreen behavior was tested on-device and confirmed working.
- Music metadata restoration after a movie handoff was tested and confirmed working.
- Playlist looping and pause/resume behavior were checked during implementation review.

## Deliberately separate work

The ALSA `dmix` migration is not part of this milestone. Its configuration, launcher audio-device changes, and dedicated audio deployment/rollback workflow are being preserved for a separate checkpoint.

## Result

This checkpoint consolidates shared slot resolution, the improved playlist-based music experience, and the TV fullscreen correction without mixing in the independent audio-backend migration.
