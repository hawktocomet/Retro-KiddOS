# Retro KiddOS

A simple, child-friendly gaming and entertainment frontend for the FunnyPlaying Retro Pixel Pocket.

## Mission

Make a small, personal, offline entertainment device that children can use independently at home and while traveling. Family multiplayer is an important part of the gaming goal.

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

Prefer using playback and system capabilities already present in the firmware over building custom infrastructure. Do not add features just because they are technically interesting.

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

The [Native Parent Center Milestone](docs/MILESTONE-NATIVE-PARENT-CENTER-2026-11-26.md) records the current implementation, verification, known limitations, and next steps.

Verified during this milestone:

- Library-data tests passed.
- Architecture tests passed.
- Desktop CMake build passed.
- ARM64 build passed.
- Deployment and application launch were verified on the blue Retro Pixel Pocket (RPPBLUE).

The deployed ARM64 executable SHA-256 at that milestone was:

`47673a8e03dac8c508bdc95b0a80c4be539e87ac6ced260f136a05f260fbb722`

This checksum identifies that particular deployed build; subsequent builds will have different checksums.

The blue device is the currently verified deployment target. Purple and green devices have not been approved or validated as deployment targets.

## Build, test, and deployment

The active frontend's build instructions, prerequisites, ARM64 build procedure, deployment procedure, backup, and rollback information are documented in [`device/working-ui/README.md`](device/working-ui/README.md).

The repository includes these test scripts:

```bash
./device/working-ui/run-library-data-tests.sh
./device/working-ui/run-architecture-tests.sh
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
3. Develop an Arch Linux SD-card preparation tool that organizes content and retrieves suitable metadata and artwork according to the SD-card data contract.
4. Investigate practical multiplayer workflows, including Pokémon trading and head-to-head games.

The Arch-side preparation tool and scraper are planned work, not current repository features.

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
