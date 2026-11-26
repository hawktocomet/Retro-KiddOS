# Retro KiddOS — Working UI

This directory contains the native C++/SDL2 frontend for Retro KiddOS on the FunnyPlaying Retro Pixel Pocket.

The application provides a child-friendly interface for launching games and playing locally stored movies, music playlists, and TV shows. It also includes an offline, on-device Parent Center for preparing the four content slots in each category.

## Current architecture

The frontend is a native C++ application. It reads the prepared media library directly from the SD card and uses a small configuration file to remember slot assignments.

The SD card is the source of truth for content. KiddOS stores slot references, not copies of the media library.

The current implementation includes:

- Four game slots, launched through RetroArch.
- Four movie slots, played through MPV.
- Four music playlist slots, played through MPV.
- Four TV show slots, played through MPV.
- Artwork-aware library discovery.
- An offline Parent Center for assigning, replacing, moving, and clearing slot contents.
- Empty slots that retain their positions; slots are never automatically compacted.
- Battery and Wi-Fi screens, plus system power controls.

Music slots represent playlists, not individual tracks. TV show slots represent shows, not individual episodes.

The Parent Center is native and works offline. The legacy browser Remote Management interface, Python management server/catalog, and their frontend service have been removed. Wi-Fi remains a separate workflow and continues to use `wifi_helper.py`.

## SD-card data and slot configuration

The data layout and artwork requirements are documented in:

[`../../docs/SD-CARD-DATA-CONTRACT-v1.md`](../../docs/SD-CARD-DATA-CONTRACT-v1.md)

The application reads slot assignments from:

```text
/storage/.config/kiddos/slots.conf
```

The file begins with this format identifier:

```text
KIDDOS_SLOTS_V1
```

It stores four ordered references for each of the four content categories. Empty entries are meaningful and must remain in place. Do not compact or reorder the file when an individual slot is empty.

Only library items that meet the artwork requirements are eligible for assignment in Parent Center.

## Service boundaries

The native library reader and slot manager own SD-card discovery and slot
persistence. The Wi-Fi service remains separate and invokes `wifi_helper.py`
for device Wi-Fi operations. Do not remove or bypass that helper as part of
unrelated library or Parent Center changes.

## Artwork refresh behavior

The library is discovered from the SD-card directory structure and artwork files.

Replacing an artwork file in place may not immediately refresh an already-loaded library entry. If artwork appears stale, reload the application or refresh the relevant library data. Avoid changing the data contract merely to work around a refresh issue.

## Build

The cross-compilation script is:

```text
build-kiddos-ui.sh
```

From this directory, run:

```bash
./build-kiddos-ui.sh
```

The script builds an ARM64 executable using the `aarch64-linux-gnu` toolchain and the required ARM64 SDL2 headers and libraries.

By default, the output is:

```text
../Build-Workspace/build/arm64/kiddos-ui
```

The build workspace can be changed for the build script with:

```bash
KIDDOS_BUILD_WORKSPACE=/path/to/workspace ./build-kiddos-ui.sh
```

The build script checks that the resulting executable is an AArch64 ELF binary and backs up an existing output before replacing it.

**Note:** The deployment script currently uses the default `../Build-Workspace` output path directly. Setting `KIDDOS_BUILD_WORKSPACE` for a custom build does not automatically change the deployment script's input path.

## Tests and local development build

The repository includes focused tests for the native library-data implementation and architecture boundaries:

```bash
./run-library-data-tests.sh
./run-architecture-tests.sh
```

A desktop CMake build can be performed from the repository root:

```bash
cmake --build build-working-ui -j2
```

These checks complement, but do not replace, testing on the target device.

## Deployment

The deployment script is:

```text
deploy-kiddos-ui.sh
```

It supports deployment and rollback operations. Deployment transfers the executable and related configuration/profile files, verifies checksums, backs up existing files, and checks the application service after installation. If installation or the service check fails, the script attempts to restore the previous files.

The script's default device is:

```text
root@192.168.0.89
```

The default executable destination is a test location:

```text
/storage/roms/.kiddos-ui-test-20261116/kiddos-ui
```

The script also deploys MP
