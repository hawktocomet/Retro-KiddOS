// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "PowerService.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace KiddosServices {

bool requestShutdown() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/systemctl", "systemctl", "poweroff", (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("Retro KiddOS: shutdown fork failed");
        return false;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("Retro KiddOS: shutdown wait failed");
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "Retro KiddOS: systemctl poweroff failed (status %d)\n", status);
        return false;
    }
    return true;
}

} // namespace KiddosServices
