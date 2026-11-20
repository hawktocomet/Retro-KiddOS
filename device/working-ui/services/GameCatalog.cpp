// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "GameCatalog.hpp"

#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <utility>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../models/Games.hpp"

namespace KiddosServices {
namespace {

// This fixed script delegates discovery, slot validation, and artwork matching
// to the same catalog module used by Remote Management.
constexpr const char* kCatalogScript = R"PY(
import sys
sys.path.insert(0, "/storage/.kiddos-management")
import catalog

data = catalog.game_catalog(
    "/storage/roms",
    "/storage/.kiddos-management"
)

out = sys.stdout.buffer
for slot in data["slots"]:
    game = slot["game"]
    fields = (
        game["title"] if game else "",
        game["system_id"] if game else "",
        game["path"] if game else "",
        (game["artwork"] or "") if game else "",
    )
    for field in fields:
        out.write(field.encode("utf-8") + b"\0")
)PY";

} // namespace

bool loadGameSlots() {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        std::perror("KiddOS: catalog pipe");
        return false;
    }

    const pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0)
            _exit(126);
        close(pipefd[1]);
        execlp("python3", "python3", "-c", kCatalogScript,
               static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);
    if (pid < 0) {
        close(pipefd[0]);
        std::perror("KiddOS: catalog fork");
        return false;
    }

    std::vector<char> output;
    char buffer[1024];
    bool readOk = true;

    for (;;) {
        const ssize_t count = read(pipefd[0], buffer, sizeof(buffer));
        if (count > 0) {
            output.insert(output.end(), buffer, buffer + count);
        } else if (count == 0) {
            break;
        } else if (errno != EINTR) {
            std::perror("KiddOS: catalog read");
            readOk = false;
            break;
        }
    }
    close(pipefd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        std::perror("KiddOS: catalog wait");
        return false;
    }

    if (!readOk || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "KiddOS: device game catalog could not be loaded\n");
        return false;
    }

    std::vector<std::string> fields;
    std::string field;
    for (char value : output) {
        if (value == '\0') {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(value);
        }
    }

    if (!field.empty() || fields.size() != 16) {
        std::fprintf(stderr,
                     "KiddOS: catalog returned an invalid slot response (%zu fields)\n",
                     fields.size());
        return false;
    }

    auto nextGames = games;
    for (std::size_t i = 0; i < nextGames.size(); ++i) {
        const std::size_t offset = i * 4;
        Game game;
        game.name = fields[offset];
        game.platform = fields[offset + 1];
        game.rom = fields[offset + 2];
        game.image = fields[offset + 3];
        game.available = !game.name.empty();

        if (game.available &&
            (game.platform.empty() || game.rom.empty())) {
            std::fprintf(stderr,
                         "KiddOS: catalog slot %zu is missing required game data\n",
                         i + 1);
            return false;
        }
        nextGames[i] = std::move(game);
    }

    games = std::move(nextGames);

    for (std::size_t i = 0; i < games.size(); ++i) {
        std::printf("KiddOS: slot %zu: %s\n", i + 1,
                    games[i].available ? games[i].name.c_str() : "(empty)");
    }
    std::fflush(stdout);
    return true;
}

} // namespace KiddosServices
