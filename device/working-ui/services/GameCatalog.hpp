// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "../models/Models.hpp"

#include <array>

namespace KiddosServices {

bool loadGameSlots();
const std::array<Game, 4>& games();

} // namespace KiddosServices
