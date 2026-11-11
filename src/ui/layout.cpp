#include "kiddos/ui/layout.hpp"

#include <algorithm>
#include <vector>

namespace kiddos::ui {

void StackLayout::apply(
    Container& container,
    Rect area,
    Axis axis,
    int gap,
    Insets padding
) noexcept {
    container.set_bounds(area);

    gap = std::max(0, gap);

    const int left = std::clamp(padding.left, 0, area.w);
    const int right = std::clamp(padding.right, 0, area.w - left);
    const int top = std::clamp(padding.top, 0, area.h);
    const int bottom = std::clamp(padding.bottom, 0, area.h - top);

    const int content_x = area.x + left;
    const int content_y = area.y + top;
    const int content_w = std::max(0, area.w - left - right);
    const int content_h = std::max(0, area.h - top - bottom);

    std::vector<Element*> visible;

    for (std::size_t i = 0; i < container.child_count(); ++i) {
        Element* item = container.child(i);
        if (item && item->visible()) {
            visible.push_back(item);
        }
    }

    if (visible.empty()) return;

    const int main_size =
        axis == Axis::Horizontal ? content_w : content_h;

    const int cross_size =
        axis == Axis::Horizontal ? content_h : content_w;

    const int total_gaps =
        gap * static_cast<int>(visible.size() - 1);

    const int available = std::max(0, main_size - total_gaps);
    const int base_size = available / static_cast<int>(visible.size());
    const int remainder = available % static_cast<int>(visible.size());

    int position = axis == Axis::Horizontal ? content_x : content_y;

    for (std::size_t i = 0; i < visible.size(); ++i) {
        const int item_size =
            base_size + (static_cast<int>(i) < remainder ? 1 : 0);

        if (axis == Axis::Horizontal) {
            visible[i]->set_bounds({
                position, content_y, item_size, cross_size
            });
        } else {
            visible[i]->set_bounds({
                content_x, position, cross_size, item_size
            });
        }

        position += item_size + gap;
    }
}

} // namespace kiddos::ui
