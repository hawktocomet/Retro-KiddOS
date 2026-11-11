#include "kiddos/ui/toggle.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

Toggle::Toggle(bool checked)
    : checked_(checked) {
    set_focusable(true);
}

void Toggle::set_checked(bool checked) {
    if (checked_ == checked) return;

    checked_ = checked;
    if (on_change_) on_change_(checked_);
}

bool Toggle::checked() const noexcept {
    return checked_;
}

bool Toggle::toggle() {
    if (!visible() || !enabled()) return false;
    set_checked(!checked_);
    return true;
}

void Toggle::set_style(ToggleStyle style) noexcept {
    style_ = style;
}

const ToggleStyle& Toggle::style() const noexcept {
    return style_;
}

void Toggle::set_on_change(std::function<void(bool)> callback) {
    on_change_ = std::move(callback);
}

void Toggle::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    const int inset = std::max(1, std::min(area.w, area.h) / 12);
    const Rect track{
        area.x + inset,
        area.y + inset,
        area.w - inset * 2,
        area.h - inset * 2
    };

    if (track.empty()) return;

    const Color track_color = checked_ ? style_.on : style_.off;
    renderer.fill_rect(track, track_color);

    if (focused()) {
        renderer.outline_rect(area, style_.focused_border);
    }

    const int knob_size = std::max(1, track.h - inset * 2);
    const int knob_y = track.y + (track.h - knob_size) / 2;
    const int knob_x = checked_
        ? track.x + track.w - knob_size - inset
        : track.x + inset;

    renderer.fill_rect(
        {knob_x, knob_y, knob_size, knob_size},
        style_.knob
    );
}

}
