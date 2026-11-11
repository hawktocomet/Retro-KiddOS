#include "kiddos/ui/screen.hpp"
namespace kiddos::ui {
Screen::Screen() noexcept : focus_(root_), controller_(focus_), text_input_(focus_) {}
Container& Screen::root() noexcept { return root_; }
FocusManager& Screen::focus() noexcept { return focus_; }
void Screen::render(Renderer& renderer) { root_.render(renderer); }
bool Screen::handle_action(UIAction action) {
    if (!focus_.focused()) focus_.focus_first();
    return controller_.handle(action);
}
bool Screen::handle_text_input(const ::kiddos::TextInputEvent& event) {
    switch (event.type) {
    case ::kiddos::TextInputEventType::Text: return text_input_.insert_text(event.text);
    case ::kiddos::TextInputEventType::Backspace: return text_input_.handle(TextInputKey::Backspace);
    case ::kiddos::TextInputEventType::Delete: return text_input_.handle(TextInputKey::Delete);
    case ::kiddos::TextInputEventType::Left: return text_input_.handle(TextInputKey::Left);
    case ::kiddos::TextInputEventType::Right: return text_input_.handle(TextInputKey::Right);
    case ::kiddos::TextInputEventType::Home: return text_input_.handle(TextInputKey::Home);
    case ::kiddos::TextInputEventType::End: return text_input_.handle(TextInputKey::End);
    }
    return false;
}
}
