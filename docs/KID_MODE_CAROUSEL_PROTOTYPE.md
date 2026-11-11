# Kid Mode Carousel Prototype

## Purpose

This is the first visible implementation of the Retro KiddOS child-facing
Kid Mode screen. It runs in the temporary Arch Linux SDL2 development
harness and is not a device firmware build.

## Implemented

- Logical 720 × 720 display.
- Dark navy background and bright blue selection treatment.
- Three sample game cards with generated placeholder artwork.
- Large centered selected game with smaller neighboring previews.
- Left/right keyboard and controller navigation.
- Wrapping selection across the three sample games.
- Sound-status and battery-status visual indicators.
- The home carousel is rendered through the KiddOS UI library as a
  `HomeCarouselScreen` (`ui::Screen`) managed by a `ScreenManager` in the
  application, reusing the generated artwork and preserving the established
  geometry and colors.
- The game action screen remains on the platform-owned manual rendering
  path, with selection state managed by the application.
- A KiddOS wordmark centered at the top of the screen.
- The selected game's title centered directly below the main card,
  following the selection (SUPER MARIO, STAR, CAR).
- Physical control hints along the bottom of the screen: browse with
  left/right, select with A, and hold START + SELECT for the parent menu.
- Clean rounded card borders: the UI renderer traces continuous corner
  arcs, and panels draw a uniform single-pass border.

## Desktop controls

- Left / Right: change selected game.
- Controller D-pad Left / Right: change selected game.
- Escape or window close: exit.

## Scope and limitations

The three games and their artwork are placeholders. This milestone does not
implement game discovery, game launching, emulator integration, real audio
status, battery telemetry, Parent Center, or device-specific input and
display integration.

The desktop harness is a development aid only. Visual and input behavior
must be verified on the actual Retro Pixel Pocket before device integration.

## Validation

The SDL2 application compiled successfully and the preview process completed.
The implementation remains subject to visual review and subsequent
hardware-specific verification.
