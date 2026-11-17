# Input Controls

This document describes the behavior verified in the current
device-first SDL frontend.

## KiddOS frontend

- D-pad: navigate the game carousel and menus.
- A: confirm the selected action.
- B: back/cancel where supported.
- Start: open the system menu.
- Select + Start: exit a running game and return to KiddOS.

## During gameplay

Normal game controls are passed to the emulator.

The Select + Start shortcut is configured as the game-exit combination.
It was tested with the current RetroArch configuration and tested games.

## System menu

The current frontend provides a system menu with a shutdown option
and a confirmation step before powering off the device.

## Scope and limitations

These controls describe the tested implementation, not a promise that
every emulator core or future firmware configuration behaves identically.

Parent PIN controls, a parent-management interface, and a universal
child-friendly exit-confirmation flow are not currently implemented.
