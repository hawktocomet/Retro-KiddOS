// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "MusicCatalog.hpp"
#include <cerrno>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {
namespace {
std::vector<MusicTrack> trackList;
constexpr const char* kScript = R"PY(
from pathlib import Path
import sys
directory = Path("/storage/roms/music")
extensions = {".mp3", ".m4a", ".aac", ".flac", ".ogg", ".opus", ".wav", ".wma"}
paths = sorted((p for p in directory.iterdir()
                if p.is_file() and p.suffix.lower() in extensions),
               key=lambda p: p.name.casefold()) if directory.is_dir() else []
out = sys.stdout.buffer
for path in paths[:4]:
    out.write(path.stem.encode("utf-8") + b"\0")
    out.write(str(path).encode("utf-8") + b"\0")
)PY";
}
bool loadMusic() {
    int fds[2];
    if (pipe(fds) != 0) { std::perror("KiddOS: music pipe"); return false; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0) _exit(126);
        close(fds[1]);
        execlp("python3", "python3", "-c", kScript, static_cast<char*>(nullptr));
        _exit(127);
    }
    close(fds[1]);
    if (pid < 0) { close(fds[0]); std::perror("KiddOS: music fork"); return false; }
    std::vector<char> output;
    char buffer[1024];
    bool ok = true;
    for (;;) {
        ssize_t n = read(fds[0], buffer, sizeof(buffer));
        if (n > 0) output.insert(output.end(), buffer, buffer+n);
        else if (n == 0) break;
        else if (errno != EINTR) { ok = false; break; }
    }
    close(fds[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        return false;
    }
    if (!ok || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return false;
    std::vector<std::string> fields;
    std::string field;
    for (char c : output) {
        if (c == '\0') { fields.push_back(field); field.clear(); }
        else field.push_back(c);
    }
    if (!field.empty() || fields.size() % 2) return false;
    std::vector<MusicTrack> next;
    for (std::size_t i=0; i<fields.size(); i+=2) {
        if (fields[i].empty() || fields[i+1].empty()) return false;
        next.push_back({std::move(fields[i]), std::move(fields[i+1])});
    }
    trackList = std::move(next);
    std::printf("KiddOS: loaded %zu music tracks\n", trackList.size());
    std::fflush(stdout);
    return true;
}
const std::vector<MusicTrack>& music() { return trackList; }
}
