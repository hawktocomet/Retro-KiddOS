// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "GameLauncher.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
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

constexpr const char* RETROARCH = "/usr/bin/retroarch";
constexpr const char* CORE = "/storage/cores/gambatte_libretro.so";
constexpr const char* CONFIG =
    "/storage/.config/retroarch/retroarch.cfg";
constexpr const char* PROFILE_DIR =
    "/storage/.config/kiddos/retroarch";
constexpr const char* HANDHELD_SHADER =
    "/storage/.config/kiddos/retroarch/handheld-lcd.glslp";

constexpr auto CHORD_WINDOW = std::chrono::milliseconds(1500);

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

        const std::string path =
            std::string("/dev/input/") + entry->d_name;

        const int fd =
            open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);

        if (fd < 0)
            continue;

        char name[256] = {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
            std::strcmp(name, "GO-Super Gamepad") == 0) {
            result = fd;
            std::printf("KiddOS: monitoring game exit buttons on %s\n",
                        path.c_str());
            break;
        }

        close(fd);
    }

    closedir(dir);

    if (result < 0) {
        std::fprintf(stderr,
            "KiddOS: gamepad event device unavailable; "
            "use normal RetroArch controls to exit\n");
    }

    return result;
}

bool reapChild(pid_t child, int& status, bool& reaped) {
    if (reaped)
        return true;

    pid_t result;
    do {
        result = waitpid(child, &status, WNOHANG);
    } while (result < 0 && errno == EINTR);

    if (result == child) {
        reaped = true;
        return true;
    }

    if (result < 0 && errno == ECHILD) {
        status = -1;
        reaped = true;
        return true;
    }

    if (result < 0) {
        std::perror("KiddOS: waitpid for RetroArch");
        // An unexpected wait error is not a successful child exit.
        status = -1;
        reaped = true;
        return true;
    }

    return false;
}

void waitForChild(pid_t child, int& status, bool& reaped,
                  std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!reaped &&
           std::chrono::steady_clock::now() < deadline) {
        if (reapChild(child, status, reaped))
            break;

        usleep(50000);
    }
}

void stopRetroArch(pid_t child, int& status, bool& reaped) {
    // SIGINT requests a normal RetroArch shutdown so the core can
    // finish writing native save data.
    if (!reaped) {
        std::printf("KiddOS: requesting RetroArch shutdown\n");
        std::fflush(stdout);

        if (kill(child, SIGINT) != 0 && errno != ESRCH)
            std::perror("KiddOS: SIGINT to RetroArch");

        waitForChild(child, status, reaped, std::chrono::seconds(10));
    }

    if (!reaped) {
        std::fprintf(stderr,
            "KiddOS: RetroArch did not exit; sending SIGTERM\n");

        if (kill(child, SIGTERM) != 0 && errno != ESRCH)
            std::perror("KiddOS: SIGTERM to RetroArch");

        waitForChild(child, status, reaped, std::chrono::seconds(1));
    }

    if (!reaped) {
        std::fprintf(stderr,
            "KiddOS: RetroArch did not stop; sending SIGKILL\n");

        if (kill(child, SIGKILL) != 0 && errno != ESRCH)
            std::perror("KiddOS: SIGKILL to RetroArch");

        while (waitpid(child, &status, 0) < 0) {
            if (errno == EINTR)
                continue;
            if (errno != ECHILD)
                std::perror("KiddOS: final waitpid for RetroArch");
            break;
        }

        reaped = true;
    }
}

} // namespace

bool launchGame(const Game& game) {
    if (!game.available || game.rom.empty() ||
        (game.platform != "gb" && game.platform != "gbc")) {
        std::fprintf(stderr,
            "KiddOS: refusing invalid or unsupported game slot\n");
        return false;
    }

    if (access(RETROARCH, X_OK) != 0) {
        std::fprintf(stderr,
            "KiddOS: RetroArch is unavailable: %s\n", RETROARCH);
        return false;
    }

    if (access(CORE, R_OK) != 0) {
        std::fprintf(stderr,
            "KiddOS: Gambatte core is unavailable: %s\n", CORE);
        return false;
    }

    if (access(CONFIG, R_OK) != 0) {
        std::fprintf(stderr,
            "KiddOS: RetroArch configuration is unavailable: %s\n",
            CONFIG);
        return false;
    }

    const std::string profile =
        std::string(PROFILE_DIR) + "/" + game.platform + ".cfg";
    if (access(profile.c_str(), R_OK) != 0) {
        std::fprintf(stderr,
            "KiddOS: RetroArch profile is unavailable: %s\n",
            profile.c_str());
        return false;
    }

    if (access(game.rom.c_str(), R_OK) != 0) {
        std::fprintf(stderr,
            "KiddOS: game ROM is missing or unreadable: %s\n",
            game.rom.c_str());
        return false;
    }

    const int gamepadFd = openGamepadEventDevice();

    std::printf("KiddOS: launching %s (%s)\n",
                game.name.c_str(), game.platform.c_str());
    std::fflush(stdout);

    const pid_t child = fork();

    if (child == 0) {
        if (gamepadFd >= 0)
            close(gamepadFd);

        execl(RETROARCH, "retroarch",
              "--config", CONFIG,
              "--appendconfig", profile.c_str(),
              "--set-shader", HANDHELD_SHADER,
              "-L", CORE,
              game.rom.c_str(),
              static_cast<char*>(nullptr));

        std::perror("KiddOS: exec RetroArch");
        _exit(127);
    }

    if (child < 0) {
        if (gamepadFd >= 0)
            close(gamepadFd);

        std::perror("KiddOS: fork for RetroArch");
        return false;
    }

    bool selectDown = false;
    bool startDown = false;
    bool chordLatched = false;
    bool haveFirstChord = false;
    bool requestedExit = false;
    bool childReaped = false;

    auto firstChord = std::chrono::steady_clock::time_point{};
    int status = 0;

    while (!childReaped) {
        if (reapChild(child, status, childReaped))
            break;

        pollfd pfd{};
        pfd.fd = gamepadFd;
        pfd.events = POLLIN;

        const int ready = poll(
            gamepadFd >= 0 ? &pfd : nullptr,
            gamepadFd >= 0 ? 1 : 0,
            50);

        if (ready < 0) {
            if (errno == EINTR)
                continue;
            std::perror("KiddOS: poll gamepad");
            continue;
        }

        if (gamepadFd < 0 || !(pfd.revents & POLLIN))
            continue;

        input_event event{};

        while (read(gamepadFd, &event, sizeof(event)) ==
               static_cast<ssize_t>(sizeof(event))) {
            if (event.type != EV_KEY)
                continue;

            const bool pressed = event.value != 0;

            if (event.code == BTN_TRIGGER_HAPPY1)
                selectDown = pressed;
            else if (event.code == BTN_TRIGGER_HAPPY2)
                startDown = pressed;
            else
                continue;

            const bool chordDown = selectDown && startDown;

            if (!chordDown) {
                if (!selectDown && !startDown)
                    chordLatched = false;
                continue;
            }

            if (chordLatched)
                continue;

            chordLatched = true;
            const auto now = std::chrono::steady_clock::now();

            if (haveFirstChord &&
                now - firstChord <= CHORD_WINDOW) {
                requestedExit = true;
                std::printf(
                    "KiddOS: confirmed double Start+Select; "
                    "returning to game library\n");
                std::fflush(stdout);
                break;
            }

            firstChord = now;
            haveFirstChord = true;

            std::printf(
                "KiddOS: Start+Select detected; press again to exit game\n");
            std::fflush(stdout);
        }

        if (requestedExit)
            break;
    }

    if (gamepadFd >= 0)
        close(gamepadFd);

    if (requestedExit)
        stopRetroArch(child, status, childReaped);

    if (!childReaped)
        return false;

    if (requestedExit) {
        std::printf(
            "KiddOS: game closed; returning to library\n");
        std::fflush(stdout);
        return true;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr,
            "KiddOS: RetroArch exited abnormally (status %d)\n",
            status);
        return false;
    }

    std::printf("KiddOS: game exited; returning to library\n");
    std::fflush(stdout);
    return true;
}

} // namespace KiddosServices
