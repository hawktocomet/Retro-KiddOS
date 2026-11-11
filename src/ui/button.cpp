#include "kiddos/ui/button.hpp"

#include <utility>

namespace kiddos::ui {

Button::Button(std::string label)
    : label_(std::move(label)) {
    set_focusable(true);
}

void Button::set_label(std::string label) {
    label_ = std::move(label);
}

const std::string& Button::label() const noexcept {
    return label_;
}

void Button::set_style(ButtonStyle style) noexcept {
    style_ = style;
}

const ButtonStyle& Button::style() const noexcept {
    return style_;
}

void Button::set_pressed(bool pressed) noexcept {
    pressed_ = pressed;
}

bool Button::pressed() const noexcept {
    return pressed_;
}

void Button::set_on_activate(std::function<void()> callback) {
    on_activate_ = std::move(callback);
}

bool Button::activate() {
    if (!visible() || !enabled() || !on_activate_) return false;
    on_activate_();
    return true;
}

void Button::draw(Renderer& renderer) {
    const Rect rect = bounds();
    if (rect.empty()) return;

    Color fill = style_.normal;
    Color border = style_.border;

    if (!enabled()) {
        fill = style_.disabled;
    } else if (pressed_) {
        fill = style_.pressed;
    } else if (focused()) {
        fill = style_.focused;
        border = style_.focused_border;
    }

    renderer.fill_rect(rect, fill);
    renderer.outline_rect(rect, border);
}

} // namespace kiddos::ui
