// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

namespace KiddosServices {

struct BatteryReading {
    bool capacityValid = false;
    int percent = -1;
    bool externalPower = false;
};

BatteryReading readBattery();

} // namespace KiddosServices
