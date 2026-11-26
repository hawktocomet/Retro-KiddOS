// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../security/pin_store.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

int main() {
    char directory[] = "/tmp/kiddos-pin-test-XXXXXX";
    if (!mkdtemp(directory)) {
        std::puts("FAIL: could not create test directory");
        return 1;
    }

    bool initialized = KiddosPin::initializePinStore(directory);
    std::printf("Default PIN store initialization: %s\n",
                initialized ? "PASS" : "FAIL");

    bool correct = initialized &&
        KiddosPin::verifyPin(directory, "1234");
    std::printf("Default PIN accepted: %s\n", correct ? "PASS" : "FAIL");

    bool wrong = initialized &&
        !KiddosPin::verifyPin(directory, "0000");
    std::printf("Incorrect PIN rejected: %s\n", wrong ? "PASS" : "FAIL");

    bool wrongChangeRejected = initialized &&
        !KiddosPin::changePin(directory, "0000", "5678");
    std::printf("Wrong current PIN rejected for change: %s\n",
                wrongChangeRejected ? "PASS" : "FAIL");

    bool changed = initialized &&
        KiddosPin::changePin(directory, "1234", "5678");
    std::printf("Correct current PIN accepted for change: %s\n",
                changed ? "PASS" : "FAIL");

    bool reinitializePreserves = changed &&
        KiddosPin::initializePinStore(directory) &&
        KiddosPin::verifyPin(directory, "5678");
    std::printf("Reinitialization preserves changed PIN: %s\n",
                reinitializePreserves ? "PASS" : "FAIL");

    bool oldRejected = changed &&
        !KiddosPin::verifyPin(directory, "1234");
    std::printf("Old PIN rejected: %s\n", oldRejected ? "PASS" : "FAIL");

    bool newAccepted = changed &&
        KiddosPin::verifyPin(directory, "5678");
    std::printf("New PIN accepted: %s\n", newAccepted ? "PASS" : "FAIL");

    char path[512];
    std::snprintf(path, sizeof(path), "%s/pin.hash", directory);
    struct stat st;
    bool permissions = stat(path, &st) == 0 &&
        (st.st_mode & 0777) == 0600;
    std::printf("PIN file permissions 0600: %s\n",
                permissions ? "PASS" : "FAIL");

    FILE* corruptFile = std::fopen(path, "w");
    bool corruptWrite = corruptFile &&
        std::fputs("not-a-valid-hash\\n", corruptFile) >= 0;
    if (corruptFile && std::fclose(corruptFile) != 0)
        corruptWrite = false;
    bool corruptRejected = corruptWrite &&
        !KiddosPin::initializePinStore(directory);
    std::printf("Corrupted PIN file fails closed: %s\n",
                corruptRejected ? "PASS" : "FAIL");
    bool defaultNotRestored = corruptRejected &&
        !KiddosPin::verifyPin(directory, "1234");
    std::printf("Corruption does not restore default PIN: %s\n",
                defaultNotRestored ? "PASS" : "FAIL");

    unlink(path);
    rmdir(directory);

    return initialized && correct && wrong && wrongChangeRejected &&
           changed && reinitializePreserves && oldRejected &&
           newAccepted && permissions ? 0 : 1;
}
