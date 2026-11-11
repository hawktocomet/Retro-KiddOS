#include "kiddos/ui/text_input_interaction.hpp"
#include "kiddos/ui/text_input.hpp"

namespace kiddos::ui {

TextInputInteraction::TextInputInteraction(FocusManager& focus) noexcept
    : focus_(focus) {
}

bool TextInputInteraction::insert_text(const std::string& text) {
    auto* input = dynamic_cast<TextInput*>(focus_.focused());
    if (!input || !input->enabled() || !input->visible()) return false;
    return input->insert_text(text);
}

bool TextInputInteraction::handle(TextInputKey key) {
    auto* input = dynamic_cast<TextInput*>(focus_.focused());
    if (!input || !input->enabled() || !input->visible()) return false;

    switch (key) {
        case TextInputKey::Backspace:
            input->backspace();
            return true;
        case TextInputKey::Delete:
            input->delete_forward();
            return true;
        case TextInputKey::Left:
            input->move_cursor_left();
            return true;
        case TextInputKey::Right:
            input->move_cursor_right();
            return true;
        case TextInputKey::Home:
            input->move_cursor_home();
            return true;
        case TextInputKey::End:
            input->move_cursor_end();
            return true;
    }

    return false;
}

} // namespace kiddos::ui
