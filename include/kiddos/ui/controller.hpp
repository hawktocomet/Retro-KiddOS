#pragma once

#include "kiddos/ui/button.hpp"
#include "kiddos/ui/focus.hpp"

namespace kiddos::ui {

enum class UIAction {
    Left,
    Right,
    Up,
    Down,
    Confirm,
    Back
};

class ControllerInteraction final {
public:
    explicit ControllerInteraction(FocusManager& focus) noexcept;

    bool handle(UIAction action);
    bool cancel() noexcept;

private:
    FocusManager& focus_;
};

} // namespace kiddos::ui
