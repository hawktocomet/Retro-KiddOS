#include "kiddos/ui/controller.hpp"
#include "kiddos/ui/toggle.hpp"
#include "kiddos/ui/checkbox.hpp"
#include "kiddos/ui/radio_group.hpp"
#include "kiddos/ui/select.hpp"
#include "kiddos/ui/slider.hpp"
#include "kiddos/ui/list_view.hpp"

namespace kiddos::ui {

ControllerInteraction::ControllerInteraction(FocusManager& focus) noexcept
    : focus_(focus) {
}

bool ControllerInteraction::handle(UIAction action) {
    Element* focused = focus_.focused();

    switch (action) {
        case UIAction::Left:
            if (auto* radio = dynamic_cast<RadioGroup*>(focused))
                return radio->select_previous();
            if (auto* select = dynamic_cast<Select*>(focused))
                return select->select_previous();
            if (auto* slider = dynamic_cast<Slider*>(focused))
                return slider->adjust(-1);
            return focus_.move(Direction::Left);

        case UIAction::Right:
            if (auto* radio = dynamic_cast<RadioGroup*>(focused))
                return radio->select_next();
            if (auto* select = dynamic_cast<Select*>(focused))
                return select->select_next();
            if (auto* slider = dynamic_cast<Slider*>(focused))
                return slider->adjust(1);
            return focus_.move(Direction::Right);

        case UIAction::Up:
            if (auto* radio = dynamic_cast<RadioGroup*>(focused))
                return radio->select_previous();
            if (auto* list = dynamic_cast<ListView*>(focused))
                return list->move_selection(-1);
            return focus_.move(Direction::Up);

        case UIAction::Down:
            if (auto* radio = dynamic_cast<RadioGroup*>(focused))
                return radio->select_next();
            if (auto* list = dynamic_cast<ListView*>(focused))
                return list->move_selection(1);
            return focus_.move(Direction::Down);

        case UIAction::Confirm:
            if (auto* button = dynamic_cast<Button*>(focused))
                return button->activate();
            if (auto* toggle = dynamic_cast<Toggle*>(focused))
                return toggle->toggle();
            if (auto* checkbox = dynamic_cast<Checkbox*>(focused))
                return checkbox->toggle();
            if (auto* list = dynamic_cast<ListView*>(focused))
                return list->activate_selected();
            return false;

        case UIAction::Back:
            return cancel();
    }

    return false;
}

bool ControllerInteraction::cancel() noexcept {
    return false;
}

} // namespace kiddos::ui
