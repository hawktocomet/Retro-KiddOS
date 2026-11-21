// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "VolumeController.hpp"
#include "VolumePolicy.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace {
constexpr const char* kGpioRoot = "/sys/class/gpio";
constexpr const char* kMixer = "/usr/bin/amixer";

bool writeText(const std::string& path, const char* value) {
    std::ofstream output(path);
    if (!output) return false;
    output << value;
    return output.good();
}

bool exportGpio(int number) {
    const std::string directory =
        std::string(kGpioRoot) + "/gpio" + std::to_string(number);

    if (access(directory.c_str(), F_OK) == 0)
        return true;

    const int fd = open((std::string(kGpioRoot) + "/export").c_str(),
                        O_WRONLY | O_CLOEXEC);
    if (fd < 0) return false;

    const std::string text = std::to_string(number);
    const ssize_t count = write(fd, text.c_str(), text.size());
    close(fd);

    if (count != static_cast<ssize_t>(text.size()) &&
        access(directory.c_str(), F_OK) != 0)
        return false;

    for (int i = 0; i < 100; ++i) {
        if (access(directory.c_str(), F_OK) == 0)
            return true;
        usleep(10000);
    }
    return false;
}
}

namespace kiddos {

VolumeController::~VolumeController() {
    if (up_fd_ >= 0) close(up_fd_);
    if (down_fd_ >= 0) close(down_fd_);
}

bool VolumeController::openGpio(int number, int& fd) {
    if (!exportGpio(number)) {
        std::fprintf(stderr, "Volume: cannot export GPIO %d\n", number);
        return false;
    }

    const std::string base =
        std::string(kGpioRoot) + "/gpio" + std::to_string(number) + "/";

    if (!writeText(base + "direction", "in") ||
        !writeText(base + "active_low", "0")) {
        std::fprintf(stderr, "Volume: cannot configure GPIO %d\n", number);
        return false;
    }

    fd = open((base + "value").c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        std::fprintf(stderr, "Volume: cannot open GPIO %d: %s\n",
                     number, std::strerror(errno));
        return false;
    }
    return true;
}

bool VolumeController::readGpio(int fd, int& value) {
    char buffer[8]{};
    if (lseek(fd, 0, SEEK_SET) < 0) return false;

    const ssize_t count = read(fd, buffer, sizeof(buffer) - 1);
    if (count <= 0) return false;

    char* end = nullptr;
    const long parsed = std::strtol(buffer, &end, 10);
    if (end == buffer || (parsed != 0 && parsed != 1)) return false;

    value = static_cast<int>(parsed);
    return true;
}

bool VolumeController::readMixer(int& left, int& right) {
    FILE* pipe = popen(
        "/usr/bin/amixer -c 0 cget numid=3 2>/dev/null", "r");
    if (!pipe) return false;

    char line[256];
    bool found = false;
    while (std::fgets(line, sizeof(line), pipe)) {
        if (std::sscanf(line, " : values=%d,%d", &left, &right) == 2) {
            found = true;
            break;
        }
    }

    const int status = pclose(pipe);
    return found && status == 0;
}

bool VolumeController::writeMixer(int value) {
    char argument[32];
    std::snprintf(argument, sizeof(argument), "%d,%d", value, value);

    const pid_t child = fork();
    if (child < 0) return false;

    if (child == 0) {
        const int nullfd = open("/dev/null", O_WRONLY);
        if (nullfd >= 0) {
            dup2(nullfd, STDOUT_FILENO);
            dup2(nullfd, STDERR_FILENO);
            close(nullfd);
        }
        execl(kMixer, "amixer", "-c", "0", "cset", "numid=3",
              argument, static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool VolumeController::initialize(bool apply) {
    apply_ = apply;

    if (!openGpio(123, up_fd_) || !openGpio(116, down_fd_))
        return false;

    int left = 0;
    int right = 0;
    if (!readMixer(left, right)) {
        std::fprintf(stderr, "Volume: cannot read current mixer volume\n");
        return false;
    }

    // The legacy control uses equal left/right values. Preserve the
    // average as the logical starting point if they differ.
    const int average = (left + right + 1) / 2;
    volume_ = logicalVolumeFromMixer(average);

    if (!readGpio(up_fd_, last_up_) ||
        !readGpio(down_fd_, last_down_)) {
        std::fprintf(stderr, "Volume: cannot read initial GPIO state\n");
        return false;
    }

    candidate_up_ = last_up_;
    candidate_down_ = last_down_;
    up_samples_ = 4;
    down_samples_ = 4;

    std::printf("Volume controller: %d/100 (mixer %d,%d), mode=%s\n",
                volume_, left, right,
                apply_ ? "APPLY" : "OBSERVE ONLY");
    return true;
}

void VolumeController::adjust(int direction) {
    const int next = adjustVolume(volume_, direction);
    if (next == volume_) return;

    const int mixer = mixerVolumeFromLogical(next);
    std::printf("Volume: %d -> %d (mixer %d,%d)%s\n",
                volume_, next, mixer, mixer,
                apply_ ? "" : " (not written)");

    if (apply_ && !writeMixer(mixer)) {
        std::fprintf(stderr, "Volume: mixer write failed\n");
        return;
    }

    volume_ = next;
    std::fflush(stdout);
}

void VolumeController::poll() {
    auto update = [](int sample, int& candidate, int& samples,
                     int& stable) {
        if (sample != candidate) {
            candidate = sample;
            samples = 1;
            return false;
        }

        if (samples < 4)
            ++samples;

        if (samples == 4 && stable != candidate) {
            stable = candidate;
            return true;
        }

        return false;
    };

    int up = last_up_;
    int down = last_down_;

    if (readGpio(up_fd_, up) &&
        update(up, candidate_up_, up_samples_, last_up_) &&
        last_up_ == 0)
        adjust(1);

    if (readGpio(down_fd_, down) &&
        update(down, candidate_down_, down_samples_, last_down_) &&
        last_down_ == 0)
        adjust(-1);
}

}  // namespace kiddos
