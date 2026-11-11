#include "kiddos/ui/controller.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Container root;
    auto& play = root.add<Button>("Play");
    auto& settings = root.add<Button>("Settings");
    auto& watch = root.add<Button>("Watch");

    play.set_bounds({0, 0, 200, 60});
    settings.set_bounds({220, 0, 200, 60});
    watch.set_bounds({440, 0, 200, 60});

    int play_count = 0;
    int settings_count = 0;
    play.set_on_activate([&] { ++play_count; });
    settings.set_on_activate([&] { ++settings_count; });

    FocusManager focus(root);
    ControllerInteraction controller(focus);

    assert(focus.focus_first());
    assert(focus.focused() == &play);

    assert(controller.handle(UIAction::Right));
    assert(focus.focused() == &settings);

    assert(controller.handle(UIAction::Confirm));
    assert(settings_count == 1);
    assert(play_count == 0);

    assert(controller.handle(UIAction::Left));
    assert(focus.focused() == &play);

    assert(controller.handle(UIAction::Confirm));
    assert(play_count == 1);

    assert(!controller.handle(UIAction::Right) == false);
    assert(focus.focused() == &settings);

    settings.set_enabled(false);
    assert(controller.handle(UIAction::Left));
    assert(focus.focused() == &play);
    assert(controller.handle(UIAction::Right));
    assert(focus.focused() == &watch);

    assert(!controller.handle(UIAction::Confirm));

    assert(!controller.handle(UIAction::Back));
    assert(focus.focused() == &watch);

    return 0;
}
