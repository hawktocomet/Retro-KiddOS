// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once
#include "../models/Models.hpp"
namespace KiddosServices {
bool launchMusic(const MusicTrack& track);
void stopMusic();
bool pauseMusic();
bool resumeMusic();
bool musicPaused();
bool musicPlaying();
}
