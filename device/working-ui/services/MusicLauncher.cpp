// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MusicLauncher.hpp"

#include <cerrno>
#include <cstdio>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {
namespace {
pid_t musicPid = -1;
}

bool launchMusic(const MusicTrack& track) {
    if (track.path.empty() || access(track.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: music file missing or unreadable: %s\n",
                     track.path.c_str());
        return false;
    }

    stopMusic();

    const pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/mpv", "mpv", "--no-config",
              "--no-video", "--ao=alsa", "--audio-device=alsa/hw:0,0",
              "--audio-channels=stereo", "--input-gamepad=yes",
              "--input-conf=/storage/.config/kiddos/mpv-input.conf",
              track.path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    if (pid < 0) {
        std::perror("KiddOS: music fork");
        return false;
    }

    musicPid = pid;
    std::printf("KiddOS: now playing %s\n", track.title.c_str());
    std::fflush(stdout);
    return true;
}

void stopMusic() {
    if (musicPid <= 0) return;

    kill(musicPid, SIGTERM);
    int status = 0;
    while (waitpid(musicPid, &status, 0) < 0 && errno == EINTR) {}
    musicPid = -1;
}

bool musicPlaying() {
    if (musicPid <= 0) return false;

    int status = 0;
    const pid_t result = waitpid(musicPid, &status, WNOHANG);
    if (result == 0) return true;

    if (result == musicPid || (result < 0 && errno == ECHILD)) {
        musicPid = -1;
        return false;
    }

    return true;
}
}
