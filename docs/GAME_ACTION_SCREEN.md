# Game Selection and Action Screen

## Implemented

- Selecting a carousel game with Confirm opens its action screen.
- Static game artwork remains visible.
- Actions use large stacked horizontal selection boxes.
- First-launch games show Play.
- The checkpoint demonstration fixture shows Resume and Start Over.
- Up/Down changes the selected action.
- Back returns to the carousel.
- The selected carousel game is preserved when returning.

## Prototype limitations

Game 1 is temporarily used as a checkpoint demonstration fixture.
This is not real save-state detection.

Confirming an action does not launch an emulator or modify saved data.
Game launching, checkpoint discovery, resume, and restart behavior require
later implementation and emulator integration.

The desktop harness does not establish handheld compatibility.
