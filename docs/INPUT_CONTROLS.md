# Input Controls

## Kid Mode

- Left / Right: browse games.
- Up / Down: choose a game action.
- A: confirm.
- B: passed through as the normal in-game B button.
- X / Y / L / R / START / SELECT: normal in-game controls, except for
  the universal START + SELECT shortcut.

## Universal shortcut

START + SELECT held continuously for approximately two seconds:

- During gameplay: request the child-friendly exit confirmation.
- On the game carousel: open the parent PIN screen.

A short press of either button individually must not trigger the shortcut.

The shortcut must ultimately work across supported emulators. Its final
implementation depends on auditing EmuELEC input handling and emulator
integration; desktop input handling alone is not proof of handheld support.

## Exit confirmation

The child can choose:

- Yes, return to games.
- No, keep playing.

The game should close gracefully when exit is confirmed, and the carousel
should remember the previously selected game.
