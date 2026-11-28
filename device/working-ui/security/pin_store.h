// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

namespace KiddosPin {

bool initializePinStore(const char* directory);
bool verifyPin(const char* directory, const char* pin);
bool changePin(const char* directory,
               const char* currentPin,
               const char* newPin);

} // namespace KiddosPin
