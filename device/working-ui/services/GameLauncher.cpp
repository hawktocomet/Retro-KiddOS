// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "GameLauncher.hpp"

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {

static const char* CORE =
    "/storage/roms/.kiddos-ui-test-20261116/gambatte-link_libretro.so";

bool launchGame(const Game& game) {
    if (!game.available || game.rom.empty() || game.platform.empty()) {
        std::fprintf(stderr, "KiddOS: refusing to launch an empty game slot\n");
        return false;
    }

    const char* rom = game.rom.c_str();
    std::printf("KiddOS: launching %s\n", game.name.c_str());
    std::fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        execlp("cp", "cp", CORE, "/tmp/cores/gambatte_libretro.so", (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("KiddOS: fork for core copy");
        return false;
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "KiddOS: patched core staging failed\n");
        return false;
    }

    pid = fork();
    if (pid == 0) {
        execl("/usr/bin/runemu.py", "runemu.py",
              "--rom", rom,
              "--platform", game.platform.c_str(),
              "--emulator", "retroarch",
              "--core", "gambatte",
              (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("KiddOS: fork for game");
        return false;
    }
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("KiddOS: wait for game");
        return false;
    }
    std::printf("KiddOS: game exited (status %d); returning to menu\n", status);
    std::fflush(stdout);
    return true;
}

} // namespace KiddosServices
