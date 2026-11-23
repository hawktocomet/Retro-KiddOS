// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MovieLauncher.hpp"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {
namespace {

int openGamepadEventDevice() {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        std::perror("KiddOS: cannot open /dev/input");
        return -1;
    }

    int result = -1;
    while (dirent* entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "event", 5) != 0)
            continue;

        const std::string path = std::string("/dev/input/") + entry->d_name;
        const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
            continue;

        char name[256] = {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
            std::strcmp(name, "GO-Super Gamepad") == 0) {
            result = fd;
            std::printf("KiddOS: monitoring movie exit buttons on %s\n",
                        path.c_str());
            break;
        }
        close(fd);
    }

    closedir(dir);
    if (result < 0)
        std::fprintf(stderr,
                     "KiddOS: gamepad event device unavailable; "
                     "movie will use normal mpv controls\n");
    return result;
}

} // namespace

bool launchMovie(const Movie& movie) {
    if (movie.path.empty() || access(movie.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: movie is missing or unreadable: %s\n",
                     movie.path.c_str());
        return false;
    }

    // A separate evdev reader observes button events without grabbing them
    // from mpv or the rest of KiddOS.
    const int gamepadFd = openGamepadEventDevice();

    std::printf("KiddOS: playing %s\n", movie.title.c_str());
    std::fflush(stdout);

    const pid_t pid = fork();
    if (pid == 0) {
        if (gamepadFd >= 0)
            close(gamepadFd);
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
        if (gamepadFd >= 0)
            close(gamepadFd);
        std::perror("Kiddos: fork for movie");
        return false;
    }

    bool selectDown = false;
    bool startDown = false;
    bool chordLatched = false;
    bool requestedExit = false;
    int chordCount = 0;
    auto firstChord = std::chrono::steady_clock::time_point{};

    int status = 0;
    for (;;) {
        const pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid)
            break;
        if (waited < 0) {
            if (errno == EINTR)
                continue;
            if (gamepadFd >= 0)
                close(gamepadFd);
            std::perror("Kiddos: wait for movie");
            return false;
        }

        pollfd pfd{};
        pfd.fd = gamepadFd;
        pfd.events = POLLIN;
        const int pollResult = poll(gamepadFd >= 0 ? &pfd : nullptr,
                                    gamepadFd >= 0 ? 1 : 0, 50);
        if (pollResult < 0 && errno != EINTR)
            continue;
        if (gamepadFd < 0 || !(pfd.revents & POLLIN))
            continue;

        input_event event{};
        for (;;) {
            const ssize_t bytes = read(gamepadFd, &event, sizeof(event));
            if (bytes != static_cast<ssize_t>(sizeof(event)))
                break;
            if (event.type != EV_KEY)
                continue;

            if (event.code == BTN_TRIGGER_HAPPY1)
                selectDown = event.value != 0;
            else if (event.code == BTN_TRIGGER_HAPPY2)
                startDown = event.value != 0;
            else
                continue;

            if (!selectDown && !startDown)
                chordLatched = false;

            if (selectDown && startDown && !chordLatched) {
                chordLatched = true;
                const auto now = std::chrono::steady_clock::now();
                if (chordCount == 0 ||
                    now - firstChord > std::chrono::milliseconds(1500)) {
                    chordCount = 1;
                    firstChord = now;
                    std::printf("KiddOS: Start+Select detected; press again to exit movie\n");
                    std::fflush(stdout);
                } else {
                    requestedExit = true;
                    std::printf("KiddOS: confirmed double Start+Select; stopping movie\n");
                    std::fflush(stdout);
                    kill(pid, SIGTERM);
                    break;
                }
            }
        }
        if (requestedExit)
            continue;
    }

    if (gamepadFd >= 0)
        close(gamepadFd);

    if (requestedExit) {
        std::printf("KiddOS: movie closed by Start+Select; returning to library\n");
        std::fflush(stdout);
        return true;
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
