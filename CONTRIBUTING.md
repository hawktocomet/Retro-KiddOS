# Contributing to Retro KiddOS

## Project mission

Retro KiddOS aims to make the FunnyPlaying Retro Pixel Pocket a simple,
child-friendly, offline gaming device. Children should be able to choose and
play their games independently at home and while traveling, with minimal
adult help and without depending on Wi-Fi.

Family multiplayer is an important part of the gaming goal.

## Project direction

The active implementation is device-first:

- Reuse the device's existing Linux firmware.
- Reuse its emulator, RetroArch, controller, and game-launching systems.
- Keep the child-facing gaming experience simple and focused.
- Prefer existing firmware playback capabilities for other media.
- Avoid rebuilding functionality the firmware already provides.
- Prefer a small, reliable solution over a large custom operating system.

Our guiding principle:

> Every hour we spend developing should make it easier for you and your
> kids to play games together. If it doesn't, we probably shouldn't be
> building it.

If a task does not improve the children's experience, question whether
it needs to be done.

The original desktop SDL application is archived project history.
The device-first SDL frontend is the active development path.

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
