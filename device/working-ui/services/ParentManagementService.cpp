// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "ParentManagementService.hpp"

#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace KiddosServices {

bool managementControl(const char* action, char* output, size_t outputSize) {
    if (!output || outputSize == 0) return false;
    output[0] = '\0';

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        std::perror("KiddOS: management pipe");
        return false;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
        close(pipefd[1]);
        execl("/storage/.kiddos-management/control.sh",
              "control.sh", action, (char*)nullptr);
        _exit(127);
    }

    close(pipefd[1]);
    if (pid < 0) {
        close(pipefd[0]);
        std::perror("KiddOS: management fork");
        return false;
    }

    size_t used = 0;
    while (used + 1 < outputSize) {
        ssize_t n = read(pipefd[0], output + used, outputSize - used - 1);
        if (n <= 0) break;
        used += static_cast<size_t>(n);
    }
    close(pipefd[0]);
    output[used] = '\0';

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("KiddOS: management wait");
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace KiddosServices
