# Contributing to Retro KiddOS

## Project direction

Retro KiddOS is a child-friendly gaming frontend for the FunnyPlaying
Retro Pixel Pocket.

The active implementation is device-first:
- Reuse the device's existing Linux firmware.
- Reuse its emulator, RetroArch, controller, and game-launching systems.
- Keep the frontend simple and focused on choosing and playing games.
- Avoid rebuilding functionality the firmware already provides.

The original desktop SDL application is archived project history.
It is not the active product or build target.

## Development principles

- Prioritize improvements that make it easier for children to play.
- Prefer small, reviewable changes.
- Verify behavior on actual hardware when relevant.
- Preserve existing game files and saves.
- Avoid destructive device operations without explicit approval.
- Document meaningful decisions, tests, and limitations.
- Do not assume the tested blue-device setup is portable to other devices.

## Before submitting changes

- Build using the documented device build process.
- Test the affected behavior.
- Test on actual hardware when relevant.
- Update documentation when behavior or requirements change.

## License

A project license has not yet been selected.
