// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

namespace KiddosNavigation {

enum Screen {
    Games,
    SelectedGame,
    GameInfo,
    SystemMenu,
    PinEntry,
    ParentCenter,
    WifiSettings,
    WifiPassword,
    ConfirmShutdown
};

// Owns the active destination and the screen to return to after the system menu.
class Navigation {
public:
    Screen current() const { return current_; }
    Screen returnScreen() const { return returnScreen_; }
    Screen& currentRef() { return current_; }
    Screen& returnScreenRef() { return returnScreen_; }

    void show(Screen screen) { current_ = screen; }

    void openSystemMenu() {
        returnScreen_ = current_;
        current_ = Screen::SystemMenu;
    }

    void closeSystemMenu() { current_ = returnScreen_; }

private:
    Screen current_ = Screen::Games;
    Screen returnScreen_ = Screen::Games;
};

} // namespace KiddosNavigation
