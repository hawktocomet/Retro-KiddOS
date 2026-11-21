// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

namespace kiddos {

class VolumeController {
public:
    VolumeController() = default;
    ~VolumeController();

    VolumeController(const VolumeController&) = delete;
    VolumeController& operator=(const VolumeController&) = delete;

    bool initialize(bool apply);
    void poll();

private:
    int up_fd_ = -1;
    int down_fd_ = -1;
    int last_up_ = 1;
    int last_down_ = 1;
    int candidate_up_ = 1;
    int candidate_down_ = 1;
    int up_samples_ = 0;
    int down_samples_ = 0;
    int volume_ = 0;
    bool apply_ = false;

    bool openGpio(int number, int& fd);
    bool readGpio(int fd, int& value);
    bool readMixer(int& left, int& right);
    bool writeMixer(int value);
    void adjust(int direction);
};

}  // namespace kiddos
