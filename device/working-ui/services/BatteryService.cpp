// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "BatteryService.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace KiddosServices {

static bool readSysfs(const char* path, char* value, size_t size) {
    FILE* file = std::fopen(path, "r");
    if (!file) return false;
    bool ok = std::fgets(value, static_cast<int>(size), file) != nullptr;
    std::fclose(file);
    if (ok) value[std::strcspn(value, "\r\n")] = 0;
    return ok;
}

BatteryReading readBattery() {
    BatteryReading reading;

    const char* base =
        "/sys/devices/platform/ff180000.i2c/i2c-0/0-0020/";
    char path[256];
    char value[64];

    std::snprintf(path, sizeof(path),
        "%srk817-battery/power_supply/battery/capacity", base);
    if (readSysfs(path, value, sizeof(value))) {
        int percent = std::atoi(value);
        if (percent >= 0 && percent <= 100) {
            reading.capacityValid = true;
            reading.percent = percent;
        }
    }

    const char* sources[] = {"ac", "usb"};
    for (const char* source : sources) {
        std::snprintf(path, sizeof(path),
            "%srk817-charger/power_supply/%s/online", base, source);
        if (readSysfs(path, value, sizeof(value)) &&
            std::atoi(value) > 0)
            reading.externalPower = true;
    }

    return reading;
}

} // namespace KiddosServices
