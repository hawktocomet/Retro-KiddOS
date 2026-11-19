// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "WifiService.hpp"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <utility>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {

WifiReading readWifiStatus() {
    WifiReading reading;

    int pipefd[2];
    if (pipe(pipefd) != 0) return reading;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return reading;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
        close(pipefd[1]);
        execl("/usr/bin/connmanctl", "connmanctl", "technologies",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);
    char output[4096];
    size_t used = 0;
    while (used + 1 < sizeof(output)) {
        ssize_t n = read(pipefd[0], output + used,
                         sizeof(output) - used - 1);
        if (n <= 0) break;
        used += static_cast<size_t>(n);
    }
    close(pipefd[0]);
    output[used] = '\0';

    int status = 0;
    if (waitpid(pid, &status, 0) < 0 ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return reading;

    const char* wifi = std::strstr(
        output, "/net/connman/technology/wifi");
    if (!wifi) return reading;

    const char* nextTechnology = std::strstr(wifi + 1, "\n/");
    const char* sectionEnd = nextTechnology ? nextTechnology : output + used;

    const char* poweredOn = std::strstr(wifi, "Powered = True");
    const char* poweredOff = std::strstr(wifi, "Powered = False");
    const char* connected = std::strstr(wifi, "Connected = True");
    const char* disconnected = std::strstr(wifi, "Connected = False");

    if (poweredOn && poweredOn < sectionEnd) {
        reading.powered = true;
        reading.powerKnown = true;
    } else if (poweredOff && poweredOff < sectionEnd) {
        reading.powered = false;
        reading.powerKnown = true;
    }

    if (connected && connected < sectionEnd) {
        reading.status = WifiStatus::Connected;
    } else if (disconnected && disconnected < sectionEnd) {
        reading.status = WifiStatus::Disconnected;
    }

    return reading;
}


WifiScanResult loadWifiNetworks(const std::string& helperPath, bool scan) {
    WifiScanResult result;

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        result.error = WifiScanError::StartFailed;
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        result.error = WifiScanError::StartFailed;
        return result;
    }

    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        const char* command = scan ? "ui-scan" : "ui-list";
        execl("/usr/bin/python3", "python3",
              helperPath.c_str(), command,
              static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);

    std::string output;
    char buffer[512];
    ssize_t count;
    while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(count));
    }
    close(pipefd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            result.error = WifiScanError::HelperFailed;
            return result;
        }
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        result.error = WifiScanError::ScanFailed;
        return result;
    }

    std::istringstream lines(output);
    std::string line;

    while (std::getline(lines, line)) {
        if (line.empty()) continue;

        std::istringstream fields(line);
        WifiNetwork network;
        std::string strength;

        if (!std::getline(fields, network.path, '\t') ||
            !std::getline(fields, network.name, '\t') ||
            !std::getline(fields, strength, '\t') ||
            !std::getline(fields, network.security, '\t') ||
            !std::getline(fields, network.state)) {
            continue;
        }

        try {
            network.strength = std::stoi(strength);
        } catch (...) {
            network.strength = 0;
        }

        result.networks.push_back(std::move(network));
    }

    return result;
}


bool startWifiConnection(WifiConnection& connection,
                         const std::string& helperPath,
                         const WifiNetwork& network,
                         const std::string& password) {
    if (connection.pid > 0) return false;

    int inputPipe[2] = {-1, -1};
    int outputPipe[2] = {-1, -1};
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
        dup2(outputPipe[1], STDERR_FILENO);
        close(inputPipe[0]); close(inputPipe[1]);
        close(outputPipe[0]); close(outputPipe[1]);

        execl("/usr/bin/python3", "python3", helperPath.c_str(),
              "connect", network.path.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }

    close(inputPipe[0]);
    close(outputPipe[1]);

    std::string secret = password + "\n";
    size_t sent = 0;
    while (sent < secret.size()) {
        ssize_t n = write(inputPipe[1], secret.data() + sent,
                          secret.size() - sent);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        sent += static_cast<size_t>(n);
    }
    close(inputPipe[1]);
    std::fill(secret.begin(), secret.end(), '\0');

    int flags = fcntl(outputPipe[0], F_GETFL, 0);
    if (flags >= 0) fcntl(outputPipe[0], F_SETFL, flags | O_NONBLOCK);

    connection.pid = pid;
    connection.outputFd = outputPipe[0];
    connection.outputText.clear();
    return true;
}

WifiConnectionResult pollWifiConnection(WifiConnection& connection) {
    if (connection.pid <= 0) return WifiConnectionResult::Idle;

    char buffer[256];
    for (;;) {
        ssize_t n = read(connection.outputFd, buffer, sizeof(buffer));
        if (n > 0) {
            connection.outputText.append(buffer, static_cast<size_t>(n));
            if (connection.outputText.size() > 1024)
                connection.outputText.erase(
                    0, connection.outputText.size() - 1024);
        } else {
            if (n < 0 && errno == EINTR) continue;
            break;
        }
    }

    int status = 0;
    pid_t result = waitpid(connection.pid, &status, WNOHANG);
    if (result == 0) return WifiConnectionResult::Running;

    // The helper may have written its final output after the first pipe
    // drain but before waitpid observed its exit. Capture that output.
    for (;;) {
        ssize_t n = read(connection.outputFd, buffer, sizeof(buffer));
        if (n > 0) {
            connection.outputText.append(buffer, static_cast<size_t>(n));
            if (connection.outputText.size() > 1024)
                connection.outputText.erase(
                    0, connection.outputText.size() - 1024);
        } else {
            if (n < 0 && errno == EINTR) continue;
            break;
        }
    }

    close(connection.outputFd);
    connection.outputFd = -1;
    connection.pid = -1;

    const bool connected =
        result > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0 &&
        connection.outputText.find("CONNECTED") != std::string::npos;
    const bool needsPassword =
        result > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 3 &&
        connection.outputText.find("NEEDS_PASSWORD") != std::string::npos;

    if (!connected && !needsPassword) {
        std::fprintf(stderr, "Retro KiddOS: Wi-Fi connection helper failed: %s\n",
                     connection.outputText.c_str());
    }

    connection.outputText.clear();
    if (connected) return WifiConnectionResult::Connected;
    if (needsPassword) return WifiConnectionResult::NeedsPassword;
    return WifiConnectionResult::Failed;
}

void cancelWifiConnection(WifiConnection& connection) {
    if (connection.pid > 0) {
        kill(connection.pid, SIGTERM);
        int status = 0;
        while (waitpid(connection.pid, &status, 0) < 0 &&
               errno == EINTR) {}
        connection.pid = -1;
    }
    if (connection.outputFd >= 0) {
        close(connection.outputFd);
        connection.outputFd = -1;
    }
    connection.outputText.clear();
}

bool setWifiPower(bool enabled) {
    pid_t pid = fork();
    if (pid < 0) return false;

    if (pid == 0) {
        execl("/usr/bin/connmanctl", "connmanctl",
              enabled ? "enable" : "disable", "wifi",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    return waitpid(pid, &status, 0) >= 0 &&
           WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace KiddosServices
