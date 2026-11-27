// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include <cstdlib>

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
bool paused = false;
}

bool launchMusic(const MusicTrack& track) {
    if (track.path.empty() || access(track.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: music file missing or unreadable: %s\n",
                     track.path.c_str());
        return false;
    }

    const pid_t pid = fork();
    if (pid == 0) {
        if (kill(getpid(), SIGSTOP) != 0) _exit(126);
        if (setenv("ALSA_CONFIG_PATH",
                   "/storage/.config/kiddos/alsa-dmix.conf", 1) != 0)
            _exit(126);
        execl("/usr/bin/mpv", "mpv", "--no-config",
              "--no-video", "--ao=alsa", "--audio-device=alsa/dmix",
              "--audio-format=s16", "--audio-samplerate=48000",
              "--audio-channels=stereo",
              track.path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    if (pid < 0) {
        std::perror("KiddOS: music fork");
        return false;
    }

    int status = 0;
    pid_t result;
    do {
        result = waitpid(pid, &status, WUNTRACED);
    } while (result < 0 && errno == EINTR);

    if (result != pid || !WIFSTOPPED(status)) {
        kill(pid, SIGKILL);
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
        std::fprintf(stderr, "KiddOS: replacement music process failed to stop\n");
        return false;
    }

    stopMusic();

    if (kill(pid, SIGCONT) != 0) {
        std::perror("KiddOS: resume replacement music process");
        kill(pid, SIGKILL);
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
        return false;
    }

    musicPid = pid;
    paused = false;
    std::printf("KiddOS: now playing %s\n", track.title.c_str());
    std::fflush(stdout);
    return true;
}
void stopMusic() {
    if (musicPid <= 0) {
        paused = false;
        return;
    }

    const pid_t pid = musicPid;
    kill(pid, SIGCONT);
    kill(pid, SIGTERM);

    int status = 0;
    for (int attempt = 0; attempt < 100; ++attempt) {
        const pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid || (result < 0 && errno == ECHILD))
            break;
        if (result < 0 && errno != EINTR)
            break;
        usleep(10000);
    }

    if (waitpid(pid, &status, WNOHANG) == 0) {
        kill(pid, SIGKILL);
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    }

    musicPid = -1;
    paused = false;
}

bool pauseMusic() {
    if (musicPid <= 0 || paused) return musicPid > 0;
    if (kill(musicPid, SIGSTOP) != 0) return false;
    paused = true;
    return true;
}

bool resumeMusic() {
    if (musicPid <= 0) return false;
    if (!paused) return true;
    if (kill(musicPid, SIGCONT) != 0) return false;
    paused = false;
    return true;
}

bool musicPaused() {
    return musicPid > 0 && paused;
}

bool musicPlaying() {
    if (musicPid <= 0) return false;

    int status = 0;
    const pid_t result = waitpid(musicPid, &status, WNOHANG);
    if (result == 0) return true;

    if (result == musicPid || (result < 0 && errno == ECHILD)) {
        musicPid = -1;
        paused = false;
        return false;
    }

    return true;
}
}
