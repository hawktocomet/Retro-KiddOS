# KiddOS Project Vision

## Purpose

KiddOS is a child-friendly gaming environment for the FunnyPlaying Retro
Pixel Pocket. It is designed to make a handheld gaming device simple and
safe for young children, with adult management controls.

A young child should be able to pick up the device, turn it on, choose a
game, play, and return to the game selection screen without needing an adult
to explain the interface.

## Product model

KiddOS has one child-facing gaming experience and a separate parent
management area.

### Kid Mode

Kid Mode is the default experience. It is a deliberately simple,
physical-control-first interface showing only games and features approved
by a parent.

The child should be able to:

- Browse available games.
- Start or resume a game.
- Start a game over where supported.
- Return safely to the game selection screen.
- Understand important device messages through simple visual communication.

### Parent Center

Parent Center is a PIN-protected area for adults to configure and manage the
child's KiddOS experience.

Initial responsibilities include:

- Choosing which installed games are visible to the child.
- Managing game artwork and gameplay previews.
- Managing save data and automatic checkpoints.
- Organizing games into optional collections.
- Configuring display, volume, and child-facing features.

Potential future responsibilities include playtime limits, schedules,
and additional parental controls.

Parent Center is not a separate adult gaming mode. KiddOS does not need to
expose the original EmuELEC frontend as an alternative user experience.

## Design principles

- Simple enough for a preschool-aged child.
- Designed for a 720 × 720 square display.
- Physical controls are the primary input.
- Visual communication before written instructions.
- Large, clear selections.
- Predictable physical-button behavior.
- Parent authority over the child-facing library and features.
- Existing game files and saves are preserved when games are hidden.
- Offline operation for the core experience.
- Hardware-specific behavior must be tested, not assumed.

## Scope

The FunnyPlaying Retro Pixel Pocket is the sole initial target.

The Arch Linux development environment is a temporary engineering host, not
a supported desktop product. Other handhelds and general desktop editions
are outside the initial scope.

KiddOS is a separate project from DualOS.

## Source of truth

See `PRODUCT_SPECIFICATION.md` for the consolidated product requirements.
Supporting design documents define detailed behavior.
