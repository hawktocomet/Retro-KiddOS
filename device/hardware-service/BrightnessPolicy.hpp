// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <algorithm>

namespace kiddos {

inline int adjustBrightness(int current, int maximum,
                            unsigned int button) noexcept {
    const int upper = std::max(1, std::min(maximum, 99));
    current = std::clamp(current, 1, upper);

    if (button == 16)
        return std::min(current + 10, upper);

    if (button == 17)
        return std::max(current - 10, 1);

    return current;
}

}  // namespace kiddos
