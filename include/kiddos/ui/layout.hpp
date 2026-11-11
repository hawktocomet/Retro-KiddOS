#pragma once

#include "kiddos/ui/container.hpp"

namespace kiddos::ui {

enum class Axis {
    Horizontal,
    Vertical
};

struct Insets {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

class StackLayout final {
public:
    // Distributes available space equally between visible children.
    // Remaining pixels are distributed one at a time.
    static void apply(
        Container& container,
        Rect area,
        Axis axis,
        int gap = 0,
        Insets padding = {}
    ) noexcept;
};

} // namespace kiddos::ui
