// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "TVShowLauncher.hpp"

#include <SDL.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

namespace KiddosServices {
namespace fs = std::filesystem;
namespace {

constexpr const char* STATE_DIR = "/storage/.config/kiddos/tv-state";
constexpr const char* WATCH_DIR = "/storage/.config/kiddos/tv-watch-later";
constexpr const char* INPUT_NAME = "GO-Super Gamepad";
constexpr int SELECT_CODE = BTN_TRIGGER_HAPPY1;
constexpr int START_CODE = BTN_TRIGGER_HAPPY2;
constexpr int LEFT_SHOULDER = BTN_TL;
constexpr int RIGHT_SHOULDER = BTN_TR;
constexpr auto CHORD_WINDOW = std::chrono::milliseconds(1500);

std::string safeId(const std::string& value) {
    static const char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char c : value) {
        result.push_back(hex[c >> 4]);
        result.push_back(hex[c & 15]);
    }
    return result.empty() ? "show" : result;
}

int readSavedEpisode(const fs::path& path) {
    std::ifstream in(path);
    int index = 0;
    if (in >> index && index >= 0) return index;
    return 0;
}

void saveEpisode(const fs::path& path, int index) {
    std::ofstream out(path, std::ios::trunc);
    if (out) out << std::max(0, index) << '\n';
}

int findGamepad() {
    for (int n = 0; n < 64; ++n) {
        const std::string path = "/dev/input/event" + std::to_string(n);
        const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;

        char name[256] = {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
            std::strcmp(name, INPUT_NAME) == 0)
            return fd;
        close(fd);
    }
    return -1;
}

bool ipcSend(const std::string& socketPath, const std::string& command,
             std::string* response = nullptr) {
    const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return false;

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (socketPath.size() >= sizeof(address.sun_path)) {
        close(fd);
        return false;
    }
    std::strncpy(address.sun_path, socketPath.c_str(),
                 sizeof(address.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) != 0) {
        close(fd);
        return false;
    }

    const std::string line = command + "\n";
    const ssize_t sent = send(fd, line.data(), line.size(), MSG_NOSIGNAL);
    if (sent != static_cast<ssize_t>(line.size())) {
        close(fd);
        return false;
    }

    if (response) {
        response->clear();
        char buffer[1024];
        pollfd pfd{fd, POLLIN, 0};
        while (response->size() < 8192 &&
               poll(&pfd, 1, 700) > 0 && (pfd.revents & POLLIN)) {
            const ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
            if (count <= 0) break;
            response->append(buffer, static_cast<std::size_t>(count));
            if (response->find('\n') != std::string::npos) break;
        }
    }

    close(fd);
    return true;
}

int queryPlaylistPosition(const std::string& socketPath, int fallback) {
    std::string response;
    if (!ipcSend(socketPath,
        R"({"command":["get_property","playlist-pos"],"request_id":71})",
        &response))
        return fallback;

    const std::size_t data = response.find("\"data\"");
    if (data == std::string::npos) return fallback;
    const std::size_t colon = response.find(':', data);
    if (colon == std::string::npos) return fallback;

    char* end = nullptr;
    const long value = std::strtol(response.c_str() + colon + 1, &end, 10);
    if (end == response.c_str() + colon + 1 || value < 0 || value > 100000)
        return fallback;
    return static_cast<int>(value);
}

bool waitForEpisodeReady(const std::string& socketPath,
                         const std::string& expectedPath,
                         std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    do {
        std::string pathResponse;
        std::string seekableResponse;

        const bool gotPath = ipcSend(socketPath,
            R"({"command":["get_property","path"],"request_id":72})",
            &pathResponse);

        const bool gotSeekable = ipcSend(socketPath,
            R"({"command":["get_property","seekable"],"request_id":73})",
            &seekableResponse);

        if (gotPath && gotSeekable) {
            const std::size_t data = pathResponse.find("\"data\"");
            if (data != std::string::npos) {
                const std::size_t colon = pathResponse.find(':', data);
                const std::size_t quote = colon == std::string::npos
                    ? std::string::npos
                    : pathResponse.find('"', colon + 1);
                const std::size_t endQuote = quote == std::string::npos
                    ? std::string::npos
                    : pathResponse.find('"', quote + 1);

                const std::size_t seekData =
                    seekableResponse.find("\"data\"");
                const std::size_t seekColon =
                    seekData == std::string::npos
                        ? std::string::npos
                        : seekableResponse.find(':', seekData);
                const std::size_t seekValue =
                    seekColon == std::string::npos
                        ? std::string::npos
                        : seekableResponse.find_first_not_of(
                              " \t", seekColon + 1);

                if (quote != std::string::npos &&
                    endQuote != std::string::npos &&
                    pathResponse.substr(quote + 1,
                                        endQuote - quote - 1) == expectedPath &&
                    seekValue != std::string::npos &&
                    seekableResponse.compare(seekValue, 4, "true") == 0)
                    return true;
            }
        }

        usleep(100000);
    } while (std::chrono::steady_clock::now() < deadline);

    return false;
}

bool writePlaylist(const TVShow& show, const fs::path& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << "#EXTM3U\n";
    for (const auto& episode : show.episodes) {
        // M3U paths are emitted as absolute paths by the catalog.
        out << episode << '\n';
    }
    return static_cast<bool>(out);
}

} // namespace

bool launchTVShow(const TVShow& show) {
    if (show.episodes.empty()) {
        std::fprintf(stderr, "KiddOS: refusing to launch empty TV show: %s\n",
                     show.title.c_str());
        return false;
    }

    const int gamepad = findGamepad();
    if (gamepad < 0) {
        std::fprintf(stderr, "KiddOS: controller not found for TV playback\n");
        return false;
    }

    std::error_code ec;
    fs::create_directories(STATE_DIR, ec);
    if (ec) {
        std::fprintf(stderr, "KiddOS: cannot create TV state directory: %s\n",
                     ec.message().c_str());
        close(gamepad);
        return false;
    }
    fs::create_directories(WATCH_DIR, ec);
    if (ec) {
        std::fprintf(stderr, "KiddOS: cannot create MPV watch-later directory: %s\n",
                     ec.message().c_str());
        close(gamepad);
        return false;
    }

    const fs::path statePath =
        fs::path(STATE_DIR) / (safeId(show.directory) + ".txt");
    const int startIndex = std::min(
        readSavedEpisode(statePath),
        static_cast<int>(show.episodes.size()) - 1);

    const std::string suffix = std::to_string(getpid());
    const fs::path playlistPath = fs::path("/tmp") /
        ("kiddos-tv-" + suffix + ".m3u");
    const std::string socketPath = "/tmp/kiddos-tv-" + suffix + ".sock";

    if (!writePlaylist(show, playlistPath)) {
        std::fprintf(stderr, "KiddOS: cannot write TV playlist\n");
        close(gamepad);
        return false;
    }

    const pid_t child = fork();
    if (child < 0) {
        std::perror("KiddOS: fork for TV playback");
        fs::remove(playlistPath, ec);
        close(gamepad);
        return false;
    }

    if (child == 0) {
        if (gamepad >= 0)
            close(gamepad);

        execl("/usr/bin/mpv", "mpv",
              "--no-config",
              "--vo=drm",
              "--ao=alsa",
              "--audio-device=alsa/hw:0,0",
              "--audio-channels=stereo",
              "--fullscreen",
              "--video-aspect-override=1:1",
              "--osd-playing-msg=${media-title}",
              "--osd-playing-msg-duration=4000",
              "--input-gamepad=yes",
              "--input-conf=/storage/.config/kiddos/mpv-input.conf",
              "--save-position-on-quit=yes",
              "--resume-playback=yes",
              "--watch-later-directory=/storage/.config/kiddos/tv-watch-later",
              ("--input-ipc-server=" + socketPath).c_str(),
              ("--playlist-start=" + std::to_string(startIndex)).c_str(),
              ("--playlist=" + playlistPath.string()).c_str(),
              static_cast<char*>(nullptr));
        std::perror("KiddOS: exec mpv for TV playback");
        _exit(127);
    }

    std::printf("KiddOS: playing %s from episode %d of %zu\n",
                show.title.c_str(), startIndex + 1, show.episodes.size());

    int childStatus = 0;
    bool childReaped = false;
    auto reapChild = [&]() {
        if (childReaped) return true;
        pid_t result;
        do {
            result = waitpid(child, &childStatus, WNOHANG);
        } while (result < 0 && errno == EINTR);
        if (result == child || (result < 0 && errno == ECHILD)) {
            childReaped = true;
            return true;
        }
        return false;
    };

    // Wait briefly for MPV's IPC socket to become available.
    bool ipcReady = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (ipcSend(socketPath,
            R"({"command":["get_property","playlist-pos"],"request_id":1})"))
        {
            ipcReady = true;
            break;
        }
        if (reapChild()) break;
        usleep(50000);
    }

    bool startDown = false;
    bool selectDown = false;
    bool chordLatched = false;
    bool haveFirstChord = false;
    auto firstChord = std::chrono::steady_clock::time_point{};
    int lastKnownPosition = startIndex;
    bool quitting = false;

    while (!quitting) {
        if (reapChild()) break;

        pollfd pfd{gamepad, POLLIN, 0};
        const int ready = poll(&pfd, 1, 100);
        if (ready <= 0 || !(pfd.revents & POLLIN)) continue;

        input_event event{};
        while (read(gamepad, &event, sizeof(event)) ==
               static_cast<ssize_t>(sizeof(event))) {
            if (event.type != EV_KEY) continue;
            const bool pressed = event.value != 0;

            if (event.code == START_CODE) startDown = pressed;
            if (event.code == SELECT_CODE) selectDown = pressed;

            if (event.code == LEFT_SHOULDER && pressed && ipcReady) {
                ipcSend(socketPath, R"({"command":["playlist-prev"]})");
            } else if (event.code == RIGHT_SHOULDER && pressed && ipcReady) {
                ipcSend(socketPath, R"({"command":["playlist-next"]})");
            }

            const bool chordDown = startDown && selectDown;
            if (chordDown && !chordLatched) {
                const auto now = std::chrono::steady_clock::now();
                if (haveFirstChord && now - firstChord <= CHORD_WINDOW) {
                    // Let the common shutdown path request MPV quit once.
                    quitting = true;
                } else {
                    firstChord = now;
                    haveFirstChord = true;
                }
            }
            chordLatched = chordDown;
        }
    }

    if (ipcReady) {
        lastKnownPosition =
            queryPlaylistPosition(socketPath, lastKnownPosition);
        lastKnownPosition = std::clamp(
            lastKnownPosition, 0, static_cast<int>(show.episodes.size()) - 1);

        // Wait for the selected episode itself, not the previous file, to load.
        const bool episodeReady = waitForEpisodeReady(
            socketPath, show.episodes[lastKnownPosition],
            std::chrono::seconds(5));
        if (!episodeReady)
            std::fprintf(stderr,
                "Kiddos: timed out waiting for selected TV episode to load\n");

        saveEpisode(statePath, lastKnownPosition);
        if (!childReaped)
            ipcSend(socketPath, R"({"command":["quit"]})");
    }

    // Never wait indefinitely for MPV: allow graceful exit, then escalate.
    auto waitForChild = [&](std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!childReaped &&
               std::chrono::steady_clock::now() < deadline) {
            if (reapChild()) break;
            usleep(50000);
        }
    };

    if (!childReaped) {
        waitForChild(std::chrono::seconds(10));
    }
    if (!childReaped) {
        if (kill(child, SIGTERM) != 0 && errno != ESRCH)
            std::perror("KiddOS: SIGTERM to MPV");
        waitForChild(std::chrono::seconds(1));
    }
    if (!childReaped) {
        if (kill(child, SIGKILL) != 0 && errno != ESRCH)
            std::perror("KiddOS: SIGKILL to MPV");
        while (waitpid(child, &childStatus, 0) < 0) {
            if (errno == EINTR) continue;
            if (errno != ECHILD)
                std::perror("KiddOS: waitpid for MPV");
            break;
        }
        childReaped = true;
    }

    close(gamepad);
    fs::remove(playlistPath, ec);
    return true;
}

} // namespace KiddosServices
