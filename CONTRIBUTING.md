# Contributing to Retro KiddOS

## Project mission

Retro KiddOS aims to make the FunnyPlaying Retro Pixel Pocket a simple,
child-friendly, offline entertainment device centered on games. The product
direction may also include music, movies, and TV, while keeping local use
independent of Wi-Fi. Children should be able to enjoy their content with
minimal adult help, at home and while traveling.

Family multiplayer is an important part of the gaming goal.

## Project direction

Retro KiddOS and Retro Pixel Foundation are independent projects. Their
responsibilities and transition rules are documented in
[docs/PROJECT-BOUNDARIES.md](docs/PROJECT-BOUNDARIES.md).

### Retro KiddOS

KiddOS is the child-facing product. It owns:

- The frontend, navigation, and presentation.
- Game and future media library experiences.
- Content organization, metadata, artwork, and curation.
- Child profiles, favorites, visibility, and content assignments.
- Parent-facing management and content-preparation workflows.
- Product-specific settings, PIN workflows, and policies.

The immediate priority is a reliable, simple experience on the existing
stock firmware and hardware. Preserve working behavior and validate
meaningful changes on the actual device.

### Retro Pixel Foundation

Foundation is the reusable platform beneath applications and frontends.
It owns:

- Bootloader, kernel, device tree, and hardware support.
- Linux userspace, system services, and hardware capabilities.
- Gaming runtime and emulator integration.
- Documented interfaces for applications and frontends.
- Reproducible image builds and dependency provisioning.
- Source provenance, licensing, redistribution, and developer documentation.

Foundation must be independently buildable and must not depend on KiddOS.
A developer should be able to use it with a different frontend.

### Transition and compatibility

The current KiddOS implementation uses stock RetroPixel firmware facilities.
Keep that compatibility path working while Foundation is developed.

Do not move or replace working components merely to make the architecture
look cleaner. Separate interfaces from implementations incrementally, with
tests and device validation appropriate to each change.

Foundation development must not unnecessarily delay practical KiddOS
progress. A complete Foundation image is a separate milestone, not a
prerequisite for the immediate KiddOS product.

### Build and contribution expectations

Keep KiddOS application builds separate from Foundation image builds.
Document all prerequisites and dependencies; do not rely on private archives,
undocumented local workspaces, or hidden state.

Preserve the existing firmware, kernel, hardware research, local backups,
dirty working trees, and device recovery paths. Never use destructive
cleanup as routine investigation.

Our guiding principle:

> Every hour we spend developing should make it easier for you and your
> kids to play games together. If it doesn't, we probably shouldn't be
> building it.

If a task does not improve the children's experience or materially advance
the reusable Foundation, question whether it needs to be done.

The original desktop SDL application is archived project history. The
device-first SDL frontend is the active KiddOS development path.

## Development principles

- Prioritize improvements that make it easier for children to enjoy the device.
- Prefer small, reviewable changes.
- Reuse existing firmware capabilities before writing custom subsystems.
- Validate gameplay and relevant workflows on actual hardware.
- Preserve existing game files, media, and saves.
- Avoid destructive device operations without explicit approval.
- Document meaningful decisions, tests, and limitations.
- Do not assume the tested blue-device setup is portable to other devices.
- Do not expand scope just because a feature is technically interesting.
- Avoid speculative abstractions and frameworks without a concrete user need.

## Before submitting changes

- Build using the documented device build process.
- Run the architecture tests.
- Test the affected behavior.
- Test on actual hardware when relevant.
- Update documentation when behavior or requirements change.

## License

The project is licensed under the GNU General Public License v3.0 or later.
See `LICENSE`.
