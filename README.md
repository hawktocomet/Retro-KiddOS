# Retro KiddOS

A simple, child-friendly gaming and entertainment frontend for the FunnyPlaying Retro Pixel Pocket.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling. Family multiplayer is an important part of the gaming goal.

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

## Preparing your SD card

Retro KiddOS reads games and media from the SD card. The card's `roms/` directory is the source of truth; you organize the files there, and KiddOS discovers them.

The expected layout is:

```text
roms/
├── gb/
├── gbc/
├── gba/
├── arcade/
├── snes/
├── genesis/
├── mastersystem/
├── nes/
├── movies/
│   ├── Movie Name.mp4
│   └── Movie Name.png
├── music/
│   └── Playlist Name/
│       ├── cover.jpg
│       └── Track 01.mp3
└── tvshows/
    └── Show Name/
        ├── poster.jpg
        ├── Season 01/
        │   └── Show Name - S01E01 - Episode Name.mp4
        └── Season 02/
            └── Show Name - S02E01 - Episode Name.mp4
```

### Artwork naming

| Content | Artwork filename |
|---|---|
| Games | Same filename stem as the game; `.png`, `.jpg`, or `.webp` |
| Movies | Same filename stem as the movie; `.png` or `.jpg` |
| Music playlists | `cover.jpg` or `cover.png` inside the playlist folder |
| TV shows | `poster.jpg`, `poster.png`, `cover.jpg`, or `cover.png` inside the show folder |

Movies and games have their artwork beside the content file. Music and TV artwork belongs inside the playlist or show folder.

### Media organization

- Movie files go directly inside `movies/`.
- Each immediate subfolder of `music/` is one playlist. Put its audio tracks inside that folder; nested track folders are allowed.
- Each immediate subfolder of `tvshows/` is one show. Put episodes inside season folders.
- The SD-card organizer will standardize season folder names (`Season 01`, `Season 02`, etc.) and episode filenames during import. It will rename only the SD-card copies, not the parent's original files.
- Episode filenames use `SHOW NAME - S##E## - EPISODE NAME.ext`, such as `Pokemon - S01E01 - Pokemon I Choose You.mp4`.
- The organizer should identify season and episode numbers where possible, flag uncertain files for the parent to resolve, and show the resulting episode order before copying.
- TV shows play sequentially in numeric season and episode order, resume from the saved position, and return to the library after the final episode. TV shows do not loop.
- File extensions are matched without regard to letter case. Hidden files and folders (names beginning with `.`) are ignored.

The folder conventions cover eight game platforms, but not every platform is currently enabled for Parent Center assignment. The current native library reader supports raw GB and GBC game files; consult the full contract for the precise conventions and current implementation boundaries.

See [SD Card Data Contract v1](docs/SD-CARD-DATA-CONTRACT-v1.md) for supported file extensions, artwork preferences, discovery rules, and slot behavior.

## Project direction

Retro KiddOS and Retro Pixel Foundation are two independent projects with separate responsibilities.

- **Retro KiddOS** owns the child-friendly experience: frontend, content library, navigation, parent-facing content management, and product-specific behavior.
- **Retro Pixel Foundation** is the reusable hardware and software platform: operating system, hardware support, system services, gaming runtime, reproducible image build, and documented application interfaces.

The Foundation must be independently buildable and useful without KiddOS. KiddOS must remain usable on existing compatible firmware while the Foundation is developed.

See [Project Boundaries](docs/PROJECT-BOUNDARIES.md) for ownership rules and transition responsibilities.

The immediate priority is a reliable, easy-to-use family entertainment experience. Foundation work must not become a prerequisite for KiddOS progress.

## Current implementation

The active native SDL application is in [`device/working-ui/`](device/working-ui/).

The frontend currently includes:

- A child-facing game carousel with four fixed game slots.
- Game launching through the device's existing RetroArch installation.
- Movies, music playlists, and TV shows as separate library categories.
- MPV-based movie, music, and TV playback.
- A hidden, PIN-protected Parent Center.
- Native, offline parent-side assignment of games, movies, music playlists, and TV shows to four fixed slots per category.
- Artwork-aware library discovery: only items with supported artwork are eligible for assignment.
- Slot assignment, replacement, movement, and clearing without compacting the remaining slots.
- Battery and charging status.
- Wi-Fi status and existing Wi-Fi configuration functionality.
- A system menu with shutdown confirmation.

Music playlists and TV shows are managed as containers. A playlist contains its audio files; a show contains its seasons and episodes. Individual tracks and episodes are not separately assigned to home-screen slots.

The Parent Center's media-management workflow is native C++. It works locally and does not require a network connection or a remote management server.

### Library and SD-card data

The SD card is the source of truth for games and media. KiddOS discovers content from the card and stores references to selected content rather than maintaining a separate library database.

The device-side slot configuration is:

`/storage/.config/kiddos/slots.conf`

The current format begins with `KIDDOS_SLOTS_V1`.

The data layout, supported artwork conventions, and slot-reference rules are documented in [SD Card Data Contract v1](docs/SD-CARD-DATA-CONTRACT-v1.md).

### Current implementation boundaries

The legacy browser Remote Management interface, its Python server/catalog, and the associated frontend service have been removed. The native Parent Center reads the SD-card library directly and works offline. The Wi-Fi workflow remains separate and continues to use `wifi_helper.py`.

Artwork changes made in place may not be detected until the application reloads its library data. Independent artwork-change detection is a known improvement.

## Latest milestone

The latest completed milestone is the [Media Playback Workflow extraction](docs/ARCHITECTURE.md#november-28-2026--media-playback-workflow-extraction).

Commit `20500fb` extracted movie and TV playback orchestration into a focused
service workflow. The five host-side test suites passed (118 checks), and the
ARM64 build completed without GCC warnings.

The build was deployed successfully to the blue Retro Pixel Pocket (RPPBLUE)
using the existing deployment script. The device executable SHA-256 matched
the local build:

`dc447d614793ba570b0efebb15f70405894617b0f7194f47aecb4affd7bc95e9`

On-device smoke tests passed for startup and controls, Parent Center PIN entry,
Movies and TV Shows navigation, movie and TV launch/return behavior, music
playback, and game launching. This validates the tested blue device
configuration; purple and green devices have not been validated as deployment
targets.

The [Native Parent Center Milestone](docs/MILESTONE-NATIVE-PARENT-CENTER-2026-11-26.md)
records the preceding implementation milestone.

## Build, test, and deployment

The active frontend's build instructions, prerequisites, ARM64 build procedure, deployment procedure, backup, and rollback information are documented in [`device/working-ui/README.md`](device/working-ui/README.md).

The repository includes five focused host-side test suites. Run them from
the repository root:

```bash
./device/working-ui/tests/run-architecture-tests.sh
./device/working-ui/tests/run-library-data-tests.sh
./device/working-ui/tests/run-pin-store-tests.sh
./device/working-ui/tests/run-music-workflow-tests.sh
./device/working-ui/tests/run-media-playback-workflow-tests.sh
```

A desktop CMake build can be run from an existing configured build directory:

```bash
cmake --build build-working-ui -j2
```

The ARM64 build uses an external Build-Workspace alongside the repository, or the location supplied through `KIDDOS_BUILD_WORKSPACE`. The workspace contains the cross-compilation dependencies, SDL sources, and device sysroot; it is not included in this repository.

Deployment is device-specific. Review the target and recovery procedure before deploying. Do not deploy to another handheld without explicit approval and a device-specific validation plan.

The tested firmware startup integration is under [`firmware/boot/`](firmware/boot/). It is specific to the inspected device setup and is not a general-purpose installer.

## Roadmap

The next priorities are:

1. Refactor and simplify the native application architecture.
2. Refine the child-facing and Parent Center interfaces.
3. Develop a small graphical SD-card preparation tool that organizes games and media, finds suitable metadata and artwork, and validates the card against the SD-card data contract.
4. Investigate practical multiplayer workflows, including Pokémon trading and head-to-head games.

The SD-card preparation tool is planned work, not a current repository feature. Initial development will target Linux, with cross-platform support for Windows and macOS where practical.

The longer-term Retro Pixel Foundation work remains separate from immediate KiddOS product development. See the [Product and Technical Roadmap](docs/PRODUCT-AND-TECHNICAL-ROADMAP.md) and [Custom Image Architecture](docs/CUSTOM-IMAGE-ARCHITECTURE.md).

## Repository layout

- `device/working-ui/` — active SDL frontend, native services, tests, and build/deployment scripts.
- `firmware/boot/` — inspected device startup integration and related artifacts.
- `docs/` — architecture, hardware, SD-card contract, project boundaries, and milestone documentation.

## Hardware

- FunnyPlaying Retro Pixel Pocket.
- 720 × 720 display.
- ARM64 Linux firmware.
- Existing RetroArch, emulator, SDL, and MPV infrastructure.

See [Hardware Notes](docs/HARDWARE.md) for the inspected device configuration.

## Distribution and licensing

The long-term Foundation direction is a transparent, reproducible platform with documented dependencies, source provenance, licenses, and redistribution restrictions.

Do not bundle ROMs, BIOS files, game content, media, artwork, or emulator cores without the appropriate rights. Any future distribution must preserve third-party licenses and notices and distinguish between software that can be built, software that can be redistributed, and content that must be supplied by users.

## License

GNU General Public License v3.0 or later. See [`LICENSE`](LICENSE).
