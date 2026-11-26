# Retro KiddOS — Native Parent Center Milestone

**Milestone date:** November 25, 2026
**Status:** Working implementation verified on the blue Retro Pixel Pocket

## Summary

Retro KiddOS now has a native C++ filesystem and library-data implementation for browsing and managing the content assigned to the four home-screen slots in each supported category.

The SD card is the source of truth for games and media. KiddOS discovers eligible content from the card and persists slot references rather than maintaining a separate copy of the library catalog.

The Parent Center's media-management workflow operates locally on the device. It does not require a network connection or a remote management server.

## Parent Center and library behavior

There are four independently managed categories:

- **Games:** individual games with supported artwork.
- **Movies:** individual movies with supported artwork.
- **Music Playlists:** playlists with artwork; each playlist contains its own audio files.
- **TV Shows:** shows with artwork; each show contains its own seasons and episodes.

Each category has four fixed home-screen slots.

The implementation supports:

- Discovering eligible library items from the SD card.
- Requiring supported artwork for an item to appear in the assignable library.
- Assigning an item to an empty slot.
- Replacing or moving an assignment.
- Preventing the same item from occupying multiple slots in the same category.
- Clearing a slot without shifting the other assignments.
- Keeping empty slots in their original positions.
- Persisting slot references and reloading them independently of a cached catalog.
- Rejecting unsafe references, including absolute paths and path traversal.

Music playlists and TV shows are managed as containers. Individual tracks and episodes are not separate Parent Center assignments.

## Data contract

The SD-card layout and artwork conventions are documented in:

`docs/SD-CARD-DATA-CONTRACT-v1.md`

The device-side slot configuration is stored at:

`/storage/.config/kiddos/slots.conf`

The current format uses the `KIDDOS_SLOTS_V1` header followed by the slot references.

The SD card remains the source of truth. The slot file stores references to content; it is not a replacement library database.

## Application architecture

The active frontend is a native C++ application using SDL2, SDL_image, and SDL_ttf.

Native services currently handle:

- Game, movie, music playlist, and TV show library discovery.
- Artwork eligibility and library data.
- Slot persistence.
- Game and media launching.

Playback delegates to the device's existing RetroArch and MPV installations.

The Parent Center's media workflow is native and offline. However, this milestone does **not** represent the removal of every legacy Python or remote-management component:

- Legacy Remote Management controls and their service are still present in the frontend.
- The Wi-Fi service still invokes `wifi_helper.py` through Python.
- Legacy management-server and Python catalog code has not yet been removed from the repository.

These are known cleanup targets, not requirements of the native media-management implementation.

## Artwork refresh behavior

The application reloads its library data when the slot configuration changes.

An artwork file changed in place may not be detected immediately by the current refresh mechanism. During device testing, replacing an invalid zero-byte TV-show `poster.jpg` with a valid image required touching `slots.conf` to trigger a reload.

Independent artwork-change detection remains a future improvement.

## Verification

The following checks passed during this milestone:

1. `./device/working-ui/run-library-data-tests.sh`
2. `./device/working-ui/run-architecture-tests.sh`
3. `cmake --build build-working-ui -j2`
4. ARM64 build using `build-kiddos-ui.sh`
5. Deployment to the approved blue device.
6. Application launch and on-device library/Parent Center testing.

The library-data tests cover artwork eligibility, supported file types, playlists and TV-show containers, hidden directories, slot assignment and movement, duplicate prevention, unsafe references, persistence, reload, and clearing slots.

The architecture tests cover input mappings, navigation, system menu behavior, selection behavior, and PIN storage and workflow.

### Deployed executable

The verified ARM64 executable SHA-256 at the time of deployment was:

`47673a8e03dac8c508bdc95b0a80c4be539e87ac6ced260f136a05f260fbb722`

The deployment target was the blue device at `root@192.168.0.89`.

This checksum identifies that deployed build; it is not a promise that later builds will have the same checksum.

## Known follow-up work

The next work is deliberately separate from this completed milestone:

1. Update the project documentation to describe the current implementation.
2. Preserve this working state in Git and push it to GitHub.
3. Audit and remove obsolete Remote Management and Python catalog/server code without breaking Wi-Fi functionality.
4. Refactor and simplify the native application architecture.
5. Refine the user interface.
6. Develop an Arch Linux SD-card preparation tool, including content and artwork scraping, that produces data conforming to the SD-card contract.

The Arch-side preparation tool is planned work; it is not part of this milestone.

## Project principle

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.
