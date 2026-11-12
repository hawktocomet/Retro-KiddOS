#pragma once

#include "kiddos/app/platform.hpp"
#include "kiddos/platform/platform.hpp"
#include "kiddos/ui/renderer.hpp"
#include "kiddos/ui/screen_manager.hpp"

namespace kiddos {

class Application {
public:
    explicit Application(Platform& platform);

    int run();

private:
    Platform& platform_;
    ui::ScreenManager screens_;
    ui::Renderer ui_renderer_;
    bool running_ = true;
    int selected_game_ = 1;
    bool action_screen_ = false;
    int selected_action_ = 0;
    static constexpr int game_count_ = static_cast<int>(app::platform_count);

    void handle_action(Action action);
    void render();
};

} // namespace kiddos
