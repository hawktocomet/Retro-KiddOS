#include "kiddos/ui/toggle.hpp"
#include "kiddos/ui/controller.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Toggle toggle;

    assert(!toggle.checked());
    assert(toggle.focusable());

    int callback_count = 0;
    bool callback_value = false;

    toggle.set_on_change([&](bool value) {
        ++callback_count;
        callback_value = value;
    });

    assert(toggle.toggle());
    assert(toggle.checked());
    assert(callback_count == 1);
    assert(callback_value);

    toggle.set_checked(true);
    assert(callback_count == 1);

    toggle.set_checked(false);
    assert(!toggle.checked());
    assert(callback_count == 2);
    assert(!callback_value);

    toggle.set_enabled(false);
    assert(!toggle.toggle());
    assert(!toggle.checked());

    toggle.set_enabled(true);
    toggle.set_visible(false);
    assert(!toggle.toggle());

    toggle.set_visible(true);

    ToggleStyle style;
    style.off = {1, 2, 3, 255};
    style.on = {4, 5, 6, 255};
    toggle.set_style(style);

    assert(toggle.style().off.r == 1);
    assert(toggle.style().on.g == 5);

    Container root;
    auto& controlled = root.add<Toggle>(false);
    FocusManager focus(root);
    ControllerInteraction controller(focus);

    assert(focus.focus_first());
    assert(focus.focused() == &controlled);
    assert(controller.handle(UIAction::Confirm));
    assert(controlled.checked());

    assert(controller.handle(UIAction::Confirm));
    assert(!controlled.checked());

    return 0;
}
