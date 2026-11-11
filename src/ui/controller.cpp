#include "kiddos/ui/controller.hpp"
#include "kiddos/ui/toggle.hpp"
#include "kiddos/ui/checkbox.hpp"
#include "kiddos/ui/slider.hpp"
#include "kiddos/ui/list_view.hpp"

namespace kiddos::ui {

ControllerInteraction::ControllerInteraction(FocusManager& focus) noexcept
    : focus_(focus) {
}

bool ControllerInteraction::handle(UIAction action) {
    switch (action) {
        case UIAction::Left: {
            if (auto* slider = dynamic_cast<Slider*>(focus_.focused())) {
                return slider->adjust(-1);
            }
            return focus_.move(Direction::Left);
        }
        case UIAction::Right: {
            if (auto* slider = dynamic_cast<Slider*>(focus_.focused())) {
                return slider->adjust(1);
            }
            return focus_.move(Direction::Right);
        }
        case UIAction::Up: {
            if (auto* list = dynamic_cast<ListView*>(focus_.focused())) {
                return list->move_selection(-1);
            }
            return focus_.move(Direction::Up);
        }
        case UIAction::Down: {
            if (auto* list = dynamic_cast<ListView*>(focus_.focused())) {
                return list->move_selection(1);
            }
            return focus_.move(Direction::Down);
        }
        case UIAction::Confirm: {
            Element* focused = focus_.focused();
            if (auto* button = dynamic_cast<Button*>(focused)) {
                return button->activate();
            }
            if (auto* toggle = dynamic_cast<Toggle*>(focused)) {
                return toggle->toggle();
            }
            if (auto* checkbox = dynamic_cast<Checkbox*>(focused)) {
                return checkbox->toggle();
            }
            if (auto* list = dynamic_cast<ListView*>(focused)) {
                return list->activate_selected();
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
