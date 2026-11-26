# Game Library and Slot Management

## Overview

KiddOS discovers games and media directly from the SD card beneath
`/storage/roms`. The SD card is the source of truth for media files, folder
organization, and artwork. The native C++ Parent Center manages assignments;
it does not upload, move, or delete content.

The native reader and slot manager are implemented in
`device/working-ui/services/LibraryData.cpp` and
`device/working-ui/services/LibraryData.hpp`.

## Game discovery and eligibility

The current native game reader scans these directories:

- `gb/` — raw `.gb` files
- `gbc/` — raw `.gbc` files

A game must have adjacent artwork with the same filename stem. Supported
artwork extensions are `.png`, `.jpg`, and `.webp`, with PNG preferred,
then JPG, then WebP. Extension matching is case-insensitive.

Other platforms and archive formats described by the SD-card data contract
are not currently offered as assignable games by this native reader.

## Other media discovery

- Movies are individual files directly inside `movies/`. Supported formats
  are `.mp4`, `.mkv`, `.avi`, `.mov`, and `.m4v`. Each requires adjacent
  same-stem `.png` or `.jpg` artwork.
- Music playlists are immediate child directories of `music/`. Each requires
  `cover.jpg` or `cover.png` and at least one supported audio track. Tracks
  may be nested; they are not independently assignable.
- TV shows are immediate child directories of `tvshows/`. Each requires
  artwork named `poster.jpg`, `poster.png`, `cover.jpg`, or `cover.png`,
  in that preference order, and at least one supported episode. Episodes
  may be nested; they are not independently assignable.

The exact folder and artwork conventions are defined in
[SD Card Data Contract v1](SD-CARD-DATA-CONTRACT-v1.md).

## Four persistent slots per category

Parent Center manages four stable slots in each category:

- Games
- Movies
- Music Playlists
- TV Shows

A parent can assign an eligible item to an empty slot, replace an occupied
slot, move an already assigned item to another slot, or clear a slot.
An item can occupy at most one slot within its category. Empty slots retain
their positions; assignments are never compacted automatically.

Assigning, replacing, moving, or clearing a slot changes only the slot
references. It does not move or delete the media or artwork on the SD card.

## Slot persistence

Slot assignments are stored in:

`/storage/.config/kiddos/slots.conf`

The file begins with the format identifier:

`KIDDOS_SLOTS_V1`

It stores four ordered, quoted relative-path references for each of the four
categories. References are relative to `/storage/roms`, not absolute mount
paths. The slot file is configuration state, not a second copy of the media
catalog.

Do not delete, reset, compact, or manually rewrite this file as part of
routine development or deployment. Back it up before any explicitly approved
migration or destructive operation.

If an assigned item is missing, no longer eligible, or no longer resolves
inside the ROM root, it is treated as unavailable. The underlying files are
not modified.

## Offline architecture

The native C++ application performs filesystem discovery, artwork
eligibility checks, slot assignment persistence, and content launch/playback.
The Parent Center does not require Wi-Fi, HTTP, Python, or a browser-based
management server.

Wi-Fi functionality remains a separate service and still uses the
`wifi_helper.py` helper.

## Current scope and limitations

- Four stable slots per category.
- Native game assignment currently supports raw GB and GBC ROMs with artwork.
- Movies, playlists, and TV shows require their category-specific artwork and
  media content.
- Music playlists and TV shows are containers; individual tracks and
  episodes are not Parent Center items.
- Artwork replaced in place may not be detected until the application
  reloads its library data.
- Multiplayer workflows, including Pokémon trading and head-to-head play,
  remain future work.
