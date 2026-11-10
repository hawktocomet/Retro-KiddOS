#include "kiddos/app/application.hpp"

#include <chrono>
#include <thread>

namespace kiddos {

Application::Application(Platform& platform)
    : platform_(platform) {
}

int Application::run() {
    if (!platform_.initialize()) {
        platform_.shutdown();
        return 1;
    }

    running_ = true;

    while (running_) {
        Action action = Action::None;

        while (platform_.poll_action(action)) {
            handle_action(action);

            if (!running_) {
                break;
            }
        }

        if (running_) {
            render();
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    platform_.shutdown();
    return 0;
}

void Application::handle_action(Action action) {
    switch (action) {
        case Action::Quit:
            running_ = false;
            break;

        case Action::NavigateLeft:
            selected_game_ =
                (selected_game_ + game_count_ - 1) % game_count_;
            break;

        case Action::NavigateRight:
            selected_game_ =
                (selected_game_ + 1) % game_count_;
            break;

        case Action::None:
        case Action::NavigateUp:
        case Action::NavigateDown:
        case Action::Confirm:
        case Action::Back:
            break;
    }
}

void Application::render() {
    platform_.begin_frame();
    platform_.draw_carousel(selected_game_);
    platform_.present();
}

} // namespace kiddos
