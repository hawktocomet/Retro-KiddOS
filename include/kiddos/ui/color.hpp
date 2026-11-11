#pragma once

#include <cstdint>

namespace kiddos::ui {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

namespace colors {
inline constexpr Color transparent{0, 0, 0, 0};
inline constexpr Color navy{8, 15, 35, 255};
inline constexpr Color blue{69, 151, 255, 255};
inline constexpr Color white{255, 255, 255, 255};
}

} // namespace kiddos::ui
