// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "Input.hpp"
#include "LibraryNavigator.hpp"
#include "Navigation.hpp"
#include "screens/ScreenState.hpp"
#include <memory>

namespace KiddosPin { class PinWorkflow; }

class Application {
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    KiddosUI::LibraryNavigator library_{4};
    int action_ = 0;
    int menuSelected_ = 0;
    KiddosNavigation::Navigation navigation_;
    std::unique_ptr<KiddosPin::PinWorkflow> pinWorkflow_;
    int parentSelected_ = 0;
    KiddosScreens::MovieBrowserState movieBrowser_;
    bool running_ = true;
};
