#include "kiddos/ui/progress_bar.hpp"

#include <algorithm>

namespace kiddos::ui {

ProgressBar::ProgressBar(int value) {
    set_value(value);
}

void ProgressBar::set_value(int value) noexcept {
    value_ = std::clamp(value, 0, 100);
}

int ProgressBar::value() const noexcept {
    return value_;
}

void ProgressBar::set_orientation(ProgressOrientation orientation) noexcept {
    orientation_ = orientation;
}

ProgressOrientation ProgressBar::orientation() const noexcept {
    return orientation_;
}

void ProgressBar::set_style(ProgressBarStyle style) noexcept {
    style.border_thickness = std::max(0, style.border_thickness);
    style_ = style;
}

const ProgressBarStyle& ProgressBar::style() const noexcept {
    return style_;
}

void ProgressBar::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    renderer.fill_rect(area, style_.background);

    const int border = std::min(
        style_.border_thickness,
        std::min(area.w, area.h) / 2
    );

    if (border > 0) {
        renderer.outline_rect(area, style_.border);
    }

    const Rect inner{
        area.x + border,
        area.y + border,
        area.w - border * 2,
        area.h - border * 2
    };

    if (inner.empty() || value_ == 0) return;

    Rect fill_area = inner;

    if (orientation_ == ProgressOrientation::horizontal) {
        fill_area.w = inner.w * value_ / 100;
    } else {
        fill_area.h = inner.h * value_ / 100;
        fill_area.y = inner.y + inner.h - fill_area.h;
    }

    if (!fill_area.empty()) {
        renderer.fill_rect(fill_area, style_.fill);
    }
}

}
