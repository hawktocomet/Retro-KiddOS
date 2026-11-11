#include "kiddos/ui/list_view.hpp"
#include "kiddos/ui/controller.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
    using namespace kiddos::ui;

    ListView list;
    assert(list.focusable());
    assert(list.selected_index() == -1);

    list.set_items({"Play", "Settings", "Parent Center", "About"});
    assert(list.items().size() == 4);
    assert(list.selected_index() == 0);

    assert(list.move_selection(1));
    assert(list.selected_index() == 1);
    assert(list.set_selected_index(3));
    assert(list.selected_index() == 3);
    assert(!list.move_selection(1));
    assert(list.move_selection(-1));
    assert(list.selected_index() == 2);

    int activated = -1;
    list.set_on_activate([&](int index) { activated = index; });
    assert(list.activate_selected());
    assert(activated == 2);

    assert(list.set_selected_index(0));
    assert(!list.move_selection(-1));
    list.set_row_height(36);
    assert(list.row_height() == 36);

    ListViewStyle style;
    style.selected = {1, 2, 3, 255};
    list.set_style(style);
    assert(list.style().selected.r == 1);

    Container root;
    auto& controlled = root.add<ListView>();
    controlled.set_items({"One", "Two", "Three"});
    int chosen = -1;
    controlled.set_on_activate([&](int index) { chosen = index; });

    FocusManager focus(root);
    ControllerInteraction controller(focus);
    assert(focus.focus_first());
    assert(controller.handle(UIAction::Down));
    assert(controlled.selected_index() == 1);
    assert(controller.handle(UIAction::Confirm));
    assert(chosen == 1);

    controlled.set_enabled(false);
    assert(!controller.handle(UIAction::Down));
    controlled.set_enabled(true);

    controlled.set_items({});
    assert(controlled.selected_index() == -1);
    assert(!controlled.activate_selected());

    return 0;
}
