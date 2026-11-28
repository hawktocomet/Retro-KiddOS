// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once

#include "../models/Models.hpp"

#include <string>

namespace KiddosServices {

class MusicWorkflow {
public:
    bool restoreCurrentTrack();
    bool playTrack(int playlistIndex, int trackIndex);
    bool previousTrack();
    bool nextTrack();
    bool advanceIfFinished();

    bool togglePause();
    bool pauseForHandoff();
    void resumeAfterHandoff();
    void stop();

    int currentPlaylistIndex() const { return currentPlaylist_; }
    int currentTrackIndex() const { return currentTrack_; }
    const std::string& currentPath() const { return currentPath_; }
    const KiddosMusicPlaylist* currentPlaylist() const;
    const MusicTrack* currentTrack() const;

    bool playing() const;
    bool paused() const;

private:
    bool moveTrack(int direction);
    void clearPlayback();

    int currentPlaylist_ = -1;
    int currentTrack_ = -1;
    std::string currentPath_;
    bool resumeAfterHandoff_ = false;
};

} // namespace KiddosServices
