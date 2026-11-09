# KiddOS Project Vision

## Purpose

KiddOS is designed to make a handheld gaming device simple and safe for
young children, with adult management controls.

It should let a young child pick up the device, turn it on, choose a game,
play, and return to the game selection screen without needing an adult to
explain the interface.

## Product model

KiddOS has one child-facing gaming experience and a separate parent
management area.

### Child experience

A deliberately simple, visual, controller-only interface showing only
games and features approved by a parent.

The child should be able to:

- Browse available games.
- Start or resume a game.
- Start a game over where supported.
- Return safely to the game selection screen.
- Understand important device messages through simple visual communication.

### Parent management

A PIN-protected area for adults to configure and manage the child's
KiddOS experience.

Initial responsibilities include:

- Choosing which installed games are visible to the child.
- Managing game artwork and gameplay previews.
- Organizing games into optional collections.
- Configuring available child-facing features and settings.

Potential future responsibilities include playtime limits, schedules,
and additional parental controls.

Parent management is not a separate adult gaming mode. KiddOS does not
need to expose the original EmuELEC frontend as an alternative user
experience.

## Design principles

- Simple enough for a preschool-aged child.
- Visual communication before written instructions.
- Large, clear selections.
- Predictable physical-button behavior.
- Parent authority over the child-facing library and features.
- Existing game files and saves are preserved when games are hidden.
- Offline operation for the core experience.
- Hardware-specific behavior must be tested, not assumed.

## Scope

The first target is the Retro Pixel Pocket. Other handhelds are outside the
initial implementation scope.
