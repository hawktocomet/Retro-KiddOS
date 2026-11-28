// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <string>

namespace KiddosPin {

enum class WorkflowResult {
    Pending,
    Unlocked,
    Changed
};

// Owns the complete parent-PIN interaction, independently of SDL and screens.
class PinWorkflow {
public:
    explicit PinWorkflow(const char* storeDirectory);

    void beginUnlock();
    void beginChange();
    void cancelChange();

    bool isChanging() const;

    void adjustDigit(int delta);
    void moveCursor(int delta);
    void setDigit(int value);

    WorkflowResult submit();

    const char* digits() const;
    int position() const;
    bool hasError() const;
    const char* prompt() const;
    const char* feedback() const;

private:
    enum class Task { Unlock, Current, New, Confirm };

    void resetDigits();
    void clearSecrets();

    std::string storeDirectory_;
    Task task_ = Task::Unlock;
    char digits_[5] = "0000";
    char currentPin_[5] = "";
    char newPin_[5] = "";
    int position_ = 0;
    bool error_ = false;
    const char* prompt_ = "ENTER PARENT PIN";
    const char* feedback_ = "INCORRECT PIN - TRY AGAIN";
};

} // namespace KiddosPin
