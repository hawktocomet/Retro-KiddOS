# Retro KiddOS Product & Technical Roadmap

**Status:** Living design document  
**Last consolidated:** November 17, 2026  
**Scope:** Product and technical decisions discussed November 16–17, 2026

This document is the central reference for the product direction, architecture, priorities, and deferred ideas. Recording an idea here does not automatically make it an implementation commitment. Implementation status is called out separately.

## 1. Product vision

Retro KiddOS is a simple, child-friendly entertainment experience for the FunnyPlaying Retro Pixel Pocket. The immediate goal is practical family use by Christmas 2026: children should be able to turn on their handhelds, choose something familiar, and play or enjoy their media with as little adult assistance as possible.

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

The broader principle applies to games, music, and movies: development should make it easier for the children to enjoy their devices. Prefer existing firmware capabilities over custom infrastructure. Avoid technically interesting work that does not improve the family experience.

### Product priorities

1. A reliable, understandable experience for children.
2. Easy independent use, especially for the younger child.
3. Easy parent management from a phone or computer.
4. Family multiplayer where supported.
5. Practical offline operation.
6. Minimal custom code and administration on the handheld.

The project is not intended to replace the underlying firmware, emulator stack, or media players.

## 2. Child-facing handheld experience

The normal KiddOS interface should remain deliberately simple, with three top-level categories:

- **Games**
- **Music**
- **Movies**

The proposed navigation uses the physical L/R buttons to switch between categories. Each category presents four active slots. Music and Movies playback interactions remain to be validated.

The normal child-facing interface should not expose network configuration, media-library administration, theme editors, or other parent tools.

### Games interface

The current working SDL prototype has a four-game selection screen and a Selected Game screen. The visual direction is:

- 720 × 720 display.
- Dark navy and electric-blue styling.
- Large, prominent box-art artwork.
- No redundant title text beneath box art in the game grid.
- Selected Game screen with prominent artwork and title/platform information.
- **Play** as the primary/default action; Game Info as a secondary action.
- Physical-button navigation with clear, minimal hints.

Current tested controls:

- D-pad: navigate.
- A: select.
- B: back.
- Start: system menu.
- Select + Start: exit a running game and return to KiddOS.

The current working frontend has been tested on the blue device. It is a prototype, not yet a validated release across all three handhelds.

### Media slots and library

Separate the **stored media library** from the **four active slots** in each category.

- Media uploaded to a device remains in its library even when it is no longer assigned to a slot.
- Parents can assign an existing library item to a slot.
- Parents can upload new content and assign it to a slot.
- Replacing a slot assignment must not automatically delete the previous media.
- Slot assignments are references to stored content, not destructive moves.

This allows parents to rotate the four visible choices without repeatedly transferring or losing media.

### Game library scope and curation

Round 1 systems are limited to:

- Game Boy
- Game Boy Color
- Game Boy Advance
- Arcade
- Super Nintendo
- Sega Genesis / Mega Drive
- Sega Master System
- Nintendo Entertainment System

Do not expand the system list unless the project scope changes.

Curate games for realistic independent play by a three-year-old and/or six-year-old: simple controls, forgiving gameplay, and minimal reading. Maintain distinct lists for:

- Three-year-old independent play.
- Six-year-old independent play.
- Shared/overlap games suitable for parent-supported multiplayer.

Pokémon is an important consideration for the six-year-old. Distinguish titles currently manageable with limited reading from reading-heavy titles that may become more rewarding as reading skills improve.

The current prototype's game list is hardcoded. A parent-managed game library and editable active slots are future work, not implemented functionality.

### Music and movies

Music may be represented as albums or playlists, potentially as folders, but the precise playback model and controls remain open design questions.

Movies should use existing firmware playback capabilities wherever possible. MPV has successfully played a 720 × 720 H.264/AAC test clip with sound. Full-length movie playback and the complete child-friendly browsing experience still need validation.

A documented local movie-preparation pipeline should build on the successful Totoro encode:

- H.264 video.
- AAC stereo audio.
- 720 × 720 output with square pixels.
- CRF 21.
- Medium preset.
- 160 kbps audio.

Aspect-ratio handling must be an explicit per-source choice; do not blindly stretch every source. Preserve originals, verify converted files, and prioritize an easy repeatable workflow.

## 3. Parent Mode on the handheld

Parent Mode is a hidden, PIN-protected area for essential on-device administration.

### Implemented and verified on the blue Retro Pixel Pocket (November 17, 2026)

- Parent Mode is accessible from the system menu and protected by a PIN.
- The default PIN is initialized on first use; parents can change it after entering the current PIN.
- PIN data is stored locally as a salted SHA-512 crypt hash, with restrictive directory/file permissions.
- The Remote Management control reports whether the LAN management server is running.
- Parents can start or stop the management server locally.
- Remote Management is independent of Wi-Fi: stopping the server does not disconnect Wi-Fi.
- The PIN and Remote Management flows were deployed and physically tested on the blue device.

The PIN is a practical child-proofing measure, not a security boundary against someone with root or physical filesystem access.

### Next implementation phase

1. **Wi-Fi controls:** inspect connection state, turn Wi-Fi on/off, and connect to a selected or newly configured network.
2. **Connection status:** add a small connected/disconnected Wi-Fi indicator beside the battery indicator on the child-facing home screen.

Network changes must not interrupt an active game unnecessarily. Preserve saved networks and avoid disrupting a working connection during implementation and testing.

Parent Mode should remain small. Media-library administration and broader configuration belong in the browser management interface.

## 4. Phone/computer management interface

The phone or computer should be the primary place where parents configure and manage KiddOS. **The handheld is the player; the browser is the management console.**

The intended management experience is a local web application served by the handheld over HTTP on the local network. It should work without internet access when the phone/computer and handheld share a LAN.

### Management capabilities

The browser interface is intended to handle:

- Uploading and organizing games, music, and movies.
- Browsing the stored media library.
- Assigning existing items to the four active slots in each category.
- Replacing slot assignments without deleting stored media.
- Editing or confirming titles, descriptions, metadata, and artwork.
- Retrieving metadata and artwork on the parent device where practical.
- Theme customization: colors, artwork, visual style, and layout options.
- Broader device configuration and maintenance as those needs become clear.

The browser is also the preferred place for media identification and preparation. The parent device should handle metadata/artwork retrieval, tag extraction, previews, edits, and confirmation before transferring prepared media to the handheld. The handheld should not need internet access or scraping services to manage its local library.

A browser-only workflow may have limitations for some metadata sources or file operations. A small companion application could be considered if browser capabilities prove insufficient; this is not a current commitment.

### Server and frontend separation

The management HTTP server should run independently of the SDL frontend. Restarting or crashing the child-facing interface should not unnecessarily take down the management service.

The server should provide a small local API and serve the browser interface. The SDL frontend reads the active slot/configuration data and presents the selected content. Exact API routes, authentication/session details, and configuration file formats remain to be designed.

Because the server is reachable on the local network, access control and safe handling of uploads and configuration changes must be considered during implementation. The Parent Mode PIN is for on-device access; browser-side authorization details still need a deliberate design.

### Why this approach

The handheld has limited resources, an older Linux environment, and a small 720 × 720 display. A lightweight HTTP service plus browser-based UI keeps the device-side implementation small while allowing a much richer management experience on modern phones and computers.

Most of the management interface can be developed and tested on a regular computer before it is deployed to the handheld.

## 5. Networking strategy

Networking is tiered, with a clear priority order:

1. **Home Wi-Fi — first priority.** This is the known-working baseline and the first implementation focus.
2. **Configured phone hotspot — fallback.** When home Wi-Fi is unavailable, the handhelds can join a phone hotspot. The hotspot only needs to provide a local network; internet access is not required for local multiplayer or parent management.
3. **IBSS/ad-hoc direct networking — last-resort experiment.** This is a possible direct device-to-device option, not a prerequisite for the core product.

### Expected behavior

- Automatically use the highest-priority available configured network.
- Fall back to a lower-priority network when the preferred one is unavailable.
- Reconnect to a higher-priority network when it becomes available again.
- Do not interrupt an active game merely because network connectivity changes.
- Preserve saved networks when parents add new ones.
- Keep local management and multiplayer functional without internet access.

The home network is first because connectivity has already been demonstrated on the blue device. Hotspot fallback is the next useful capability for travel. IBSS/ad-hoc remains experimental and should wait until the other physical handhelds are available for testing.

### Current hardware findings

The blue Retro Pixel Pocket uses a Realtek RTL8188EUS Wi-Fi adapter and a vendor rtl8188eu driver. The device has connected successfully to the home Wi-Fi network. Driver capability listings include IBSS and P2P-related modes, but this does not establish that direct networking is reliable or fully supported by the shipped firmware.

Do not disrupt the working home Wi-Fi connection for speculative experiments. Further direct-networking tests are deferred until the other devices are physically available.

## 6. Multiplayer

Family multiplayer is an important secondary goal, but it should not delay the core child-friendly experience.

Desired examples include:

- Pokémon trading between the children's handhelds.
- Mario Kart head-to-head play.

The intended network foundation is home Wi-Fi first, then a shared phone hotspot when traveling. Direct device-to-device networking is a possible later fallback.

Actual game compatibility, emulator/core requirements, connection workflow, and child-friendly pairing experience still need investigation and hardware testing. The goal is to make multiplayer require as little adult intervention as practical.

## 7. Technical architecture at a glance

| Area | Handheld | Parent phone/computer |
|---|---|---|
| Child experience | SDL interface and playback | Not required |
| Media storage | Stores local media | Uploads and organizes |
| Active slots | Reads/applies assignments | Creates and edits assignments |
| Metadata/artwork | Displays prepared assets | Retrieves, previews, edits |
| Themes | Loads/applies selected theme | Creates and customizes themes |
| Network | Connects and exposes local services | Configures through Parent Mode or browser when reachable |
| Management | Lightweight HTTP server/API | Browser management UI |
| Playback | Uses existing firmware players/emulators | Prepares media and manages library |

Conceptually:

**Parent browser → local HTTP API/server → media library and configuration → KiddOS SDL frontend**

The management server should remain independent of the SDL process. Each handheld manages its own local library and configuration; the same phone or computer can manage each device separately.

## 8. Current implementation status

### Verified on the blue Retro Pixel Pocket

- SDL frontend launches through the existing firmware startup mechanism.
- Four-game selection screen and Selected Game screen work.
- D-pad, A, B, Start, and game-exit controls work as intended.
- Games launch through the existing firmware and return to KiddOS.
- System menu and shutdown confirmation work.
- Battery percentage and charging status are displayed.
- The device has connected to the home Wi-Fi network.
- MPV has played a 720 × 720 H.264/AAC test clip with sound.
- Hidden, PIN-protected Parent Mode is deployed and working.
- Parent Mode can start/stop the LAN management server through the installed control script.
- Remote Management can be disabled without disconnecting Wi-Fi; continuous ping remained active during testing.
- Change PIN flow is deployed and working.

### Not yet implemented or fully validated

- Parent Mode Wi-Fi controls and saved-network selection.
- Home-screen Wi-Fi connection indicator.
- Browser-based media upload/library and slot assignment.
- Browser-based theme customization.
- Automatic network priority/fallback behavior.
- Phone-hotspot fallback.
- IBSS/ad-hoc direct networking.
- Multiplayer workflows.
- Full-length movie playback and a complete music experience.
- Validation on the purple and green handhelds.

The current frontend is a working prototype validated on one blue device. Do not assume its startup hook or deployment procedure is ready for the other devices without reviewing device paths and recovery steps.

## 9. Priorities and deferred work

### Near-term priorities

1. Keep the verified Parent Mode, PIN, and Remote Management features stable.
2. Add Wi-Fi on/off and connect-to-network controls in Parent Mode, preserving saved networks.
3. Add a connected/disconnected Wi-Fi indicator beside the home-screen battery indicator.
4. Validate the changes on the blue handheld without disrupting the existing management service or gameplay.
5. Continue the browser management experience incrementally after the on-device network controls are reliable.

### Deferred until the foundation is proven

- Phone-hotspot fallback and automatic network priority switching.
- IBSS/ad-hoc and other direct device-to-device experiments.
- Full multiplayer pairing and game-specific testing.
- Rich media metadata/artwork automation and any companion application.
- Advanced theme editor and broad device configuration.
- Wi-Fi configuration screens outside the minimal Parent Mode workflow.

### Explicit project guardrails

- Do not build a large custom OS when the existing firmware can do the job.
- Do not put the full media manager or theme editor on the handheld.
- Do not make internet access a requirement for local playback, local management, or local multiplayer.
- Do not delete library media just because a slot assignment changes.
- Do not let network transitions interrupt an active game.
- Do not pursue IBSS/direct networking ahead of the known-working home Wi-Fi path.
- Do not let infrastructure work displace the Christmas goal: children should be able to use their devices easily and enjoy games together.

## 10. Open design questions

These questions are intentionally left open rather than prematurely specified:

- What exact physical shortcut opens Parent Mode, and how is the PIN entered?
- What local hostname/address will parents use to open each device's management page?
- What authentication/session model should protect browser-side administration on the LAN?
- What API and on-device configuration format should connect the server, media library, slot assignments, themes, and SDL frontend?
- How should Music albums/playlists and Movies browsing/playback work for children?
- Which metadata/artwork sources and parent-side preparation workflow are practical?
- What network manager/configuration mechanism is safest on the shipped firmware?
- Which multiplayer titles and emulator cores can reliably communicate?

Resolve these when their implementation becomes the next useful step, not as prerequisites for keeping the current prototype usable.

## Management Server v0.3.0 — Deployed and Validated

**Status: deployed and tested on the blue Retro Pixel Pocket, November 17, 2026.**

The independent LAN management server now provides a read-only inventory of the handheld's stored games and movies.

Implemented:
- Server version 0.3.0.
- `GET /api/library` returns games, supported-system counts, movies, and summary counts.
- Game inventory scans only the configured Round 1 systems: GB, GBC, GBA, arcade, SNES, Genesis/Mega Drive, Master System, and NES.
- Genesis and Mega Drive directories are presented as one combined system.
- XML game metadata is used when valid, with filename fallback.
- Save files and unrelated embedded game audio are excluded.
- Movie inventory scans `/storage/roms/movies`.
- Music is explicitly shown as not configured; no unsupported music inventory is inferred.
- Dashboard displays the handheld, network, battery, and management-server status alongside the media library.
- The API remains read-only. No upload, delete, or active-slot modification operations have been introduced.

Validation:
- Local syntax and fixture tests passed.
- Local API and dashboard checks passed.
- Deployed to the blue handheld and verified version 0.3.0 and HTTP 200 dashboard response.
- The live dashboard was opened successfully from the user's computer and independently from the user's wife's laptop.

Deployment details:
- Dashboard: `http://192.168.0.89:8081/`
- Device service: transient `kiddos-management-test.service`
- Live directory: `/storage/.kiddos-management-test-20261117`
- v0.2 backup retained at `/storage/.kiddos-management-backup-v02-before-v03-20261117`

The Android Chrome Nearby devices permission requirement remains deferred. The current LAN browser-management approach stays in place; do not start a companion-app or browser redesign effort without revisiting priorities with the user.

The small Wi-Fi status icon beside the handheld battery indicator also remains deferred.

This milestone validates the core architecture: the handheld stores and applies information, while a phone or computer provides the management interface.

### Later distribution work

- Build a reversible installer for an existing RetroPixel SD card rather than redistributing a complete firmware image by default.
- Audit the exact RetroPixel fork and every bundled component before publishing any complete image.
- Preserve third-party licenses, notices, and source-offer obligations.
- Do not bundle ROMs, BIOS files, or artwork without explicit redistribution rights.
