// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include <linux/joystick.h>
#include "BrightnessPolicy.hpp"
#include "VolumeController.hpp"
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace {
volatile sig_atomic_t running = 1;

constexpr const char* kJoystick = "/dev/input/js0";
constexpr const char* kBrightness =
    "/sys/class/backlight/backlight/brightness";
constexpr const char* kMaxBrightness =
    "/sys/class/backlight/backlight/max_brightness";

void stop(int) { running = 0; }

bool readInteger(const char* path, int& result) {
    std::ifstream input(path);
    return static_cast<bool>(input >> result);
}

bool writeInteger(const char* path, int value) {
    std::ofstream output(path);
    if (!output) return false;
    output << value << '\n';
    return output.good();
}

int openJoystick() {
    while (running) {
        int fd = open(kJoystick, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd >= 0) {
            std::printf("Opened %s\n", kJoystick);
            return fd;
        }

        std::fprintf(stderr, "Waiting for joystick: %s\n",
                     std::strerror(errno));
        for (int i = 0; i < 10 && running; ++i)
            usleep(100000);
    }
    return -1;
}
}

int main(int argc, char** argv) {
    bool apply = false;
    if (argc == 2 && std::strcmp(argv[1], "--apply") == 0) {
        apply = true;
    } else if (argc != 1) {
        std::fprintf(stderr, "Usage: %s [--apply]\n", argv[0]);
        return 2;
    }

    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    int brightness = 50;
    int maximum = 99;

    if (!readInteger(kBrightness, brightness)) {
        std::fprintf(stderr,
                     "Cannot read current brightness; using 50 as observation baseline\n");
    }
    if (!readInteger(kMaxBrightness, maximum) || maximum < 1)
        maximum = 99;

    brightness = std::clamp(brightness, 1, std::min(maximum, 99));

    std::printf("KiddOS hardware service: mode=%s, brightness=%d/%d\n",
                apply ? "APPLY" : "OBSERVE ONLY", brightness, maximum);

    kiddos::VolumeController volume;
    const bool volume_ready = volume.initialize(apply);
    if (!volume_ready)
        std::fprintf(stderr, "Volume controller unavailable; brightness remains active\n");

    while (running) {
        int fd = openJoystick();
        if (fd < 0) break;

        pollfd descriptor{fd, POLLIN, 0};

        while (running) {
            descriptor.revents = 0;
            if (volume_ready) volume.poll();

            int ready = poll(&descriptor, 1, 10);

            if (ready < 0) {
                if (errno == EINTR) continue;
                std::perror("joystick poll");
                break;
            }
            if (ready == 0) continue;

            if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::fprintf(stderr, "Joystick disconnected\n");
                break;
            }
            if (!(descriptor.revents & POLLIN))
                continue;

            bool reconnect = false;
            while (true) {
                js_event event{};
                ssize_t count = read(fd, &event, sizeof(event));

                if (count < 0 && errno == EINTR)
                    continue;
                if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    break;
                if (count != static_cast<ssize_t>(sizeof(event))) {
                    std::fprintf(stderr,
                                 "Joystick read failed or returned a short event\n");
                    reconnect = true;
                    break;
                }

                if ((event.type & ~JS_EVENT_INIT) != JS_EVENT_BUTTON ||
                    (event.type & JS_EVENT_INIT) || event.value != 1)
                    continue;

                int next = kiddos::adjustBrightness(
                    brightness, maximum, event.number);
                if (next == brightness)
                    continue;

                std::printf("Button %u: brightness %d -> %d%s\n",
                            event.number, brightness, next,
                            apply ? "" : " (not written)");

                if (apply && !writeInteger(kBrightness, next)) {
                    std::fprintf(stderr, "Failed to write brightness: %s\n",
                                 std::strerror(errno));
                    continue;
                }

                brightness = next;
                std::fflush(stdout);
            }

            if (reconnect) break;
        }

        close(fd);
    }

    std::puts("KiddOS hardware service stopped.");
    return 0;
}
