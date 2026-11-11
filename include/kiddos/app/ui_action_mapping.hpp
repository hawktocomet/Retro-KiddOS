#pragma once

#include "kiddos/core/action.hpp"
#include "kiddos/ui/controller.hpp"

#include <optional>

namespace kiddos::app {

// Translates a platform action into the matching abstract UI action.
// Returns std::nullopt for actions without a UI counterpart (None, Quit).
inline std::optional<ui::UIAction> to_ui_action(Action action) noexcept {
    switch (action) {
        case Action::NavigateLeft:
            return ui::UIAction::Left;
        case Action::NavigateRight:
            return ui::UIAction::Right;
        case Action::NavigateUp:
            return ui::UIAction::Up;
        case Action::NavigateDown:
            return ui::UIAction::Down;
        case Action::Confirm:
            return ui::UIAction::Confirm;
        case Action::Back:
            return ui::UIAction::Back;
        case Action::None:
        case Action::Quit:
            return std::nullopt;
    }
    return std::nullopt;
}

} // namespace kiddos::app
