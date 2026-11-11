#pragma once

namespace kiddos::ui {

struct Point {
    int x = 0;
    int y = 0;
};

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    constexpr bool empty() const noexcept {
        return w <= 0 || h <= 0;
    }

    constexpr bool contains(Point p) const noexcept {
        return !empty() &&
               p.x >= x && p.y >= y &&
               p.x < x + w && p.y < y + h;
    }
};

} // namespace kiddos::ui
