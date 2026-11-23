// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "MusicLauncher.hpp"
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
namespace KiddosServices {
bool launchMusic(const MusicTrack& track) {
    if (track.path.empty() || access(track.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: music file missing or unreadable: %s\n", track.path.c_str());
        return false;
    }
    std::printf("KiddOS: playing music %s\n", track.title.c_str());
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/mpv", "mpv", "--no-config",
              "--no-video", "--ao=alsa", "--audio-device=alsa/hw:0,0",
              "--audio-channels=stereo", "--input-gamepad=yes",
              "--input-conf=/storage/.config/kiddos/mpv-input.conf",
              track.path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    if (pid < 0) { std::perror("KiddOS: music fork"); return false; }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        std::perror("KiddOS: music wait");
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
}
