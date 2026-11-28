// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "pin_store.h"


#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosPin {

static bool validPin(const char* pin) {
    if (!pin || std::strlen(pin) != 4) return false;
    for (int i = 0; i < 4; ++i)
        if (pin[i] < '0' || pin[i] > '9') return false;
    return true;
}

static bool runOpenSSL(const char* pin, const char* salt,
                       std::string& result) {
    int inputPipe[2], outputPipe[2];
    if (pipe(inputPipe) != 0) return false;
    if (pipe(outputPipe) != 0) {
        close(inputPipe[0]);
        close(inputPipe[1]);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(inputPipe[0]); close(inputPipe[1]);
        close(outputPipe[0]); close(outputPipe[1]);
        return false;
    }

    if (pid == 0) {
        dup2(inputPipe[0], STDIN_FILENO);
        dup2(outputPipe[1], STDOUT_FILENO);
        close(inputPipe[0]); close(inputPipe[1]);
        close(outputPipe[0]); close(outputPipe[1]);
        execl("/usr/bin/openssl", "openssl", "passwd", "-6",
              "-salt", salt, "-stdin", (char*)nullptr);
        _exit(127);
    }

    close(inputPipe[0]);
    close(outputPipe[1]);

    std::string input(pin);
    input.push_back('\n');
    size_t written = 0;
    while (written < input.size()) {
        ssize_t n = write(inputPipe[1], input.data() + written,
                          input.size() - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        written += static_cast<size_t>(n);
    }
    close(inputPipe[1]);

    char buffer[512];
    result.clear();
    bool readOk = true;
    for (;;) {
        ssize_t n = read(outputPipe[0], buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { readOk = false; break; }
        if (n == 0) break;
        result.append(buffer, static_cast<size_t>(n));
        if (result.size() > 256) { readOk = false; break; }
    }
    close(outputPipe[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return false;
    }
    if (!readOk || written != input.size() ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return false;

    while (!result.empty() &&
           (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result.compare(0, 3, "$6$") == 0;
}

static bool makeSalt(std::string& salt) {
    unsigned char bytes[8];
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return false;

    size_t count = 0;
    while (count < sizeof(bytes)) {
        ssize_t n = read(fd, bytes + count, sizeof(bytes) - count);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { close(fd); return false; }
        count += static_cast<size_t>(n);
    }
    close(fd);

    static const char hex[] = "0123456789abcdef";
    salt.clear();
    for (unsigned char byte : bytes) {
        salt.push_back(hex[byte >> 4]);
        salt.push_back(hex[byte & 15]);
    }
    return true;
}

static bool makePaths(const char* directory, std::string& file) {
    if (!directory || !*directory) return false;
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) return false;

    struct stat st;
    if (stat(directory, &st) != 0 || !S_ISDIR(st.st_mode)) return false;
    if (chmod(directory, 0700) != 0) return false;

    file = std::string(directory) + "/pin.hash";
    return true;
}

static bool readHash(const std::string& file, std::string& hash) {
    int fd = open(file.c_str(), O_RDONLY | O_NOFOLLOW);
    if (fd < 0) return false;

    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_size <= 0 || st.st_size > 256) {
        close(fd);
        return false;
    }

    char buffer[257];
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (n <= 0) return false;

    hash.assign(buffer, static_cast<size_t>(n));
    while (!hash.empty() &&
           (hash.back() == '\n' || hash.back() == '\r'))
        hash.pop_back();

    size_t first = hash.find('$', 1);
    size_t second = first == std::string::npos
        ? first : hash.find('$', first + 1);
    if (hash.compare(0, 3, "$6$") != 0 ||
        first != 2 || second == std::string::npos ||
        second - first < 2 || second - first > 17)
        return false;

    return true;
}

static bool writeHash(const std::string& file,
                      const std::string& hash) {
    char temporary[512];
    int length = std::snprintf(temporary, sizeof(temporary),
        "%s.tmp.%ld", file.c_str(), static_cast<long>(getpid()));
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(temporary))
        return false;

    int fd = open(temporary, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
                  0600);
    if (fd < 0) return false;

    std::string data = hash + "\n";
    size_t written = 0;
    while (written < data.size()) {
        ssize_t n = write(fd, data.data() + written,
                          data.size() - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        written += static_cast<size_t>(n);
    }

    bool ok = written == data.size() &&
              fsync(fd) == 0 && fchmod(fd, 0600) == 0;
    if (close(fd) != 0) ok = false;

    if (!ok || rename(temporary, file.c_str()) != 0) {
        unlink(temporary);
        return false;
    }
    return true;
}

static bool savePin(const char* directory, const char* pin) {
    if (!validPin(pin)) return false;

    std::string file, salt, hash;
    if (!makePaths(directory, file) || !makeSalt(salt) ||
        !runOpenSSL(pin, salt.c_str(), hash))
        return false;

    return writeHash(file, hash);
}

bool initializePinStore(const char* directory) {
    std::string file, existing;
    if (!makePaths(directory, file)) return false;

    if (readHash(file, existing)) return true;

    if (access(file.c_str(), F_OK) == 0 || errno != ENOENT)
        return false;

    return savePin(directory, "1234");
}

bool verifyPin(const char* directory, const char* pin) {
    if (!validPin(pin)) return false;

    std::string file, stored;
    if (!makePaths(directory, file) || !readHash(file, stored))
        return false;

    size_t first = stored.find('$', 1);
    size_t second = stored.find('$', first + 1);
    std::string salt = stored.substr(first + 1, second - first - 1);

    std::string candidate;
    if (!runOpenSSL(pin, salt.c_str(), candidate)) return false;

    if (candidate.size() != stored.size()) return false;
    unsigned char difference = 0;
    for (size_t i = 0; i < stored.size(); ++i)
        difference |= static_cast<unsigned char>(candidate[i] ^ stored[i]);
    return difference == 0;
}

// A parent must prove knowledge of the current PIN before changing it.
bool changePin(const char* directory,
                      const char* currentPin,
                      const char* newPin) {
    if (!validPin(currentPin) || !validPin(newPin)) return false;
    if (!verifyPin(directory, currentPin)) return false;
    return savePin(directory, newPin);
}

} // namespace KiddosPin
