// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <string>

struct Game {
    std::string name;
    std::string rom;
    std::string platform;
    std::string image;
    bool available = false;
};

struct Movie {
    std::string title;
    std::string path;
};

struct MusicTrack {
    std::string title;
    std::string path;
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
