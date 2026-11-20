# Game Library and Slot Management

## Overview

The game library is discovered from the device's ROM storage. The device-side
Python catalog is shared by the Remote Management service and the KiddOS
frontend, so ROM discovery, slot eligibility, artwork association, and
assignments have one source of truth.

The current child-facing carousel contains four stable slots. Parents manage
the assignments through the Remote Management web interface.

## ROM discovery and eligibility

The catalog scans the supported system directories under `/storage/roms` and
also recognizes the legacy `.kiddos-ui-test-20261116` directory.

The broader library recognizes these systems:

- Game Boy
- Game Boy Color
- Game Boy Advance
- Arcade
- Super Nintendo
- Genesis / Mega Drive
- Master System
- Nintendo Entertainment System

The current four-slot carousel is intentionally narrower: only raw `.gb` and
`.gbc` ROM files are eligible. ZIP archives and games from other systems can
appear in the discovered library but cannot currently be assigned to a slot.

Each discovered game has a stable ID derived from its path relative to the ROM
root. The catalog reports its title, platform, ROM path, file size, artwork
path, and slot eligibility.

## Four persistent slots

Slot assignments are stored in:

`/storage/.kiddos-management/slots.json`

The catalog module is installed alongside it at:

`/storage/.kiddos-management/catalog.py`

The four slot positions remain stable even when a slot is empty. A game can
occupy only one slot at a time; assigning it to another slot moves it.
Replacing an occupied slot requires an explicit replace operation.

Unassigning a game clears its slot but leaves the ROM and artwork on the
device. Deleting a game removes its ROM and clears any slot assignment that
references it. Deletion is distinct from unassignment.

**Preserve the slot configuration as user data.** Do not delete, reset, or
manually rewrite `slots.json` as part of routine development or deployment.
Back it up before any explicitly approved migration or destructive operation.

## Artwork requirements

Every newly assigned game must have artwork. A game without artwork remains
in the discovered library, but is not offered as an assignable game and the
server rejects attempts to assign it.

Artwork is stored beside its ROM using the same base filename. Supported
sidecar extensions are `.png`, `.jpg`, and `.webp`.

The Remote Management page offers two ways to provide artwork:

1. **Find Artwork** searches the Libretro thumbnails repository from the
   browser for Game Boy and Game Boy Color box art. It uses normalized title
   matching, downloads a candidate image, and displays a preview. The parent
   must confirm the preview before the image is uploaded to the device.
2. **Upload Artwork** allows the parent to select a local PNG, JPG, or WebP
   image when automatic search is unavailable or does not find a match.

Automatic artwork search currently supports Game Boy and Game Boy Color
only. If no exact candidate is found, use manual upload. The browser checks
the downloaded PNG signature and size before upload. The device independently
validates the game ID, filename, extension, content length, and image
signature. The server accepts artwork from 1 byte through 20 MiB and writes it
to a temporary file before atomically replacing the destination.

Existing slot assignments are not automatically cleared simply because an
artwork requirement is added or a catalog refresh occurs. The artwork rule
prevents new assignments without art; it does not silently rewrite existing
user slot state.

## Live carousel refresh

KiddOS loads the four slots from the shared Python catalog at startup. While
the application is running, it checks
`/storage/.kiddos-management/slots.json` once per second.

The management catalog atomically replaces the slot file when assignments
change. KiddOS detects the replacement using inode and nanosecond
modification-time changes, reloads the catalog, and refreshes game names,
platform labels, and artwork. If a selected game has been removed from its
slot, KiddOS returns to the Games carousel. A failed reload is retried on a
subsequent poll.

No reboot, application restart, manual refresh, or frontend rebuild is
required for a slot assignment change to appear.

## Remote Management workflow

The Remote Management page is served by the device-side service. Its library
view shows discovered games, their slot eligibility, and artwork readiness.
Parents can upload ROMs, find or upload artwork, assign or replace a slot,
unassign a game, and delete a game.

The service API and implementation details are documented in
`../device/management-server/README.md`.

## Current scope and limitations

- Four child-facing slots.
- Only raw `.gb` and `.gbc` files are currently slot-eligible.
- Browser-side automatic artwork search supports GB and GBC; manual upload
  covers other cases.
- The broader library can discover additional systems, but those systems are
  not yet assignable to the four-slot carousel.
- Multiplayer workflows, including Pokémon trading and head-to-head play,
  remain future work.
- Music and Movies browsing/playback are not implemented.

The library should remain focused on games the children can realistically
play, with simple controls, forgiving gameplay, and minimal reading.
