// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../services/MusicWorkflow.hpp"
#include "../services/MusicCatalog.hpp"
#include "../services/MusicLauncher.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace {
std::vector<KiddosMusicPlaylist> playlists;
bool isPlaying = false;
bool isPaused = false;
bool launchSucceeds = true;
int launches = 0;
int pauses = 0;
int resumes = 0;
int stops = 0;
int failures = 0;

void check(bool condition, const char* description) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++failures;
}

MusicTrack track(const char* title, const char* path) {
    return MusicTrack{title, path};
}

void resetPlayback() {
    isPlaying = false;
    isPaused = false;
    launchSucceeds = true;
    launches = pauses = resumes = stops = 0;
}

void setupCatalog() {
    playlists.clear();
    KiddosMusicPlaylist first;
    first.title = "Playlist One";
    first.path = "/music/one";
    first.artwork = "/music/one.png";
    first.tracks = {
        track("Track A", "/music/a.mp3"),
        track("Track B", "/music/b.mp3")
    };
    KiddosMusicPlaylist second;
    second.title = "Playlist Two";
    second.path = "/music/two";
    second.tracks = {track("Track C", "/music/c.mp3")};
    playlists = {first, second};
}
}

namespace KiddosServices {
bool loadMusic() { return true; }
const std::vector<KiddosMusicPlaylist>& musicPlaylists() { return playlists; }

bool launchMusic(const MusicTrack&) {
    ++launches;
    if (!launchSucceeds) return false;
    isPlaying = true;
    isPaused = false;
    return true;
}
void stopMusic() {
    ++stops;
    isPlaying = false;
    isPaused = false;
}
bool pauseMusic() {
    ++pauses;
    if (!isPlaying || isPaused) return false;
    isPaused = true;
    return true;
}
bool resumeMusic() {
    ++resumes;
    if (!isPaused) return false;
    isPaused = false;
    isPlaying = true;
    return true;
}
bool musicPaused() { return isPaused; }
bool musicPlaying() { return isPlaying; }
}

int main() {
    using KiddosServices::MusicWorkflow;

    setupCatalog();
    resetPlayback();
    MusicWorkflow workflow;

    check(workflow.playTrack(0, 0), "starts selected track");
    check(workflow.currentPlaylistIndex() == 0 &&
          workflow.currentTrackIndex() == 0 &&
          workflow.currentPath() == "/music/a.mp3",
          "records current playlist, track, and path");
    check(launches == 1, "launches the track once");

    check(workflow.nextTrack() && workflow.currentTrackIndex() == 1,
          "next track advances");
    check(workflow.nextTrack() && workflow.currentTrackIndex() == 0,
          "next track wraps at playlist end");
    check(workflow.previousTrack() && workflow.currentTrackIndex() == 1,
          "previous track wraps at playlist beginning");

    check(workflow.togglePause() && workflow.paused(),
          "pause toggle pauses playback");
    const int launchesBeforeResume = launches;
    check(workflow.playTrack(0, 1) && workflow.playing() &&
          launches == launchesBeforeResume,
          "selecting the same paused track resumes instead of relaunching");

    check(workflow.pauseForHandoff() && workflow.paused(),
          "handoff pauses active music");
    workflow.resumeAfterHandoff();
    check(workflow.playing() && !workflow.paused(),
          "handoff completion resumes music");
    check(pauses == 2 && resumes == 2,
          "pause and resume calls are balanced");

    isPaused = true;
    check(workflow.pauseForHandoff(), "already-paused music permits handoff");
    workflow.resumeAfterHandoff();
    check(workflow.paused(),
          "handoff does not resume music that was already paused");

    check(workflow.togglePause(), "music can be resumed after paused handoff");
    isPlaying = false;
    check(workflow.advanceIfFinished() && workflow.currentTrackIndex() == 0,
          "natural completion advances to next track");
    isPlaying = false;
    check(workflow.advanceIfFinished() && workflow.currentTrackIndex() == 1,
          "natural completion loops within playlist");

    isPaused = true;
    check(!workflow.advanceIfFinished() && workflow.currentTrackIndex() == 1,
          "paused playback does not auto-advance");
    isPaused = false;
    isPlaying = true;

    playlists = {playlists[1], playlists[0]};
    check(workflow.restoreCurrentTrack() &&
          workflow.currentPlaylistIndex() == 1 &&
          workflow.currentTrackIndex() == 1,
          "restores track indices by file path after catalog reorder");

    playlists = {playlists[0]};
    check(!workflow.restoreCurrentTrack() &&
          workflow.currentPlaylistIndex() == -1 &&
          workflow.currentTrackIndex() == -1 &&
          workflow.currentPath() == "/music/b.mp3",
          "missing catalog track clears indices but preserves playing path");

    setupCatalog();
    resetPlayback();
    check(!workflow.playTrack(9, 0), "rejects invalid playlist index");
    check(!workflow.playTrack(0, 9), "rejects invalid track index");

    check(workflow.playTrack(0, 0), "starts track before failed-launch test");
    launchSucceeds = false;
    check(!workflow.nextTrack(), "reports a failed next-track launch");
    check(workflow.currentTrackIndex() == 0,
          "failed manual track change preserves prior playback state");

    launchSucceeds = true;
    check(workflow.playTrack(0, 1), "starts track before completion test");
    isPlaying = false;
    launchSucceeds = false;
    check(!workflow.advanceIfFinished() &&
          workflow.currentPlaylistIndex() == -1 &&
          workflow.currentTrackIndex() == -1 &&
          workflow.currentPath().empty(),
          "failed automatic advance clears completed playback state");

    workflow.stop();
    check(stops == 1, "stop delegates to music launcher");

    return failures == 0 ? 0 : 1;
}
