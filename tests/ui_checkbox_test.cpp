#include "kiddos/ui/checkbox.hpp"
#include "kiddos/ui/controller.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Checkbox checkbox("Enable sound");
    assert(!checkbox.checked());
    assert(checkbox.focusable());
    assert(checkbox.label() == "Enable sound");

    int callback_count = 0;
    bool callback_value = false;
    checkbox.set_on_change([&](bool value) {
        ++callback_count;
        callback_value = value;
    });

    assert(checkbox.toggle());
    assert(checkbox.checked());
    assert(callback_count == 1 && callback_value);

    checkbox.set_checked(true);
    assert(callback_count == 1);

    checkbox.set_checked(false);
    assert(!checkbox.checked());
    assert(callback_count == 2 && !callback_value);

    checkbox.set_enabled(false);
    assert(!checkbox.toggle());
    checkbox.set_enabled(true);
    checkbox.set_visible(false);
    assert(!checkbox.toggle());
    checkbox.set_visible(true);

    Container root;
    auto& controlled = root.add<Checkbox>("Wi-Fi", false);
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
