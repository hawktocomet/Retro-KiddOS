// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../BrightnessPolicy.hpp"

#include <cstdio>

namespace {
int failures = 0;

void check(const char* name, int actual, int expected) {
    if (actual == expected) {
        std::printf("PASS: %s\n", name);
    } else {
        std::printf("FAIL: %s (expected %d, got %d)\n",
                    name, expected, actual);
        ++failures;
    }
}
}

int main() {
    using kiddos::adjustBrightness;

    check("increase by 10", adjustBrightness(50, 99, 16), 60);
    check("decrease by 10", adjustBrightness(50, 99, 17), 40);
    check("increase clamps at 99", adjustBrightness(95, 99, 16), 99);
    check("decrease clamps at 1", adjustBrightness(5, 99, 17), 1);
    check("respects device maximum", adjustBrightness(90, 95, 16), 95);
    check("caps maximum at 99", adjustBrightness(95, 120, 16), 99);
    check("handles invalid maximum", adjustBrightness(50, 0, 16), 1);
    check("ignores unrelated button", adjustBrightness(50, 99, 3), 50);
    check("clamps starting value", adjustBrightness(120, 99, 3), 99);

    if (failures != 0) {
        std::printf("%d test(s) failed.\n", failures);
        return 1;
    }

    std::puts("All brightness policy tests passed.");
    return 0;
}
