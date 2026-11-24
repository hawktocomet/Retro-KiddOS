# Remote Parent Management Milestone — November 24, 2026

## Goal

Establish a practical browser-based way for a parent to manage the media available on a Retro Pixel Pocket over the home network, without requiring routine maintenance on the handheld.

The guiding principle remains:

> Every hour we spend developing should make it easier for the family to play games together. If it doesn't, we probably shouldn't be building it.

This milestone establishes the Remote Parent Management foundation. It does not attempt to finish every media category or replace the existing handheld game experience.

## Parent Management browser

The management page has been reorganized around a category-based home screen.

Implemented:
- Parent Management home screen with Games, Music, Movies, and TV Shows categories.
- Separate Games slot-management and installed-library pages.
- Four fixed game slots, with artwork, title, system, and clear/replace/assign actions.
- Assignment picker with explicit confirmation before replacing an occupied slot.
- Installed game library grouped by system, with artwork thumbnails and missing-artwork indicators.
- Expandable device-status section with connection, battery, storage, and KiddOS installation/version information.
- Responsive layouts for smaller browser screens.
- Clear placeholder destinations for workflows that are not implemented yet.

Music, Movies, and TV Shows destinations are placeholders. General media scanning and general media upload are also placeholders. The existing game upload workflow remains available for the currently enabled game systems.

## Game catalog and slot behavior

- The catalog retains awareness of the broader supported-system list, while only Game Boy and Game Boy Color are enabled for the current slot-management workflow.
- Games can appear in the installed library without artwork.
- Missing artwork no longer prevents an otherwise eligible game from being assigned to a slot.
- Replacing or clearing a slot does not delete the underlying ROM.
- Deleting a game is a separate, explicit operation; deleting an assigned game clears its slot.
- Slot assignments persist across catalog refreshes.
- Upload and assignment operations validate system, filename, slot, and replacement conditions.

## Artwork and metadata

Artwork lookup runs in the browser/client side of Remote Parent Management, using the more capable computer rather than making the handheld perform scraping work.

The browser workflow supports:
- Searching for Game Boy and Game Boy Color box art.
- Previewing a candidate before accepting it.
- Uploading accepted artwork to the handheld.
- Uploading a parent-selected image file.
- PNG, JPEG, and WebP artwork, with a 20 MiB upload limit.

The device-side artwork-find endpoint has been removed. The device serves artwork files and accepts validated uploads; it is not responsible for searching external artwork sources.

A credential-free metadata provider module is also present:
- Music metadata search uses MusicBrainz release-group data.
- TV metadata search uses TVmaze.
- Provider responses are normalized for the management API, summaries have HTML removed, and requests have bounded query lengths, result limits, and timeouts.

These provider capabilities are backend foundations; they do not mean the Music and TV Shows parent workflows are complete.

## Management server

The management server now includes:
- A short-lived, thread-safe game-catalog cache to avoid rebuilding the catalog for repeated artwork requests.
- Cache invalidation after operations that change ROMs, artwork, or slots.
- Explicit catalog refresh behavior that observes changes made outside the management API.
- Device storage totals, used space, free space, and usage percentage.
- KiddOS installation and version reporting.
- Meaningful management activity tracking.
- A 15-minute inactivity watchdog. Active management requests prevent shutdown, while passive status polling does not keep the service alive.
- An activity endpoint for browser-side workflows that need to register meaningful work.

The parent can also disable Remote Management manually through the existing device control workflow. The control script now exposes the device's Wi-Fi IPv4 address for connection information.

## API and validation coverage

The management API covers catalog and library retrieval, artwork retrieval and upload, ROM upload, slot assignment and unassignment, game deletion, device status, activity tracking, and metadata search.

Validation includes unsupported systems, invalid filenames, duplicate ROMs, unknown game IDs, invalid image contents, occupied slots without explicit replacement, and invalid provider requests.

## Milestone verification

On November 24, 2026:
- 42 tests passed.
- `git diff --check` passed.
- Local and handheld SHA-256 hashes matched for `server.py`, `catalog.py`, `providers.py`, and `web/index.html`.

Test command:

```bash
python -m unittest discover -s device/management-server/tests -v
## Deployment state

The current management implementation is deployed on the blue Retro Pixel Pocket at `192.168.0.89`.

The following local and device files were verified byte-for-byte by matching SHA-256 hashes:

| File | SHA-256 |
|---|---|
| `server.py` | `a56fad8a613261f18167e68c84b65a2f448bd1bcf12965d473411a5edccd3440` |
| `catalog.py` | `cdde9b21d2e88407dd9170ebfb1b14fbbbcb22240a0d4054b9684824922de85b` |
| `providers.py` | `4705d9a5bbf251a917b9f95ce04fe71e7a9bf0088ffcc01c09c89db414740774` |
| `web/index.html` | `b16b6784eec932ea38e701aca7d762429dd4d5b8b09d1050f683c86c548ede1c` |

The server health check succeeded after deployment. The server deployment backup was retained on the device at:

`/storage/.kiddos-management/.backups/server.py.20261124-004230`

No additional device deployment is required for this milestone.

## Explicitly out of scope

- Completing Music, Movies, or TV Shows parent workflows.
- General-purpose media scanning and upload.
- Expanding the currently enabled game-slot systems beyond GB and GBC.
- Moving artwork or metadata scraping onto the handheld.
- Reworking the child-facing handheld UI as part of this management milestone.

The child-facing UI and firmware changes in the working tree are separate work and are intentionally not included in this milestone commit.

## Next direction

Continue Remote Parent Management from this foundation. Prioritize workflows that reduce the effort required for a parent to curate and maintain the children's available media. Keep unfinished categories visibly unfinished rather than presenting placeholder navigation as working functionality.
