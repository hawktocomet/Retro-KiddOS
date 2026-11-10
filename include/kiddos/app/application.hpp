#pragma once

#include "kiddos/platform/platform.hpp"

namespace kiddos {

class Application {
public:
    explicit Application(Platform& platform);

    int run();

private:
    Platform& platform_;
    bool running_ = true;
    int selected_game_ = 0;
    bool action_screen_ = false;
    int selected_action_ = 0;
    static constexpr int game_count_ = 3;

    void handle_action(Action action);
    void render();
};

} // namespace kiddos
