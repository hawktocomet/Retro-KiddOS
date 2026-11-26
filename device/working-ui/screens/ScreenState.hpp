// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "../models/Models.hpp"
#include "../services/WifiService.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace KiddosScreens {

// Wi-Fi screen interaction state. Device operations remain in WifiService.
struct WifiState {
    std::vector<WifiNetwork> networks;
    int selectedRow = 0;
    int scrollOffset = 0;
    std::string listMessage = "SCAN FOR NETWORKS";
    bool scanAttempted = false;

    std::string password;
    int passwordCursor = 0;
    int passwordChar = 0;
    int keyboardRow = 0;
    int keyboardCol = 0;
    bool keyboardShift = false;
    bool keyboardSymbols = false;
    bool needsPassword = false;

    WifiStatus status = WifiStatus::Unknown;
    bool powered = false;
    bool powerKnown = false;
    bool commandError = false;
    KiddosServices::WifiConnection connection;
};

// Shared selection state; each media workflow keeps an independent instance.
struct BrowserSelectionState {
    int selectedRow = 0;
    int scrollOffset = 0;
};

// Persistent browsing state survives SDL UI teardown during external playback.
struct MediaBrowserState {
    BrowserSelectionState movies;
    BrowserSelectionState tvShows;
    BrowserSelectionState music;
    int musicPlaylistIndex = -1;
};

// Battery and connection indicators are shared chrome, not screen workflow state.
struct StatusBarState {
    int batteryPercent = -1;
    bool externalPower = false;
    std::uint32_t lastPoll = 0;
};

} // namespace KiddosScreens
