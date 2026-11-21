// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "VolumePolicy.hpp"

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
    using namespace kiddos;

    check("increase by 5", adjustVolume(45, 1), 50);
    check("decrease by 5", adjustVolume(45, -1), 40);
    check("increase clamps at 100", adjustVolume(100, 1), 100);
    check("decrease clamps at 0", adjustVolume(0, -1), 0);
    check("zero direction unchanged", adjustVolume(45, 0), 45);
    check("clamps starting value high", adjustVolume(120, 0), 100);
    check("clamps starting value low", adjustVolume(-5, 0), 0);

    check("logical 0 maps to mixer 119", mixerVolumeFromLogical(0), 119);
    check("logical 50 maps to mixer 178", mixerVolumeFromLogical(50), 178);
    check("logical 100 maps to mixer 237", mixerVolumeFromLogical(100), 237);
    check("current mixer 172 maps to logical 45",
          logicalVolumeFromMixer(172), 45);
    check("logical 45 maps back to mixer 172",
          mixerVolumeFromLogical(45), 172);
    check("mixer below range clamps", logicalVolumeFromMixer(0), 0);
    check("mixer above range clamps", logicalVolumeFromMixer(255), 100);
    check("logical above range clamps", mixerVolumeFromLogical(120), 237);
    check("logical below range clamps", mixerVolumeFromLogical(-5), 119);

    if (failures != 0) {
        std::printf("%d test(s) failed.\n", failures);
        return 1;
    }

    std::puts("All volume policy tests passed.");
    return 0;
}
