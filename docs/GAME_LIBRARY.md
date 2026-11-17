# Game Library

## Current prototype

The tested blue-device prototype currently presents four games:

- Mario Tennis
- Pokemon Red
- Super Mario Bros. Deluxe
- Tetris

The game list and artwork are currently configured directly in the
frontend. This is a prototype configuration, not yet a parent-managed
library.

## Intended direction

The long-term goal is for parents to choose which installed games appear
in the child-facing KiddOS library.

Hiding a game should never:
- Uninstall the game.
- Delete its ROM or game files.
- Delete native in-game saves.
- Delete save states or automatic checkpoints.

The library should favor games that children can realistically play,
with simple controls, forgiving gameplay, and minimal reading.

Parent-managed collections may eventually organize games by age,
difficulty, or progression.

## Current limitations

A parent interface, game discovery workflow, editable library, and
automated artwork management are not yet implemented.
