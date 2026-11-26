// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "Application.hpp"

#include <cstdio>
#include <cstring>

#ifndef KIDDOS_VERSION
#define KIDDOS_VERSION "unknown"
#endif

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("KiddOS %s\n", KIDDOS_VERSION);
        return 0;
    }

    Application application;
    return application.run();
}
