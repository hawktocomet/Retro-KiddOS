# Retro KiddOS and Retro Pixel Foundation

## Purpose

This document establishes the architectural boundary between two
independent projects:

1. Retro KiddOS: a child-friendly gaming and media experience.
2. Retro Pixel Foundation: a reusable, reproducible platform for the
   FunnyPlaying Retro Pixel Pocket hardware.

The projects are related, but neither may depend on the other's
implementation.

The Foundation must be useful without KiddOS. KiddOS must be able to
run on the existing stock firmware while the Foundation is developed.

## Guiding principle

Every hour spent developing should make it easier for the family to use
the devices, or materially improve the reusable platform that makes
those experiences possible.

Foundation development must not unnecessarily delay practical KiddOS
progress.

## 1. Retro KiddOS responsibilities

KiddOS owns the user experience and product-specific behavior.

This includes:

- Child-facing navigation, screens, and presentation.
- Game, music, movie, and television experiences.
- Content organization, metadata, artwork, and curation.
- Child profiles, favorites, visibility, and navigation complexity.
- Game slots and other child-facing content assignments.
- Parent-facing management workflows.
- Content preparation workflows and parent-computer companion tools.
- Product-specific settings, PIN workflows, and policies.
- KiddOS application packaging and deployment.

KiddOS decides what content is presented and how people interact with it.

KiddOS must not require Foundation to understand its product concepts,
such as child profiles, game slots, collections, or parental policies.

## 2. Retro Pixel Foundation responsibilities

Foundation owns the reusable hardware and software platform.

This includes:

- Bootloader, kernel, device tree, and hardware support.
- Linux operating system and userspace.
- System configuration and service lifecycle.
- Hardware input and display capabilities.
- Brightness, audio, volume, and battery capabilities.
- Network and power-management capabilities.
- Gaming runtime and emulator integration.
- Documented interfaces for applications and frontends.
- Build system, dependency provisioning, and image assembly.
- Source provenance, licensing, and redistribution documentation.
- Developer documentation and platform maintenance.

Foundation must be independently buildable and usable without KiddOS.

A developer must be able to create or install a different frontend without
needing to adopt KiddOS source code or product assumptions.

## 3. Shared and transition responsibilities

The current implementation runs on stock RetroPixel firmware. Some
components therefore combine product behavior with firmware-specific
integration.

These components are transitional:

- Stock firmware startup integration.
- Stock firmware application and service adapters.
- Current game-launch integration through runemu.py.
- Current Wi-Fi integration through ConnMan.
- Current battery, power, and audio hardware access.
- Device-side management server integration.
- Current deployment and compatibility scripts.

During the transition, these components must preserve the working stock
firmware experience.

Where practical, product-facing interfaces should be separated from
their stock firmware implementations. Foundation will eventually provide
native implementations of the same documented capabilities.

The transition layer is not intended to become a permanent dependency
between the two projects.

## 4. Application and platform interfaces

KiddOS should consume documented platform capabilities rather than
depend directly on hardware-specific paths, commands, or implementation
details.

The initial capability areas are:

- Device identity and status.
- Battery and charging status.
- Network status and configuration.
- Power operations.
- Input and display capabilities.
- Audio and volume capabilities.
- Content launch and runtime capabilities.

These are architectural interface areas, not a commitment to implement
a large abstraction framework immediately.

Interfaces should be introduced when needed, remain small, and have
documented behavior and error handling.

The stock firmware adapter may implement these interfaces using
existing firmware facilities. Foundation will provide native
implementations when available.

## 5. Content management boundary

KiddOS owns the content library and the rules governing what children
can see and use.

Foundation provides the runtime capabilities required to use supported
content. It does not own child-facing catalog policy.

Content preparation should primarily happen on the parent's computer,
not on the resource-constrained handheld.

Future parent-computer tooling may handle:

- Import and organization.
- Metadata and artwork resolution.
- Format conversion and device compatibility checks.
- Device-ready copies.
- Assignment and synchronization.

The handheld should primarily browse, launch, play, and report status.

Content should not become child-assignable until it is both usable by the
target device and has sufficient recognizable metadata and artwork.

These are product-level rules, not Foundation responsibilities.

## 6. Emulator and runtime boundary

Foundation should expose documented runtime capabilities and supported
launch operations.

KiddOS should request that supported content be launched without
depending on a particular emulator implementation.

The current Gambatte integration is an intentional prototype decision.
It must not be replaced or generalized merely to anticipate future
runtime management.

Emulator/core management is deferred until after the current MVP. If
implemented, it should reuse existing distribution infrastructure where
technically and legally appropriate.

## 7. Build and distribution independence

Foundation and KiddOS must have separate build responsibilities.

Foundation build responsibilities:

- Provision documented build prerequisites.
- Obtain or build documented source dependencies.
- Assemble the operating system and runtime.
- Produce a reproducible device image.
- Document licenses, provenance, and redistribution restrictions.

KiddOS build responsibilities:

- Build the application and product components.
- Package product assets and configuration.
- Build stock firmware compatibility components where needed.
- Build and validate the Foundation-facing application integration.

A Foundation image must not require KiddOS to build successfully.

A KiddOS build must not require an undocumented local workspace or
private dependency archive.

## 8. Compatibility and preservation rules

Until a replacement has been demonstrated and approved:

- Preserve the working stock firmware startup path.
- Preserve the current deployed blue-device experience.
- Do not deploy experimental changes to other devices without approval.
- Do not replace the current game launcher or emulator configuration
  without a demonstrated need.
- Do not remove existing firmware, kernel, or hardware research.
- Do not discard historical backups or forensic artifacts as cleanup.
- Do not overwrite existing build outputs without inspecting them.
- Do not make destructive repository or device changes as part of
  architectural cleanup.

The stock firmware implementation remains a supported compatibility
target during Foundation development.

## 9. Migration sequence

The intended order is:

1. Document project ownership and boundaries.
2. Inventory current implementations and dependencies.
3. Define the smallest useful application-facing platform interfaces.
4. Document stock firmware implementations of those interfaces.
5. Establish Foundation source provenance, dependency, and license
   requirements.
6. Establish a reproducible Foundation build.
7. Build and validate Foundation independently of KiddOS.
8. Implement and validate native Foundation application integration.
9. Verify KiddOS on both stock firmware and Foundation.
10. Demonstrate that an independent frontend can use Foundation without
    depending on KiddOS.

No source-code migration is implied by this sequence. Each implementation
change requires its own review and validation.

## 10. Definition of architectural success

The separation is successful when:

- Foundation can be cloned, built, and used without KiddOS.
- Foundation documentation identifies all required inputs and
  dependencies.
- A developer can create a different frontend using documented
  Foundation interfaces.
- KiddOS can run against Foundation without importing Foundation
  implementation details.
- KiddOS can continue to run on the stock firmware compatibility path.
- Product-specific content and parental policies remain in KiddOS.
- Hardware-specific implementation details remain in Foundation.
- Neither project relies on undocumented files or hidden local state.

## Current status

This document establishes intended ownership and direction. It does not
claim that the current repository already satisfies these boundaries.

The existing implementation remains the compatibility baseline until
individual replacements have been built, tested, and approved.
