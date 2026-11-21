# Retro KiddOS Product & Technical Roadmap

**Status:** Living design document  
**Last consolidated:** November 20, 2026
**Scope:** Product and technical decisions discussed November 16–20, 2026

This document records product direction, architecture, historical decisions, and deferred ideas. Recording an idea here does not automatically make it an implementation commitment.

## Current direction — two independent projects

The project now has two related but independently useful parts. Their responsibilities and transition boundaries are documented in [`PROJECT-BOUNDARIES.md`](PROJECT-BOUNDARIES.md).

### Retro KiddOS — immediate product priority

The immediate priority is a reliable, child-friendly experience on the existing FunnyPlaying Retro Pixel Pocket firmware. Preserve the working stock-firmware integration and build on the tested device-side frontend and parent-management capabilities.

Christmas progress must not depend on first producing a custom firmware image. Do not freeze useful product work for the sake of image-building infrastructure, and do not destabilize the working device setup to pursue the longer-term platform.

Near-term work should be judged by whether it makes the devices easier for the children to use and easier for parents to manage. Keep the implementation small and practical.

### Retro Pixel Foundation — independent platform direction

The longer-term platform effort is a transparent, reproducible, independently buildable foundation for the Retro Pixel Pocket. It should provide hardware support, operating-system/runtime integration, shared device capabilities, documented interfaces, dependency provenance, and developer build instructions.

The Foundation must be useful without KiddOS. KiddOS should become a replaceable reference frontend that uses public Foundation interfaces rather than defining or owning the platform.

A reproducible flashable image remains an important Foundation deliverable, but it is a separate workstream—not a prerequisite for Christmas KiddOS progress. Existing firmware-image research and artifacts remain valuable technical evidence. See [`CUSTOM-IMAGE-ARCHITECTURE.md`](CUSTOM-IMAGE-ARCHITECTURE.md).

### Working rules

- Keep the current stock-firmware path working while the Foundation is developed separately.
- Do not move or rewrite source code merely to make the conceptual split look complete. Establish and test interfaces before migration.
- Preserve existing source changes, backups, forensic artifacts, and working device state.
- Keep current emulator/core behavior—including the intentional Gambatte prototype integration—unless a demonstrated product need justifies changing it.
- Prefer small, reversible steps. Do not perform destructive cleanup, firmware flashing, or device deployment without explicit approval.

## 1. Product vision

Retro KiddOS is a simple, child-friendly entertainment experience for the FunnyPlaying Retro Pixel Pocket. The immediate goal is practical family use by Christmas 2026: children should be able to turn on their handhelds, choose something familiar, and play or enjoy their media with as little adult assistance as possible.

> Every hour we spend developing should make it easier for you and your kids to play games together. If it doesn't, we probably shouldn't be building it.

The broader principle applies to games, music, movies, and TV: development should make it easier for the children to enjoy their devices. Prefer existing firmware capabilities over custom infrastructure. Avoid technically interesting work that does not improve the family experience.

### Product priorities

1. A reliable, understandable experience for children.
2. Easy independent use, especially for the younger child.
3. Easy parent management from a phone or computer.
4. Family multiplayer where supported.
5. Practical offline operation.
6. Minimal custom code and administration on the handheld.

The project is not intended to replace the underlying firmware, emulator stack, or media players.

## 2. Child-facing handheld experience

The current implemented child-facing library experience is Games. The broader product direction includes:

- **Games**
- **Music**
- **Movies**
- **TV shows**

These are product areas, not a requirement for four fixed top-level buttons or identical navigation depth. The experience should suit the child's age and familiarity: a younger child may need a very flat selection of familiar items, while an older child may benefit from categories, systems, collections, playlists, shows, seasons, and episodes.

The same underlying content library may support different child-facing experiences. Keep this as future product direction; do not mistake it for implemented functionality or expand the Christmas scope without a clear benefit.

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

The intended design separates the **stored content library** from the **featured choices** shown to a child. The four-game slot workflow is implemented today; equivalent music, movie, and TV assignment workflows are future work.

- Content should remain in the library when it is no longer featured.
- Parents should be able to feature an existing library item or add new content.
- Replacing a featured assignment must not automatically delete the previous content.
- Assignments should be references to stored content, not destructive moves.

The number and presentation of featured choices may vary by content type and child experience. Do not force every category into an identical four-slot model.

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

Historical note: this paragraph described an earlier prototype. The maintained implementation now has a shared device-side game catalog, parent-managed game assignment, artwork workflows, and live carousel updates. See `README.md` and `docs/ARCHITECTURE.md` for current application status. The custom-image architecture document preserves earlier firmware packaging research.

### Music, movies, and TV

Music may be organized as playlists, albums, or individual tracks. Movies may be organized into collections or genres. TV may be organized by show, season, and episode. The precise child-facing playback controls and navigation remain future design work.

MPV has already been demonstrated playing a 720 × 720 H.264/AAC test clip with sound on the device. This is useful proof of playback capability, not validation of a complete movie or TV experience.

Content preparation should primarily happen on the parent's computer, not on the resource-constrained handheld. The intended workflow is to keep originals in the parent's library and prepare device-ready copies for transfer. A local companion service may be needed alongside the browser interface for filesystem access, FFmpeg conversion, metadata/artwork work, compatibility checks, and synchronization. This is an architectural direction, not a commitment to a particular implementation yet.

The successful Totoro encode provides a starting point for a repeatable movie profile:

- H.264 video.
- AAC stereo audio.
- 720 × 720 output with square pixels.
- CRF 21.
- Medium preset.
- 160 kbps audio.

Aspect-ratio handling must be an explicit per-source choice; do not blindly stretch every source. Preserve originals and verify converted files.

For content to be assignable to a child's experience, it should be both **usable** on the target device (the required player/runtime exists and the content is in a supported format) and **recognizable** (it has a title, relevant metadata, and suitable artwork). Content can remain in the parent's library while it is being prepared or awaiting metadata/artwork.

## 3. Parent Mode on the handheld

Parent Mode is a hidden, PIN-protected area for essential on-device administration.

### Implemented and verified on the blue Retro Pixel Pocket (November 17–18, 2026)

- Parent Mode is hidden and PIN-protected.
- The default PIN is initialized on first use; parents can change it after entering the current PIN.
- PIN data is stored locally as a salted SHA-512 crypt hash, with restrictive directory/file permissions.
- Parent Center has three entries: Remote Management, Wi-Fi Settings, and Change Parent PIN.
- Remote Management reports whether the LAN management server is running and lets parents start or stop it locally.
- Wi-Fi Settings reports Wi-Fi radio power and connection state, and provides a single action to turn Wi-Fi on/off.
- Wi-Fi state is read from ConnMan; power changes use `connmanctl enable wifi` and `connmanctl disable wifi`.
- A compact Wi-Fi connected/disconnected indicator appears beside the battery indicator.
- Remote Management is independent of Wi-Fi power. This was tested by turning Wi-Fi off and back on locally, then confirming Remote Management still worked.
- Parent Mode, Remote Management, Change PIN, Wi-Fi controls, and the indicator were deployed and physically tested on the blue device.

The PIN is a practical child-proofing measure, not a security boundary against someone with root or physical filesystem access.

### Wi-Fi configuration — complete and frozen

- Parent Mode can scan for available Wi-Fi networks and display selectable results.
- Parents can select open or secured networks and connect through ConnMan.
- Secured-network password entry uses an on-screen QWERTY keyboard with letter-case and symbol layouts, space, backspace, clear, and connect actions.
- Connection progress and results are reported in the UI.
- The complete flow—scanning, selection, password entry, and connection—was tested on the blue Retro Pixel Pocket.
- A saved-network switching test exposed a ConnMan D-Bus `NoReply` timing case: the connection could complete at the wireless layer even though the request callback timed out.
- The helper now polls the requested service after `NoReply` and reports success only when ConnMan reaches `ready` or `online`; it does not reinterpret `NoReply` as an authentication failure.
- Switching between the two saved home networks was re-tested successfully on the blue device on November 19, 2026.
- Deployed and verified November 18–19, 2026. The workflow is frozen unless real-world use exposes another problem.

Parent Mode should remain small. Media-library administration and broader configuration belong in the browser management interface.

## 4. Phone/computer management interface

The phone or computer should be the primary place where parents configure and manage KiddOS. **The handheld is the player; the browser is the management console.**

The intended management experience is a local web application served by the handheld over HTTP on the local network. It should work without internet access when the phone/computer and handheld share a LAN.

### Management capabilities

The browser interface is intended to handle:

- Uploading and organizing games, music, and movies.
- Browsing the stored media library.
- Assigning existing library items to the featured choices available in each child-facing experience.
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

## 8. Historical implementation status snapshot

The following records the state documented during the earlier device-first development phase. It is not a complete description of the current frontend, hardware service, or management server. For current KiddOS software status, see `README.md` and `docs/ARCHITECTURE.md`. The hardware-service deployment record is in `docs/HARDWARE-SERVICE-MIGRATION.md`.


### Verified on the blue Retro Pixel Pocket

- SDL frontend launches through the existing firmware startup mechanism.
- Four-game selection screen and Selected Game screen work.
- D-pad, A, B, Start, and game-exit controls work as intended.
- Games launch through the existing firmware and return to KiddOS.
- System menu and shutdown confirmation work.
- Battery percentage and charging status are displayed.
- Wi-Fi connection status is displayed beside the battery indicator.
- Parent Mode Wi-Fi Settings reports radio power/connection state and turns Wi-Fi on/off through ConnMan.
- The device has connected to the home Wi-Fi network.
- MPV has played a 720 × 720 H.264/AAC test clip with sound.
- Hidden, PIN-protected Parent Mode is deployed and working.
- Parent Mode can start/stop the LAN management server through the installed control script.
- Remote Management can be disabled without disconnecting Wi-Fi; continuous ping remained active during testing.
- Change PIN flow is deployed and working.
- Wi-Fi power controls and the Wi-Fi indicator are deployed and working on the blue device.
- Remote Management was re-tested after Wi-Fi was switched off and restored; the controls remain independent.
- Saved-network switching was re-tested successfully after correcting delayed ConnMan `NoReply` handling.
- The SDL frontend source has been reorganized into focused battery, game-launch, Wi-Fi, parent-management, and power services, with shared models separated from the UI controller.

### Not yet implemented or fully validated

- Further Wi-Fi edge cases and automatic reconnection behavior, deferred unless real-world use exposes a problem.
- Full media-library administration beyond the currently implemented game catalog, artwork, and active-slot workflows.
- Browser-based theme customization.
- Automatic network priority/fallback behavior.
- Phone-hotspot fallback.
- IBSS/ad-hoc direct networking.
- Multiplayer workflows.
- Full-length movie playback and a complete music experience.
- Validation on the purple and green handhelds.

The current frontend is a working prototype validated on one blue device. Do not assume its startup hook or deployment procedure is ready for the other devices without reviewing device paths and recovery steps.

## 9. Priorities and deferred work

### Immediate KiddOS priority

- Keep the existing blue-device experience stable while preparing the blue, purple, and green devices for practical family use.
- Improve the child-facing game selection and launch experience only where it reduces friction for the children.
- Keep parent management reliable and straightforward.
- Validate the actual family workflows before adding more infrastructure.
- Protect the working stock-firmware compatibility path.

### Foundation work — separate, longer-term workstream

- Define and document the public boundary between applications and device/platform capabilities.
- Inventory existing firmware dependencies, hardware assumptions, and licensing/provenance.
- Make the Foundation build reproducible from documented prerequisites and legally distributable inputs.
- Keep the Foundation independently useful and buildable without KiddOS.
- Treat a flashable image as a Foundation deliverable, not a gate on KiddOS product progress.

### Deferred product capabilities

- Music, movie, and TV library browsing and child-facing playback experiences.
- Parent-computer content preparation, conversion, metadata/artwork resolution, and synchronization.
- Phone-hotspot fallback and automatic network priority switching.
- IBSS/ad-hoc and other direct device-to-device experiments.
- Full multiplayer pairing and game-specific testing.
- Advanced theme editor and broad device configuration.
- Emulator/core management in the remote management interface. This is post-MVP; retain the current intentional Gambatte prototype behavior for now.

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

## Historical snapshot: Management Server v0.3.0

This section preserves the earlier v0.3.0 read-only inventory milestone. It is superseded by the current management-server implementation, which includes game-catalog management, artwork workflows, and live slot updates. See `README.md` and `docs/ARCHITECTURE.md` for current KiddOS state. The earlier firmware-image investigation is retained as historical research in `docs/CUSTOM-IMAGE-ARCHITECTURE.md`.


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

### Distribution and licensing guardrails

A reproducible flashable image remains a Foundation goal, separate from the immediate KiddOS application work.

Before distributing an image or bundled software, audit the exact source revisions and every component. Preserve third-party licenses, notices, and source-offer obligations. Distinguish the right to use or build software from the right to redistribute vendor binaries, firmware, ROMs, BIOS files, or artwork. Do not bundle content without explicit redistribution rights.
