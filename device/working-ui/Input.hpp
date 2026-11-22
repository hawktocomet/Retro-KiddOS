// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <cstdint>

namespace KiddosInput {

enum class Action {
    None,
    Back,
    Confirm,
    Up,
    Down,
    Left,
    Right,
    PreviousSection,
    NextSection,
    Start
};

// The handheld exposes its controls through SDL's joystick button events.
// Keep SDL types out of this header so the mapping is independently testable.
constexpr Action fromJoystickButton(std::uint8_t button) {
    switch (button) {
        case 0:  return Action::Back;    // B
        case 1:  return Action::Confirm; // A
        case 4:  return Action::PreviousSection; // L
        case 5:  return Action::NextSection;     // R
        case 8:  return Action::Up;
        case 9:  return Action::Down;
        case 10: return Action::Left;
        case 11: return Action::Right;
        case 13: return Action::Start;
        default: return Action::None;
    }
}

} // namespace KiddosInput
