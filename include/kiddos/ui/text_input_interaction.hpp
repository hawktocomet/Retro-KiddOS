#pragma once

#include "kiddos/ui/focus.hpp"

#include <string>

namespace kiddos::ui {

enum class TextInputKey {
    Backspace,
    Delete,
    Left,
    Right,
    Home,
    End
};

class TextInputInteraction final {
public:
    explicit TextInputInteraction(FocusManager& focus) noexcept;

    bool insert_text(const std::string& text);
    bool handle(TextInputKey key);

private:
    FocusManager& focus_;
};

} // namespace kiddos::ui
