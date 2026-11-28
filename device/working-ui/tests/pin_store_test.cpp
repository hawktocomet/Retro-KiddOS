// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include <sys/stat.h>

#include "../security/pin_store.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

namespace {
int failures = 0;

void check(bool condition, const char* description) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++failures;
}
}

int main() {
    char directory[] = "/tmp/kiddos-pin-test-XXXXXX";
    if (!mkdtemp(directory)) {
        std::puts("FAIL: could not create test directory");
        return 1;
    }

    bool initialized = KiddosPin::initializePinStore(directory);
    check(initialized, "Default PIN store initialization");

    bool correct = initialized &&
        KiddosPin::verifyPin(directory, "1234");
    check(correct, "Default PIN accepted");

    bool wrong = initialized &&
        !KiddosPin::verifyPin(directory, "0000");
    check(wrong, "Incorrect PIN rejected");

    bool wrongChangeRejected = initialized &&
        !KiddosPin::changePin(directory, "0000", "5678");
    check(wrongChangeRejected, "Wrong current PIN rejected for change");

    bool changed = initialized &&
        KiddosPin::changePin(directory, "1234", "5678");
    check(changed, "Correct current PIN accepted for change");

    bool reinitializePreserves = changed &&
        KiddosPin::initializePinStore(directory) &&
        KiddosPin::verifyPin(directory, "5678");
    check(reinitializePreserves, "Reinitialization preserves changed PIN");

    bool oldRejected = changed &&
        !KiddosPin::verifyPin(directory, "1234");
    check(oldRejected, "Old PIN rejected");

    bool newAccepted = changed &&
        KiddosPin::verifyPin(directory, "5678");
    check(newAccepted, "New PIN accepted");

    char path[512];
    std::snprintf(path, sizeof(path), "%s/pin.hash", directory);
    struct stat st;
    bool permissions = stat(path, &st) == 0 &&
        (st.st_mode & 0777) == 0600;
    check(permissions, "PIN file permissions 0600");

    FILE* corruptFile = std::fopen(path, "w");
    bool corruptWrite = corruptFile &&
        std::fputs("not-a-valid-hash\\n", corruptFile) >= 0;
    if (corruptFile && std::fclose(corruptFile) != 0)
        corruptWrite = false;

    bool corruptRejected = corruptWrite &&
        !KiddosPin::initializePinStore(directory);
    check(corruptRejected, "Corrupted PIN file fails closed");

    bool defaultNotRestored = corruptRejected &&
        !KiddosPin::verifyPin(directory, "1234");
    check(defaultNotRestored, "Corruption does not restore default PIN");

    unlink(path);
    rmdir(directory);

    return failures == 0 ? 0 : 1;
}
