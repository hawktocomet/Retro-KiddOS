// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MovieCatalog.hpp"

#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <utility>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {
namespace {

std::vector<Movie> movieList;

constexpr const char* kMovieCatalogScript = R"PY(
from pathlib import Path
import sys

directory = Path("/storage/roms/movies")
extensions = {".mp4", ".mkv", ".avi", ".mov", ".m4v"}

if directory.is_dir():
    paths = sorted(
        (path for path in directory.iterdir()
         if path.is_file() and path.suffix.lower() in extensions),
        key=lambda path: path.name.casefold()
    )
else:
    paths = []

out = sys.stdout.buffer
for path in paths:
    out.write(path.stem.encode("utf-8") + b"\0")
    out.write(str(path).encode("utf-8") + b"\0")
)PY";

} // namespace

bool loadMovies() {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        std::perror("KiddOS: movie catalog pipe");
        return false;
    }

    const pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0)
            _exit(126);
        close(pipefd[1]);
        execlp("python3", "python3", "-c", kMovieCatalogScript,
               static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);
    if (pid < 0) {
        close(pipefd[0]);
        std::perror("KiddOS: movie catalog fork");
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
            std::perror("KiddOS: movie catalog read");
            readOk = false;
            break;
        }
    }
    close(pipefd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        std::perror("KiddOS: movie catalog wait");
        return false;
    }

    if (!readOk || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "KiddOS: movie catalog could not be loaded\n");
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

    if (!field.empty() || fields.size() % 2 != 0) {
        std::fprintf(stderr,
                     "KiddOS: movie catalog returned an invalid response\n");
        return false;
    }

    std::vector<Movie> nextMovies;
    nextMovies.reserve(fields.size() / 2);

    for (std::size_t i = 0; i < fields.size(); i += 2) {
        if (fields[i].empty() || fields[i + 1].empty()) {
            std::fprintf(stderr,
                         "KiddOS: movie catalog returned incomplete movie data\n");
            return false;
        }

        Movie movie;
        movie.title = std::move(fields[i]);
        movie.path = std::move(fields[i + 1]);
        nextMovies.push_back(std::move(movie));
    }

    movieList = std::move(nextMovies);

    std::printf("KiddOS: loaded %zu movies\n", movieList.size());
    std::fflush(stdout);
    return true;
}

const std::vector<Movie>& movies() {
    return movieList;
}

} // namespace KiddosServices
