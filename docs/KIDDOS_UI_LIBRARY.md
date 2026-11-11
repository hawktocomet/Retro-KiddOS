# KiddOS UI Library

Reusable C++20 SDL2 interface library for handheld applications.

## Build and install

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure
    cmake --install build --prefix ./install

Include the complete public API with `#include <kiddos/kiddos_ui.hpp>`.
The CMake build-tree alias is `KiddOS::UI`.

## Screens

Derive from `kiddos::ui::Screen`, add widgets to `root()`, and override
lifecycle hooks or `update()` as required. `ScreenManager` owns screens
and provides push, pop, replace, clear, update, render, controller-action
dispatch, and text-input dispatch.

Pushing suspends the previous screen. Popping exits the active screen and
resumes the previous one. Replacing exits the active screen. Back pops
the stack. Abstract platform events are routed to the active screen.

Containers own child elements. ScreenManager owns screens. The application
remains responsible for SDL initialization and renderer lifetime.

## Widgets

Button, Image, Label, Panel, ProgressBar, Toggle, Slider, Checkbox,
RadioGroup, Select, TextInput, and ListView.

## Scope

The library contains no game catalog, emulator, game policy, or parental
control logic. Those belong to application layers built on top of the UI.
