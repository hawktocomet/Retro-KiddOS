// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <algorithm>

namespace kiddos {

constexpr int kVolumeMinimum = 0;
constexpr int kVolumeMaximum = 100;
constexpr int kVolumeStep = 5;
constexpr int kMixerMinimum = 119;
constexpr int kMixerMaximum = 237;

// Recover the logical volume represented by the current mixer value.
inline int logicalVolumeFromMixer(int mixer) noexcept {
    mixer = std::clamp(mixer, kMixerMinimum, kMixerMaximum);
    return ((mixer - kMixerMinimum) * 100 + 59) / 118;
}

// Preserve the legacy mixer conversion.
inline int mixerVolumeFromLogical(int volume) noexcept {
    volume = std::clamp(volume, kVolumeMinimum, kVolumeMaximum);
    return kMixerMinimum + (118 * volume) / 100;
}

// direction: positive = volume up, negative = volume down, zero = unchanged.
inline int adjustVolume(int current, int direction) noexcept {
    current = std::clamp(current, kVolumeMinimum, kVolumeMaximum);

    if (direction > 0)
        return std::min(current + kVolumeStep, kVolumeMaximum);
    if (direction < 0)
        return std::max(current - kVolumeStep, kVolumeMinimum);

    return current;
}

}  // namespace kiddos
