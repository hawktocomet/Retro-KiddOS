# Retro KiddOS — Session Handoff
Date: November 17, 2026

## Product goal

Prepare three FunnyPlaying Retro Pixel Pocket handhelds for Christmas family gaming. The children should be able to turn on their devices and easily choose games, music, or movies with minimal adult help.

Guiding principle:
"Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it."

The handheld is the player. A phone or computer is the management console. Avoid building custom software that does not materially improve family use.

## Hardware and development environment

Repository:
`https://github.com/hawktocomet/Retro-KiddOS`

Arch checkout:
`~/Projects/Retro-Pixel-Pocket/KiddOS`

Build workspace:
`~/Projects/Retro-Pixel-Pocket/Build-Workspace`

Current branch: `main`

Only the blue handheld is physically available right now. The purple and green devices are not yet in hand.

Blue device:
- Hostname: `RPPBLUE`
- IP: `192.168.0.89`
- SSH: `root@192.168.0.89`
- Firmware: FunnyPlaying Retro Pixel Pocket v1.1.3
- OS: RetroPixel 20251127 / CoreELEC 2025.11
- Architecture: ARM64/aarch64
- Display: 720x720
- ROM root: `/storage/roms`

Home network:
- SSID: `AZKABAN`
- Deco BE5000 mesh is in Access Point mode.
- Main router handles routing and DHCP.
- Arch computer: `192.168.0.94`
- Phone: `192.168.0.187`
- Jellyfin PC: `192.168.0.241:8096`

## Current working handheld interface

A custom SDL frontend is deployed and working on the blue device:
- Game selection screen with a 2x2 grid of box art.
- Selected Game screen with Play and Game Info.
- D-pad navigation.
- A selects; B backs out; Start opens the system menu.
- Select+Start exits a game.
- Shutdown menu and confirmation work.
- Battery/charging indicator is implemented and verified.

Current test executable:
`/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`

The current UI works. Avoid unnecessary redesign or lower-level SDL work.

## Management server — v0.3.0 deployed

The Python standard-library HTTP management server runs independently from the SDL frontend.

Live dashboard:
`http://192.168.0.89:8081/`

Live device directory:
`/storage/.kiddos-management-test-20261117`

Service:
`kiddos-management-test.service`

It is a transient systemd-run service. To restart it, stop it and recreate it with systemd-run. Do NOT use `systemctl restart kiddos-management-test.service`; transient units may disappear when stopped.

Launch command:
```
systemd-run --quiet --no-block \
  --unit=kiddos-management-test \
  --property=Restart=on-failure \
  --property=RestartSec=2 \
  --setenv=HOME=/storage \
  /usr/bin/python3 /storage/.kiddos-management-test-20261117/server.py --host 0.0.0.0 --port 8081
```

v0.3.0 features:
- `/api/status`: device, network, battery, and server status.
- `/api/library`: read-only games, systems, movies, and summary counts.
- Dashboard shows device status and the media library.
- Game inventory scans only the intended supported system directories.
- Genesis and Mega Drive are consolidated.
- Valid `gamelist.xml` metadata supplies display names; filenames are fallback.
- Save files and embedded game sound assets are not treated as media.
- Movies are scanned from `/storage/roms/movies`.
- Music is not configured yet and is displayed as such.
- No write operations, uploads, deletes, or active-slot configuration yet.

Validation:
- Local Python syntax check passed.
- Fixture tests passed, including metadata parsing and save-file exclusion.
- Local API and dashboard checks passed.
- v0.3.0 deployed successfully and verified on the device.
- User confirmed dashboard looks correct.
- User's wife independently accessed the dashboard from her laptop, validating LAN access from another household computer.

Backup of v0.2:
`/storage/.kiddos-management-backup-v02-before-v03-20261117`

Local source:
- `device/management-server/server.py`
- `device/management-server/web/index.html`

## Important deployment / terminal safety rules

The device's network can be slow and it may suspend after inactivity. Wake it with a button before SSH work. Keep remote commands short and targeted.

Direct SCP or raw SSH transfers of larger binaries can stall around 64 KiB. For executable deployments, split into 24 KB chunks, transfer individually, reassemble remotely, and verify SHA-256 before installation.

For management-server source files, transfer files individually and verify checksums.

Never use fail-fast shell patterns in instructions for the user's interactive terminal:
- No `set -e`
- No `set -u`
- No `set -o pipefail`
- No unnecessary `exit 1`

Use guarded operations, clear error messages, and recoverable steps. The user strongly prefers one terminal-ready block at a time, then they provide output or say "done".

Do not overwrite a live executable while its service is active. Keep backups and provide rollback.

## Media library findings

Current inventory on blue:
- GB: Kirby's Dream Land, Tetris
- GBC: Game & Watch Gallery 3
- GBA: Mario Kart - Super Circuit, Super Bust-A-Move
- SNES: Teenage Mutant Ninja Turtles IV - Turtles in Time
- Mega Drive: Streets of Rage 2
- Movies: original Totoro MKV and 720x720 encoded MP4
- Music: none configured

There are many embedded `.ogg` and `.wav` assets under `/storage/roms/homebrew/hcl`; these are game assets, not the user's music library. Do not include them in media inventory.

Keep existing ROM paths intact. Do not migrate files. `/storage/roms/movies` is the current movie location. A dedicated music location still needs to be chosen.

## Product direction and architecture

Handheld has four slots each for:
- Games
- Music albums/playlists
- Movies

L/R switches between Games, Music, and Movies.

Keep the stored media library separate from the four active slots. The library retains previously uploaded media; assigning a different item to a slot must not delete the old item.

Parent phone/computer should handle metadata retrieval, artwork, tagging, preview, and confirmation. Handheld stores and plays the prepared media. Do not require internet on the handheld for media management.

The browser management server must remain independent from the SDL frontend and operate on the LAN, including when internet access is unavailable.

Before adding write operations, design authentication and safe file handling. The current server is read-only and has no authentication. Do not casually add upload/delete endpoints without discussing the security model.

## Parent Mode and networking

Parent Mode direction:
- Hidden, PIN-protected, minimal on-device area.
- Primarily network setup, connection status, device information, and exit.
- Kids' Games/Music/Movies UI remains uncluttered.
- Browser management interface handles media, themes, and most configuration.

Network priorities:
1. Home Wi-Fi.
2. Configured phone hotspot for local-only management.
3. IBSS/ad-hoc direct networking as an experimental last resort.

Keep saved networks when adding new ones. Do not interrupt an active game for network changes. Do not test IBSS until other devices are physically available.

Network diagnostics must remain read-only. Never modify wireless config, restart supplicant, alter interfaces/channels, or run disruptive tests without explicit user consent.

Android Chrome access:
The user confirmed Chrome's app-level Nearby devices permission is the blocker for accessing the local handheld management server. Enabling it allowed access. The user considers this poor UX and has explicitly deferred a solution. Do not pursue a companion app or redesign now; retain the current LAN browser approach.

The Wi-Fi indicator is now implemented and verified beside the battery indicator. Wi-Fi radio power control is also implemented in Parent Mode; network scanning/joining and credential entry remain future work.

## Game curation scope

Round 1 only:
- Game Boy
- Game Boy Color
- Game Boy Advance
- Arcade
- SNES
- Genesis/Mega Drive
- Master System
- NES

Curate independent-play lists separately for the 3-year-old and 6-year-old, plus shared parent-supported multiplayer. Favor simple controls, forgiving gameplay, and minimal reading. The 6-year-old loves Pokémon and is learning to read; distinguish low-reading options from more reading-heavy titles.

## Movie conversion

A successful Totoro encode established a baseline:
- H.264 video
- AAC stereo audio
- 720x720 square pixels
- CRF 21
- medium preset
- 160 kbps audio

Preserve originals and make aspect-ratio treatment an explicit per-source decision. Do not blindly stretch everything.

## Suggested next steps

1. Keep this v0.3.0 milestone stable; do not immediately add more features.
2. Review and decide the next small, useful slice of the management experience with the user.
3. Identify the intended music directory and inventory rules before implementing music scanning.
4. Identify where/how active four-slot assignments should be stored before implementing slot management.
5. Design authentication and safe write behavior before adding uploads or configuration changes.
6. Keep testing against the actual blue handheld and household devices.

The user values practical progress and family validation over speculative architecture or large rewrites.


## Latest milestone — Parent Mode controls deployed (November 17, 2026)

The following update supersedes earlier statements in this handoff that Parent Mode and PIN entry were not implemented.

### Verified on blue device

- Parent Mode is hidden behind PIN entry.
- PIN default initialization and local salted SHA-512 crypt storage are implemented.
- PIN storage uses a private configuration directory and restrictive file permissions.
- Change PIN requires the current PIN and confirms the new PIN.
- Remote Management displays the actual LAN management server state.
- The local control starts/stops the management server using the installed control script:
  `/storage/.kiddos-management/control.sh`
- Stopping Remote Management does not turn off Wi-Fi. The user verified continuous ping during toggle testing.
- Both Remote Management and Change PIN were deployed and physically tested successfully.

### Deployment record

- Device: `RPPBLUE` (`192.168.0.89`)
- UI service: `kiddos-ui.service`
- Test executable:
  `/storage/roms/.kiddos-ui-test-20261116/kiddos-ui`
- Deployed executable SHA-256:
  `d0942a6e2ebf0bfe336cdafde64aa839c71eb67de87a5bac05bdf0372068a53d`
- Pre-deployment rollback executable:
  `/storage/roms/.kiddos-ui-test-20261116/kiddos-ui.pre-deploy-20261117-160015`
- Management control script on device:
  `/storage/.kiddos-management/control.sh`

The management server is independently controlled by the local Parent Mode toggle. Do not replace its service lifecycle implementation with duplicated C++ service-management logic; reuse the installed control script.

### Relevant source files

- `device/working-ui/main.cpp`
- `device/working-ui/pin_store.h`
- `device/working-ui/pin_store_test.cpp`
- `device/management-server/control.sh`

Native PIN helper tests passed (11 tests), including default initialization, PIN verification/change, file permissions, and fail-closed behavior for corrupted storage. The ARM64 cross-compile test passed.

### Latest milestone — Parent Mode Wi-Fi controls deployed (November 18, 2026)

Verified on the blue device (`RPPBLUE`, `192.168.0.89`):

- Parent Center now has Remote Management, Wi-Fi Settings, and Change Parent PIN.
- Wi-Fi Settings displays radio power (ON/OFF) and connection state (Connected/Not connected).
- Its action toggles Wi-Fi power using ConnMan (`connmanctl enable wifi` / `connmanctl disable wifi`).
- The Wi-Fi indicator is displayed beside the battery indicator and shows disconnected state with a red slash.
- The user tested Wi-Fi off/on locally on the handheld.
- Remote Management was confirmed to continue working independently after Wi-Fi was turned off and restored.
- No network scanning, joining, or credential-entry workflow was added.

Deployed ARM64 executable SHA-256:

`2531fa4d3d1376943fec0fbe4185f28450948dee060db1a93a0aef0fbceeb882`

The next development step should be selected based on practical value to the Christmas family-gaming goal, not automatically assumed to be more networking work. Keep the current implementation stable unless a concrete need emerges.

### Licensing and distribution direction

- Project license selected: GNU GPL-3.0-or-later.
- Copyright holder in project notices: `hawktocomet`; do not publish the user's legal name.
- The repository's own code can be licensed separately from the underlying RetroPixel firmware and third-party components.
- Preferred initial distribution approach: a reversible KiddOS installer for an existing RetroPixel SD card.
- A complete firmware image is a later possibility only after auditing the exact RetroPixel fork, component licenses, notices, and redistribution permissions.
- Do not distribute game ROMs, BIOS files, or third-party artwork without redistribution rights.
