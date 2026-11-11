#include "kiddos/ui/panel.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

Panel::Panel(PanelStyle style)
    : style_(std::move(style)) {
    style_.border_thickness = std::max(0, style_.border_thickness);
    style_.corner_radius = std::max(0, style_.corner_radius);
}

void Panel::set_style(PanelStyle style) noexcept {
    style.border_thickness = std::max(0, style.border_thickness);
    style.corner_radius = std::max(0, style.corner_radius);
    style_ = style;
}

const PanelStyle& Panel::style() const noexcept {
    return style_;
}

void Panel::set_background(Color color) noexcept {
    style_.background = color;
}

void Panel::set_border(Color color) noexcept {
    style_.border = color;
}

void Panel::set_border_thickness(int thickness) noexcept {
    style_.border_thickness = std::max(0, thickness);
}

void Panel::set_corner_radius(int radius) noexcept {
    style_.corner_radius = std::max(0, radius);
}

void Panel::draw(Renderer& renderer) {
    const Rect rect = bounds();
    if (rect.empty()) return;

    renderer.fill_rounded_rect(rect, style_.corner_radius, style_.background);

    if (style_.border_thickness <= 0) return;

    for (int inset = 0; inset < style_.border_thickness; ++inset) {
        const Rect outline{
            rect.x + inset,
            rect.y + inset,
            rect.w - inset * 2,
            rect.h - inset * 2
        };

        if (outline.empty()) break;
        renderer.outline_rounded_rect(
            outline,
            std::max(0, style_.corner_radius - inset),
            style_.border
        );
    }
}

}
