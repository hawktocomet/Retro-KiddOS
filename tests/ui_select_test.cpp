#include "kiddos/ui/select.hpp"
#include "kiddos/ui/controller.hpp"
#include "kiddos/ui/theme.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
    using namespace kiddos::ui;

    Select select({"Low", "Medium", "High"}, 1);
    assert(select.focusable());
    assert(select.selected() == 1);
    assert(select.selected_option());
    assert(*select.selected_option() == "Medium");

    int callback_count = 0;
    int callback_value = -1;
    select.set_on_change([&](int value) {
        ++callback_count;
        callback_value = value;
    });

    assert(select.select_next());
    assert(select.selected() == 2);
    assert(callback_count == 1 && callback_value == 2);

    assert(select.select_next());
    assert(select.selected() == 0);

    assert(select.select_previous());
    assert(select.selected() == 2);

    assert(select.set_selected(1));
    assert(callback_value == 1);
    assert(!select.set_selected(99));
    assert(callback_count == 4);

    select.set_enabled(false);
    assert(!select.select_next());
    select.set_enabled(true);
    select.set_visible(false);
    assert(!select.select_previous());
    select.set_visible(true);

    select.set_options({"One", "Two"});
    assert(select.selected() == 0);
    assert(*select.selected_option() == "One");
    select.set_options({});
    assert(select.selected() == -1);
    assert(select.selected_option() == nullptr);
    assert(!select.select_next());

    SelectStyle style;
    style.background = {1, 2, 3, 255};
    select.set_style(style);
    assert(select.style().background.r == 1);

    Theme theme = Theme::kiddos_default();
    const SelectStyle themed = theme.select_style();
    assert(themed.background.r == theme.colors().surface.r);
    assert(themed.focused_border.b == theme.colors().primary_bright.b);

    Container root;
    auto& controlled = root.add<Select>(
        std::vector<std::string>{"A", "B", "C"}, 0);
    FocusManager focus(root);
    ControllerInteraction controller(focus);

    assert(focus.focus_first());
    assert(controller.handle(UIAction::Right));
    assert(controlled.selected() == 1);
    assert(controller.handle(UIAction::Left));
    assert(controlled.selected() == 0);

    return 0;
}
