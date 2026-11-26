// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once
#include "../models/Models.hpp"
#include <vector>
namespace KiddosServices {
bool loadMusic();
const std::vector<MusicTrack>& music();
const std::vector<KiddosMusicPlaylist>& musicPlaylists();
}
