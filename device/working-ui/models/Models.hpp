// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <string>

struct Game {
    const char* name;
    const char* rom;
    const char* platform;
    const char* image;
};

enum class WifiStatus {
    Unknown,
    Disconnected,
    Connected
};

struct WifiNetwork {
    std::string path;
    std::string name;
    int strength = 0;
    std::string security;
    std::string state;
};
