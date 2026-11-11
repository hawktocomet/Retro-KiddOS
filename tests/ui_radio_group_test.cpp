#include "kiddos/ui/radio_group.hpp"
#include "kiddos/ui/controller.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
    using namespace kiddos::ui;

    RadioGroup group({"Play", "Watch", "Settings"}, 1);
    assert(group.focusable());
    assert(group.selected() == 1);
    assert(group.options().size() == 3);

    int callback_count = 0;
    int callback_value = -1;
    group.set_on_change([&](int value) {
        ++callback_count;
        callback_value = value;
    });

    assert(group.select_next());
    assert(group.selected() == 2);
    assert(callback_count == 1 && callback_value == 2);

    assert(group.select_next());
    assert(group.selected() == 0);

    assert(group.select_previous());
    assert(group.selected() == 2);

    assert(group.set_selected(1));
    assert(callback_value == 1);
    assert(!group.set_selected(99));

    group.set_enabled(false);
    assert(!group.select_next());
    group.set_enabled(true);
    group.set_visible(false);
    assert(!group.select_previous());
    group.set_visible(true);

    group.set_options({"One", "Two"});
    assert(group.selected() == 0);
    group.set_options({});
    assert(group.selected() == -1);
    assert(!group.select_next());

    Container root;
    auto& controlled = root.add<RadioGroup>(
        std::vector<std::string>{"A", "B", "C"}, 0);
    FocusManager focus(root);
    ControllerInteraction controller(focus);
    assert(focus.focus_first());
    assert(controller.handle(UIAction::Right));
    assert(controlled.selected() == 1);
    assert(controller.handle(UIAction::Down));
    assert(controlled.selected() == 2);
    assert(controller.handle(UIAction::Left));
    assert(controlled.selected() == 1);

    return 0;
}
