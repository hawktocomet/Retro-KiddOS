#include "kiddos/app/ui_action_mapping.hpp"

#include <cassert>
#include <optional>

int main() {
    using kiddos::Action;
    using kiddos::app::to_ui_action;
    using kiddos::ui::UIAction;

    assert(to_ui_action(Action::NavigateLeft) == UIAction::Left);
    assert(to_ui_action(Action::NavigateRight) == UIAction::Right);
    assert(to_ui_action(Action::NavigateUp) == UIAction::Up);
    assert(to_ui_action(Action::NavigateDown) == UIAction::Down);
    assert(to_ui_action(Action::Confirm) == UIAction::Confirm);
    assert(to_ui_action(Action::Back) == UIAction::Back);

    assert(to_ui_action(Action::None) == std::nullopt);
    assert(to_ui_action(Action::Quit) == std::nullopt);

    return 0;
}
