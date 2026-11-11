#include "kiddos/ui/controller.hpp"
#include "kiddos/ui/toggle.hpp"

namespace kiddos::ui {

ControllerInteraction::ControllerInteraction(FocusManager& focus) noexcept
    : focus_(focus) {
}

bool ControllerInteraction::handle(UIAction action) {
    switch (action) {
        case UIAction::Left:
            return focus_.move(Direction::Left);
        case UIAction::Right:
            return focus_.move(Direction::Right);
        case UIAction::Up:
            return focus_.move(Direction::Up);
        case UIAction::Down:
            return focus_.move(Direction::Down);
        case UIAction::Confirm: {
            Element* focused = focus_.focused();
            if (auto* button = dynamic_cast<Button*>(focused)) {
                return button->activate();
            }
            if (auto* toggle = dynamic_cast<Toggle*>(focused)) {
                return toggle->toggle();
            }
            return false;
        }
        case UIAction::Back:
            return cancel();
    }

    return false;
}

bool ControllerInteraction::cancel() noexcept {
    return false;
}

} // namespace kiddos::ui
