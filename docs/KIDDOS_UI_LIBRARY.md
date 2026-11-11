# KiddOS UI Library

Reusable C++20 SDL2 interface library for the KiddOS application.

The library provides generic interface primitives and screen navigation.
Game catalogs, emulator behavior, parental policy, and device-specific
services belong in application/platform layers rather than this library.

## Build and install

From the repository root:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure
    cmake --install build --prefix ./install

Include the public API with:

    #include <kiddos/kiddos_ui.hpp>

The CMake build-tree alias is `KiddOS::UI`. The library target is
`kiddos_ui`.

## Main concepts

### Elements and containers

`Element` is the base class for drawable interface elements. It provides
bounds, visibility, enabled state, and focus state.

`Container` owns its child elements. Add widgets using its typed `add`
function:

    auto& button = container.add<kiddos::ui::Button>("Play");

The returned reference remains owned by the container. Do not delete it
or retain it after its owning container is destroyed.

### Widgets

The library currently includes:

- Button
- Image
- Label
- Panel
- ProgressBar
- Toggle
- Slider
- Checkbox
- RadioGroup
- Select
- TextInput
- ListView

Widgets expose their own configuration and rendering behavior. Use
application-level code to decide what a widget means or what action it
performs.

`Image` supports per-instance opacity from 0 (transparent) to 255 (opaque).
`PanelStyle::corner_radius` controls rounded panel corners; zero preserves
square corners. Negative radii are clamped to zero.

### Focus and controller actions

`FocusManager` manages focus among eligible elements in a screen's root
container. It supports initial focus and directional movement.

`UIAction` represents abstract interface actions:

- `Left`
- `Right`
- `Up`
- `Down`
- `Confirm`
- `Back`

`ControllerInteraction` dispatches those actions to the focused interface
element. A Button can provide an activation callback with
`set_on_activate`.

### Screens

Derive application screens from `kiddos::ui::Screen`. Add their widgets to
`root()` and implement lifecycle hooks as needed.

Example:

    #include <kiddos/kiddos_ui.hpp>

    class HomeScreen final : public kiddos::ui::Screen {
    public:
        void on_enter() override {
            auto& play = root().add<kiddos::ui::Button>("Play");
            play.set_on_activate([] {
                // Notify the application that Play was selected.
            });

            focus().focus_first();
        }

        void update(float delta_seconds) override {
            // Update screen-specific presentation state.
            (void)delta_seconds;
        }
    };

The callback should communicate intent to the application layer. Avoid
putting game launching, emulator selection, or parental-policy decisions
inside generic widgets.

### ScreenManager

`ScreenManager` owns screens and provides stack-based navigation.

Example:

    kiddos::ui::ScreenManager screens;
    screens.push(std::make_unique<HomeScreen>());

    // Route abstract controller actions from the platform adapter.
    screens.handle_action(kiddos::ui::UIAction::Down);
    screens.handle_action(kiddos::ui::UIAction::Confirm);

    // Call from the application's update/render loop.
    screens.update(delta_seconds);
    screens.render(renderer);

The application must supply its actual `Renderer` instance and delta time.
The example illustrates the API; it is not a standalone SDL application.

Navigation behavior:

- `push` suspends the previous screen and enters the new screen.
- `pop` exits the active screen and resumes the previous screen.
- `replace` exits the active screen and enters its replacement.
- `clear` exits all screens.
- `Back` is dispatched to the active screen's controller interaction.

The manager owns the screens passed to it. Do not separately delete or
reuse those screen objects after transferring ownership.

### Text input

The library includes `TextInput` and `TextInputInteraction`. Platform code
should translate native keyboard/text events into the platform-neutral
text-input event types and dispatch them to the active screen.

Keep SDL event polling and device-specific input translation in the
platform adapter, not in individual widgets.

## Ownership and responsibilities

- The application owns the SDL lifecycle and renderer lifetime.
- `ScreenManager` owns its screen instances.
- Each screen owns its widget tree through its root container.
- Containers own the elements added to them.
- The platform adapter translates physical input into abstract UI actions
  and text-input events.
- Application services own game catalog, launch, save-state, and parental
  management behavior.

## Testing

Run the full suite after changes:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

The tests cover UI components, image opacity, rounded-panel rendering,
focus/controller behavior, text input, and screen-manager lifecycle/navigation.

## Scope and limitations

This is a reusable UI foundation, not a complete operating-system shell.
It does not implement a game catalog, emulator integration, parental
controls, persistent settings, or hardware-specific input/display services.
Those are responsibilities of future KiddOS application and platform work.
