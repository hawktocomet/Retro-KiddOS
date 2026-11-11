# KiddOS UI Library

## Purpose

KiddOS UI is a first-party C++20 interface library built on SDL2.

SDL2 remains responsible for low-level window, graphics, input, and audio
integration. KiddOS UI owns the reusable interface abstractions and
components.

## Current foundation

- Geometry: Point and Rect.
- Color: RGBA values and initial theme colors.
- Renderer: non-owning SDL_Renderer adapter.
- Drawing: clear, filled rectangles, outlines, lines, and points.
- Clipping: set and clear rectangular clipping regions.
- Tests: geometry boundary behavior.

## Ownership

Renderer does not own SDL_Renderer. The platform adapter creates and
destroys SDL resources. This keeps hardware and SDL lifecycle management
outside the reusable UI library.

## Planned modules

1. Rendering and drawing primitives.
2. Layout and containers.
3. Widget and element hierarchy.
4. Buttons, cards, lists, and menus.
5. Controller focus and navigation.
6. Themes, fonts, images, and animation.

## Design principles

- Handheld-first and physical-control-first.
- 720 x 720 logical coordinate system.
- No dependency on LVGL.
- No application-specific game logic inside the library.
- Reusable components instead of manually drawing every screen.
- Keep memory and rendering overhead appropriate for the target device.
