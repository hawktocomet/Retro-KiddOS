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

    const int radius = style_.corner_radius;
    const int thickness = std::min(
        style_.border_thickness, std::min(rect.w, rect.h) / 2
    );

    // An opaque background allows a clean, uniform border: fill the card
    // with the border color, then the inset area with the background. This
    // avoids stacking nested outlines, which look crude at small radii.
    if (thickness > 0 && style_.background.a == 255) {
        renderer.fill_rounded_rect(rect, radius, style_.border);

        const Rect inner{
            rect.x + thickness,
            rect.y + thickness,
            rect.w - thickness * 2,
            rect.h - thickness * 2
        };

        if (!inner.empty()) {
            renderer.fill_rounded_rect(
                inner, std::max(0, radius - thickness), style_.background
            );
        }
        return;
    }

    renderer.fill_rounded_rect(rect, radius, style_.background);

    // Transparent backgrounds (for example glow outlines) draw the border
    // as clean rounded outlines instead of covering the content beneath.
    for (int inset = 0; inset < thickness; ++inset) {
        const Rect outline{
            rect.x + inset,
            rect.y + inset,
            rect.w - inset * 2,
            rect.h - inset * 2
        };

        if (outline.empty()) break;
        renderer.outline_rounded_rect(
            outline,
            std::max(0, radius - inset),
            style_.border
        );
    }
}

}
