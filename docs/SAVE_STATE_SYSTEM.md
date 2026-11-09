# Save-State and Resume System

## First launch

Show Play only.

## Subsequent launches

Offer Resume and Start Over when a valid automatic checkpoint exists.

## Exit shortcut

When the child requests an exit, KiddOS should attempt to create an automatic
checkpoint before returning to the carousel.

## Save preservation

Automatic checkpoints and native in-game saves are separate.

Start Over should clear only the automatic resume checkpoint, not erase
native in-game saved progress.

## Compatibility

Save-state creation and restoration depend on emulator support. Failures
must be handled safely and must not be represented as successful saves.
The implementation must be tested per emulator.
