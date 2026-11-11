#include "kiddos/ui/checkbox.hpp"
#include "kiddos/ui/text.hpp"
#include "kiddos/ui/geometry.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

Checkbox::Checkbox(std::string label, bool checked)
    : label_(std::move(label)), checked_(checked) {
    set_focusable(true);
}

void Checkbox::set_label(std::string label) {
    label_ = std::move(label);
}

const std::string& Checkbox::label() const noexcept {
    return label_;
}

void Checkbox::set_checked(bool checked) {
    if (checked_ == checked) return;
    checked_ = checked;
    if (on_change_) on_change_(checked_);
}

bool Checkbox::checked() const noexcept {
    return checked_;
}

bool Checkbox::toggle() {
    if (!visible() || !enabled()) return false;
    set_checked(!checked_);
    return true;
}

void Checkbox::set_font(const Font* font) noexcept {
    font_ = font;
}

const Font* Checkbox::font() const noexcept {
    return font_;
}

void Checkbox::set_style(CheckboxStyle style) noexcept {
    style_ = style;
}

const CheckboxStyle& Checkbox::style() const noexcept {
    return style_;
}

void Checkbox::set_on_change(std::function<void(bool)> callback) {
    on_change_ = std::move(callback);
}

void Checkbox::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    const int box_size = std::max(1, std::min(area.h - 8, 36));
    const int box_y = area.y + (area.h - box_size) / 2;
    const Rect box{area.x, box_y, box_size, box_size};

    renderer.fill_rect(box, enabled() && checked_ ? style_.checked : style_.background);
    renderer.outline_rect(box, focused() ? style_.focused_border : style_.border);

    if (checked_) {
        const int inset = std::max(2, box_size / 4);
        renderer.draw_line(
            {box.x + inset, box.y + box_size / 2},
            {box.x + box_size / 2 - 1, box.y + box_size - inset},
            style_.checkmark
        );
        renderer.draw_line(
            {box.x + box_size / 2 - 1, box.y + box_size - inset},
            {box.x + box_size - inset, box.y + inset},
            style_.checkmark
        );
    }

    if (font_ && font_->valid() && !label_.empty()) {
        const int gap = 12;
        Rect text_area{
            box.x + box.w + gap,
            area.y,
            area.w - box.w - gap,
            area.h
        };
        if (!text_area.empty()) {
            TextRenderer text(renderer);
            text.draw_centered(
                label_, *font_, text_area,
                enabled() ? style_.text : style_.disabled_text
            );
        }
    }
}

}
