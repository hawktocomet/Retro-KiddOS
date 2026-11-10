#include "kiddos/app/application.hpp"

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

        case Action::None:
        case Action::NavigateLeft:
        case Action::NavigateRight:
        case Action::NavigateUp:
        case Action::NavigateDown:
        case Action::Confirm:
        case Action::Back:
            break;
    }
}

void Application::render() {
    platform_.begin_frame();
    platform_.present();
}

} // namespace kiddos
