# Retro KiddOS SD Card Data Contract v1

**Status:** Initial contract for the native, offline Parent Center
**Baseline reviewed:** `832790a1c8c5644e8b8bddc241f6536e32fb895e`

This document defines how KiddOS discovers parent-prepared content on the SD card. The SD card is the source of truth for media files, artwork, and folder organization. KiddOS must not copy media into an internal catalog or require a network service to browse or assign it.

## 1. General rules

- Content is discovered beneath the device ROM root (currently `/storage/roms`).
- Hidden files and folders (names beginning with `.`) are ignored.
- File-extension matching is case-insensitive.
- A discovered item must resolve to a regular file or directory inside the ROM root. Paths that resolve outside that root are rejected.
- Only items meeting the category's artwork and content requirements appear in the Parent Center's assignable library.
- Parent Center manages assignments, not files. Assigning, replacing, or unassigning an item must never delete or move SD-card content.
- Each category has four slots. An item may occupy at most one slot within its own category.
- Music playlists and TV shows are containers. Tracks and episodes are not independently assignable items.

## 2. Folder and artwork conventions

### Games

Games remain in their existing platform-specific folders directly under the ROM root. The folder and extension conventions currently defined by the catalog are:

| Platform folder | Accepted game extensions |
|---|---|
| `gb/` | `.gb`, `.zip` |
| `gbc/` | `.gbc`, `.zip` |
| `gba/` | `.gba`, `.zip` |
| `arcade/` | `.zip`, `.7z` |
| `snes/` | `.sfc`, `.smc`, `.zip` |
| `genesis/` or `megadrive/` | `.gen`, `.md`, `.bin`, `.smd`, `.zip` |
| `mastersystem/` | `.sms`, `.zip` |
| `nes/` | `.nes`, `.zip` |

Artwork is placed alongside the game file and has the same filename stem. Supported artwork extensions are `.png`, `.jpg`, and `.webp`; matching is case-insensitive. If more than one matching image exists, preference is PNG, then JPG, then WebP, with filename order used to break ties.

The existing catalog defines all eight platform families above, but its current initial-release enabled-system setting and slot-eligibility checks are narrower. Native implementation must preserve actual launcher support and must not make an unsupported game assignable merely because it was discovered.

Example:

```text
/storage/roms/
  gb/
    Tetris.gb
    Tetris.png
```

### Movies

- Directory: `movies/`
- Movies are individual files directly inside this directory; discovery is not recursive.
- Accepted extensions: `.mp4`, `.mkv`, `.avi`, `.mov`, `.m4v`.
- Each movie requires an adjacent image with the same filename stem.
- Accepted artwork extensions: `.png` and `.jpg`, case-insensitive.
- If both are present, PNG is preferred over JPG.

Example:

```text
/storage/roms/
  movies/
    The Movie.mp4
    The Movie.png
```

### Music playlists

- Directory: `music/`
- Each immediate child directory is one playlist.
- A playlist requires `cover.jpg` or `cover.png`; JPG is preferred when both exist.
- The playlist must contain at least one supported audio file. Tracks may be in the playlist directory or in nested subdirectories.
- Accepted audio extensions: `.mp3`, `.m4a`, `.aac`, `.flac`, `.ogg`, `.opus`, `.wav`, `.wma`.
- The playlist folder is the assignable item. Individual tracks are not shown as Parent Center items.

Example:

```text
/storage/roms/
  music/
    Favorite Songs/
      cover.jpg
      track-01.mp3
      track-02.mp3
```

### TV shows

- Directory: `tvshows/`
- Each immediate child directory is one show.
- The show folder is the assignable item. Individual seasons and episodes are not separate Parent Center items.
- A show requires one of these artwork files, in preference order: `poster.jpg`, `poster.png`, `cover.jpg`, `cover.png`.
- The show must contain at least one supported episode file. Episodes may be in nested subdirectories.
- Accepted episode extensions: `.mp4`, `.mkv`, `.avi`, `.mov`, `.m4v`, `.webm`.
- Season folders should use the naming convention `Season 01`, `Season 02`, and so on.
- Episode filenames must include a season and episode identifier in the format `SHOW NAME - S##E## - EPISODE NAME.ext`. For example: `Pokemon - S01E01 - Pokemon I Choose You.mp4`.
- Episodes play sequentially across the entire show: seasons in numeric order, with episodes in numeric order within each season. After the final episode, playback loops back to the first episode.
- The organizer must identify the `S##E##` season and episode numbers and flag missing or duplicate identifiers for parent review. It must not silently guess an episode's position.
- Season and episode ordering is numeric, not alphabetical. For example, Season 10 follows Season 9, and Episode 10 follows Episode 9.

Example:

```text
/storage/roms/
  tvshows/
    Pokemon/
      poster.jpg
      Season 01/
        Pokemon - S01E01 - Pokemon I Choose You.mp4
        Pokemon - S01E02 - Pokemon Emergency.mp4
      Season 02/
        Pokemon - S02E01 - Example Episode.mp4
```

## 3. Parent Center assignment behavior

Parent Center contains four slots in each category:

- Games
- Movies
- Music Playlists
- TV Shows

For each category, a parent can assign an eligible item to an empty slot, replace an occupied slot, or clear a slot. An item already assigned in that category is not offered as another available item; assigning it to a different slot moves its assignment. Clearing a slot only clears the assignment.

Items without required artwork, or without the required media content (a track for a playlist or an episode for a show), are not eligible for assignment.

## 4. Slot persistence and item references

The native implementation stores only the four slot assignments per category in a small local configuration. It does not persist a second copy of the discovered catalog, media metadata, artwork, or media files.

Assignments refer to content by its path relative to the ROM root, not by an absolute mount path. The native reader resolves and validates that relative path against the current ROM root before use. If an assigned item is missing, no longer eligible, or no longer resolves inside the ROM root, its slot is treated as unavailable; the underlying files are never modified.

The precise on-disk configuration filename and serialization are implementation details of the native slot manager and must be versioned. They must not become a second source of truth for content inventory.

## 5. Offline and implementation boundary

The native C++ application is responsible for filesystem discovery, artwork eligibility, slot assignment persistence, and passing selected content to the existing launch/playback paths.

The finished Parent Center must work without Wi-Fi, HTTP, a Python process, the management server, or Remote Management. Existing Python catalog/server code may remain temporarily during replacement and verification, but it is not part of the target architecture and must not receive new features.

## 6. Compatibility note

This contract formalizes the folder and artwork conventions found in the existing catalog at the baseline commit above. Where current native scanners disagree with these rules (notably movie artwork filtering and playlist-level music discovery), the contract is authoritative for the native replacement.
