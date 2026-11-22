// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MovieLauncher.hpp"

#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {

bool launchMovie(const Movie& movie) {
    if (movie.path.empty() || access(movie.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: movie is missing or unreadable: %s\n",
                     movie.path.c_str());
        return false;
    }

    std::printf("KiddOS: playing %s\n", movie.title.c_str());
    std::fflush(stdout);

    const pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/mpv", "mpv",
              "--no-config",
              "--vo=drm",
              "--ao=alsa",
              "--audio-device=alsa/hw:0,0",
              "--audio-channels=stereo",
              "--fullscreen",
              "--input-gamepad=yes",
              "--input-conf=/storage/.config/kiddos/mpv-input.conf",
              movie.path.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    if (pid < 0) {
        std::perror("KiddOS: fork for movie");
        return false;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("KiddOS: wait for movie");
        return false;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "KiddOS: movie player exited with status %d\n",
                     status);
        return false;
    }

    std::printf("KiddOS: movie ended; returning to library\n");
    std::fflush(stdout);
    return true;
}

} // namespace KiddosServices
