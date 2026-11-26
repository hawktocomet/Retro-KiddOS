// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "pin_store.h"

#include <algorithm>
#include <cstring>

namespace KiddosPin {

enum class WorkflowResult {
    Pending,
    Unlocked,
    Changed
};

// Owns the complete parent-PIN interaction, independently of SDL and screens.
class PinWorkflow {
public:
    explicit PinWorkflow(const char* storeDirectory)
        : storeDirectory_(storeDirectory ? storeDirectory : "") {
        resetDigits();
    }

    void beginUnlock() {
        task_ = Task::Unlock;
        clearSecrets();
        resetDigits();
        prompt_ = "ENTER PARENT PIN";
        feedback_ = "INCORRECT PIN - TRY AGAIN";
    }

    void beginChange() {
        task_ = Task::Current;
        clearSecrets();
        resetDigits();
        prompt_ = "ENTER CURRENT PIN";
        feedback_ = "INCORRECT PIN - TRY AGAIN";
    }

    void cancelChange() {
        task_ = Task::Unlock;
        clearSecrets();
        resetDigits();
        prompt_ = "ENTER PARENT PIN";
        feedback_ = "INCORRECT PIN - TRY AGAIN";
    }

    bool isChanging() const { return task_ != Task::Unlock; }

    void adjustDigit(int delta) {
        const int value = digits_[position_] - '0';
        digits_[position_] = static_cast<char>('0' + (value + delta + 10) % 10);
    }

    void moveCursor(int delta) {
        position_ = (position_ + delta + 4) % 4;
    }

    void setDigit(int value) {
        if (value < 0 || value > 9) return;
        digits_[position_] = static_cast<char>('0' + value);
        if (position_ < 3) ++position_;
        error_ = false;
    }

    WorkflowResult submit() {
        if (task_ == Task::Unlock) {
            if (verifyPin(storeDirectory_.c_str(), digits_)) {
                error_ = false;
                return WorkflowResult::Unlocked;
            }
            resetDigits();
            error_ = true;
            return WorkflowResult::Pending;
        }

        if (task_ == Task::Current) {
            if (!verifyPin(storeDirectory_.c_str(), digits_)) {
                resetDigits();
                error_ = true;
                return WorkflowResult::Pending;
            }
            std::memcpy(currentPin_, digits_, sizeof(currentPin_));
            task_ = Task::New;
            resetDigits();
            prompt_ = "ENTER NEW PIN";
            feedback_ = " ";
            return WorkflowResult::Pending;
        }

        if (task_ == Task::New) {
            std::memcpy(newPin_, digits_, sizeof(newPin_));
            task_ = Task::Confirm;
            resetDigits();
            prompt_ = "CONFIRM NEW PIN";
            feedback_ = " ";
            return WorkflowResult::Pending;
        }

        if (std::strcmp(newPin_, digits_) != 0) {
            task_ = Task::New;
            resetDigits();
            prompt_ = "ENTER NEW PIN";
            feedback_ = "PINS DID NOT MATCH - TRY AGAIN";
            error_ = true;
            return WorkflowResult::Pending;
        }

        if (!changePin(storeDirectory_.c_str(), currentPin_, newPin_)) {
            task_ = Task::New;
            resetDigits();
            prompt_ = "ENTER NEW PIN";
            feedback_ = "COULD NOT SAVE PIN - TRY AGAIN";
            error_ = true;
            return WorkflowResult::Pending;
        }

        task_ = Task::Unlock;
        clearSecrets();
        resetDigits();
        prompt_ = "ENTER PARENT PIN";
        feedback_ = "INCORRECT PIN - TRY AGAIN";
        return WorkflowResult::Changed;
    }

    const char* digits() const { return digits_; }
    int position() const { return position_; }
    bool hasError() const { return error_; }
    const char* prompt() const { return prompt_; }
    const char* feedback() const { return feedback_; }

private:
    enum class Task { Unlock, Current, New, Confirm };

    void resetDigits() {
        std::fill(digits_, digits_ + 4, '0');
        digits_[4] = '\0';
        position_ = 0;
        error_ = false;
    }

    void clearSecrets() {
        std::fill(currentPin_, currentPin_ + sizeof(currentPin_), '\0');
        std::fill(newPin_, newPin_ + sizeof(newPin_), '\0');
    }

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
