// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MusicWorkflow.hpp"

#include "MusicCatalog.hpp"
#include "MusicLauncher.hpp"

namespace KiddosServices {

bool MusicWorkflow::restoreCurrentTrack() {
    if (currentPath_.empty() || !musicPlaying())
        return false;

    const auto& playlists = musicPlaylists();
    for (std::size_t playlistIndex = 0;
         playlistIndex < playlists.size(); ++playlistIndex) {
        const auto& playlist = playlists[playlistIndex];
        for (std::size_t trackIndex = 0;
             trackIndex < playlist.tracks.size(); ++trackIndex) {
            if (playlist.tracks[trackIndex].path != currentPath_)
                continue;

            currentPlaylist_ = static_cast<int>(playlistIndex);
            currentTrack_ = static_cast<int>(trackIndex);
            return true;
        }
    }

    // Preserve the path if the playing file disappeared from the reloaded
    // catalog. This matches the previous UI-recreation behavior.
    currentPlaylist_ = -1;
    currentTrack_ = -1;
    return false;
}

bool MusicWorkflow::playTrack(int playlistIndex, int trackIndex) {
    const auto& playlists = musicPlaylists();
    if (playlistIndex < 0 ||
        playlistIndex >= static_cast<int>(playlists.size()))
        return false;

    const auto& playlist = playlists[static_cast<std::size_t>(playlistIndex)];
    if (trackIndex < 0 ||
        trackIndex >= static_cast<int>(playlist.tracks.size()))
        return false;

    const auto& track = playlist.tracks[static_cast<std::size_t>(trackIndex)];
    const bool resuming =
        playlistIndex == currentPlaylist_ &&
        trackIndex == currentTrack_ &&
        currentPath_ == track.path &&
        musicPaused();

    const bool started = resuming ? resumeMusic() : launchMusic(track);
    if (!started)
        return false;

    currentPlaylist_ = playlistIndex;
    currentTrack_ = trackIndex;
    currentPath_ = track.path;
    return true;
}

bool MusicWorkflow::moveTrack(int direction) {
    const auto& playlists = musicPlaylists();
    if (currentPlaylist_ < 0 ||
        currentPlaylist_ >= static_cast<int>(playlists.size()))
        return false;

    const auto& tracks =
        playlists[static_cast<std::size_t>(currentPlaylist_)].tracks;
    if (tracks.empty() || currentTrack_ < 0 ||
        currentTrack_ >= static_cast<int>(tracks.size()))
        return false;

    const int count = static_cast<int>(tracks.size());
    const int next = (currentTrack_ + direction + count) % count;
    return playTrack(currentPlaylist_, next);
}

bool MusicWorkflow::previousTrack() {
    return moveTrack(-1);
}

bool MusicWorkflow::nextTrack() {
    return moveTrack(1);
}

bool MusicWorkflow::advanceIfFinished() {
    if (currentTrack_ < 0 || musicPaused() || musicPlaying())
        return false;

    const auto& playlists = musicPlaylists();
    if (currentPlaylist_ < 0 ||
        currentPlaylist_ >= static_cast<int>(playlists.size())) {
        clearPlayback();
        return false;
    }

    const auto& tracks =
        playlists[static_cast<std::size_t>(currentPlaylist_)].tracks;
    if (tracks.empty() || currentTrack_ < 0 ||
        currentTrack_ >= static_cast<int>(tracks.size())) {
        clearPlayback();
        return false;
    }

    const int next =
        (currentTrack_ + 1) % static_cast<int>(tracks.size());
    if (!playTrack(currentPlaylist_, next)) {
        clearPlayback();
        return false;
    }
    return true;
}

bool MusicWorkflow::togglePause() {
    if (musicPaused())
        return resumeMusic();
    return pauseMusic();
}

bool MusicWorkflow::pauseForHandoff() {
    resumeAfterHandoff_ = false;
    if (!musicPlaying() || musicPaused())
        return true;

    if (!pauseMusic())
        return false;

    resumeAfterHandoff_ = true;
    return true;
}

void MusicWorkflow::resumeAfterHandoff() {
    if (!resumeAfterHandoff_)
        return;

    resumeAfterHandoff_ = false;
    resumeMusic();
}

void MusicWorkflow::stop() {
    stopMusic();
    clearPlayback();
}

void MusicWorkflow::clearPlayback() {
    currentPlaylist_ = -1;
    currentTrack_ = -1;
    currentPath_.clear();
}

const KiddosMusicPlaylist* MusicWorkflow::currentPlaylist() const {
    const auto& playlists = musicPlaylists();
    if (currentPlaylist_ < 0 ||
        currentPlaylist_ >= static_cast<int>(playlists.size()))
        return nullptr;
    return &playlists[static_cast<std::size_t>(currentPlaylist_)];
}

const MusicTrack* MusicWorkflow::currentTrack() const {
    const auto* playlist = currentPlaylist();
    if (!playlist || currentTrack_ < 0 ||
        currentTrack_ >= static_cast<int>(playlist->tracks.size()))
        return nullptr;
    return &playlist->tracks[static_cast<std::size_t>(currentTrack_)];
}

bool MusicWorkflow::playing() const {
    return musicPlaying();
}

bool MusicWorkflow::paused() const {
    return musicPaused();
}

} // namespace KiddosServices
