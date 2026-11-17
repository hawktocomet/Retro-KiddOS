# Contributing to Retro KiddOS

## Project mission

Retro KiddOS aims to make the FunnyPlaying Retro Pixel Pocket a simple,
child-friendly, offline entertainment device for games, music, and movies.
Children should be able to enjoy their favorites at home and while traveling,
with minimal adult help and without depending on Wi-Fi.

Family multiplayer is an important part of the gaming goal.

## Project direction

The active implementation is device-first:
- Reuse the device's existing Linux firmware.
- Reuse its emulator, RetroArch, controller, and game-launching systems.
- Investigate and reuse existing local audio/video playback tools.
- Keep the child-facing experience simple and focused.
- Avoid rebuilding functionality the firmware already provides.
- Prefer a small, reliable solution over a large custom operating system.

The guiding principle is:

> Every hour we spend developing should make it easier for you and your
> kids to play games together. If it doesn't, we probably shouldn't be
> building it.

More broadly, development should make it easier for the kids to enjoy
their own games, music, and movies. If a task does not improve that
experience, question whether it needs to be done.

The original desktop SDL application is archived project history.
It is not the active product or build target.

## Development principles

- Prioritize improvements that make it easier for children to enjoy the device.
- Prefer small, reviewable changes.
- Reuse existing firmware capabilities before writing custom subsystems.
- Validate media and gameplay workflows on actual hardware when relevant.
- Preserve existing game files, media, and saves.
- Avoid destructive device operations without explicit approval.
- Document meaningful decisions, tests, and limitations.
- Do not assume the tested blue-device setup is portable to other devices.
- Do not expand scope just because a feature is technically interesting.

## Before submitting changes

- Build using the documented device build process.
- Test the affected behavior.
- Test on actual hardware when relevant.
- Update documentation when behavior or requirements change.

## License

A project license has not yet been selected.
