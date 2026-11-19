// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "../models/Models.hpp"

#include <sys/types.h>
#include <string>
#include <vector>

namespace KiddosServices {

struct WifiReading {
    WifiStatus status = WifiStatus::Unknown;
    bool powerKnown = false;
    bool powered = false;
};

enum class WifiScanError {
    None,
    StartFailed,
    HelperFailed,
    ScanFailed
};

struct WifiScanResult {
    WifiScanError error = WifiScanError::None;
    std::vector<WifiNetwork> networks;

    bool success() const {
        return error == WifiScanError::None;
    }
};

struct WifiConnection {
    pid_t pid = -1;
    int outputFd = -1;
    std::string outputText;
};

enum class WifiConnectionResult {
    Idle,
    Running,
    Connected,
    NeedsPassword,
    Failed
};

WifiReading readWifiStatus();
bool setWifiPower(bool enabled);

WifiScanResult loadWifiNetworks(const std::string& helperPath, bool scan);

bool startWifiConnection(WifiConnection& connection,
                         const std::string& helperPath,
                         const WifiNetwork& network,
                         const std::string& password);
WifiConnectionResult pollWifiConnection(WifiConnection& connection);
void cancelWifiConnection(WifiConnection& connection);

} // namespace KiddosServices
