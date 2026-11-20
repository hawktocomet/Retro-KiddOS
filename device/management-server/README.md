# Retro KiddOS Remote Management

The device-side Remote Management service provides a browser interface for
viewing the installed game library and managing the four child-facing game
slots. It shares its catalog implementation with the KiddOS frontend.

## Components

- `server.py` — HTTP server, device status, library and catalog APIs, ROM
  upload, artwork upload, and slot-management endpoints.
- `catalog.py` — ROM discovery, stable game IDs, artwork association,
  slot validation, persistent assignment storage, and catalog generation.
- `web/index.html` — browser interface for library management, artwork
  discovery/upload, and slot assignment.
- `control.sh` — device-side service control helper.
- `tests/` — catalog and HTTP API tests.

The server reports version `0.3.0` and uses port `8081`.

## Device paths

| Purpose | Path |
|---|---|
| ROM library root | `/storage/roms` |
| Persistent management state | `/storage/.kiddos-management` |
| Slot assignments | `/storage/.kiddos-management/slots.json` |
| Installed shared catalog module | `/storage/.kiddos-management/catalog.py` |

The slot file is user data. Do not remove or overwrite it during routine
frontend or service updates. The catalog creates an initial four-slot
configuration only when no slot file exists.

## Catalog and slot rules

The catalog discovers games across the supported library systems, including
GB, GBC, GBA, arcade, SNES, Genesis/Mega Drive, Master System, and NES.
However, the four KiddOS slots currently accept only raw `.gb` and `.gbc`
files.

New assignments require associated artwork. Supported artwork sidecars are
PNG, JPG, and WebP files beside the ROM. Games without artwork remain visible
in the library but are excluded from the available-to-assign list, and the
assignment API rejects them.

A game can occupy only one slot. Reassigning a game moves it; replacing an
occupied slot requires an explicit replace operation. Unassigning leaves the
ROM on the device. Deleting a game removes its ROM and clears any slot
assignment that refers to it.

The catalog writes `slots.json` using a temporary file and atomic replacement.
The KiddOS frontend polls the file once per second and detects changes, so
slot updates appear in the open carousel without restarting the frontend.

## Artwork workflow

The browser interface can search the Libretro thumbnail repository for GB/GBC
box art. It normalizes titles for matching, downloads a candidate, and asks
the parent to preview and confirm before uploading it. This search runs in
the browser; the handheld does not need to fetch artwork from GitHub.

If no matching image is found, or the system is not supported by automatic
search, the parent can upload a local PNG, JPG, or WebP image.

The server validates the game ID, filename, extension, content length, and
image signature. It accepts files from 1 byte through 20 MiB, writes to a
temporary file in the ROM directory, and atomically replaces the artwork
destination after validation.

## HTTP API

| Method | Route | Purpose |
|---|---|---|
| GET | `/` | Remote Management web page |
| GET | `/api/status` | Device and service status |
| GET | `/api/library` | Device library status |
| GET | `/api/catalog` | Four slots and discovered game catalog |
| POST | `/api/upload` | Upload a ROM |
| POST | `/api/artwork/upload` | Upload artwork for a discovered game |
| POST | `/api/slots/assign` | Assign or replace a slot |
| POST | `/api/slots/unassign` | Clear a slot without deleting its game |
| POST | `/api/games/delete` | Delete a game and clear any assignment |

For exact request fields, response structures, and validation behavior, see
the route handlers in `server.py` and the API tests.

## Tests

Run the management-server tests from the repository root:

```bash
python3 -m unittest discover -s device/management-server/tests -v
```
