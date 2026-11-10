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
            if (!action_screen_) {
                selected_game_ =
                    (selected_game_ + game_count_ - 1) % game_count_;
            }
            break;

        case Action::NavigateRight:
            if (!action_screen_) {
                selected_game_ =
                    (selected_game_ + 1) % game_count_;
            }
            break;

        case Action::NavigateUp:
            if (action_screen_ && selected_game_ == 1) {
                selected_action_ =
                    (selected_action_ + 1) % 2;
            }
            break;

        case Action::NavigateDown:
            if (action_screen_ && selected_game_ == 1) {
                selected_action_ =
                    (selected_action_ + 1) % 2;
            }
            break;

        case Action::Confirm:
            if (!action_screen_) {
                action_screen_ = true;
                selected_action_ = 0;
            }
            // Game launching is intentionally not implemented yet.
            break;

        case Action::Back:
            if (action_screen_) {
                action_screen_ = false;
                selected_action_ = 0;
            }
            break;

        case Action::None:
            break;
    }
}

void Application::render() {
    platform_.begin_frame();
    if (action_screen_) {
        // Game 1 is a temporary checkpoint demonstration fixture.
        platform_.draw_game_actions(
            selected_game_, selected_game_ == 1, selected_action_);
    } else {
        platform_.draw_carousel(selected_game_);
    }
    platform_.present();
}

} // namespace kiddos
