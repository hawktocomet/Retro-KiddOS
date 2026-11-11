#include "kiddos/app/application.hpp"

#include "kiddos/app/home_carousel_screen.hpp"
#include "kiddos/app/ui_action_mapping.hpp"

#include <SDL_image.h>

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

    // Attach the platform-owned SDL renderer to the non-owning UI renderer
    // adapter; the UI layer never creates a second SDL renderer.
    ui_renderer_.attach(platform_.native_renderer());

    // SDL_image initialization is left to the application lifecycle; the
    // carousel artwork is BMP, which needs no extra image format flags.
    IMG_Init(IMG_INIT_PNG);

    screens_.push(std::make_unique<app::HomeCarouselScreen>(
        platform_.native_renderer()));

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
            screens_.update(0.008f);
            render();
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    // Release screens and their SDL textures before the platform shuts
    // down and destroys the native renderer they were created from.
    screens_.clear();
    IMG_Quit();

    platform_.shutdown();
    return 0;
}

void Application::handle_action(Action action) {
    switch (action) {
        case Action::Quit:
            running_ = false;
            break;

        case Action::NavigateUp:
        case Action::NavigateDown:
            if (action_screen_ && selected_game_ == 1) {
                selected_action_ =
                    (selected_action_ + 1) % 2;
            }
            break;

        case Action::Back:
            // Back is handled at the application level only; it must not
            // pop the home screen off the screen manager.
            if (action_screen_) {
                action_screen_ = false;
                selected_action_ = 0;
            }
            break;

        case Action::NavigateLeft:
        case Action::NavigateRight:
        case Action::Confirm: {
            if (action_screen_) break;

            // Translate the platform action and give the active screen a
            // chance to respond first. The home carousel is currently
            // display-only, so the application carousel state handles the
            // remaining behavior.
            const std::optional<ui::UIAction> ui_action =
                app::to_ui_action(action);
            if (!ui_action || screens_.handle_action(*ui_action)) break;

            switch (*ui_action) {
                case ui::UIAction::Left:
                    selected_game_ =
                        (selected_game_ + game_count_ - 1) % game_count_;
                    break;

                case ui::UIAction::Right:
                    selected_game_ =
                        (selected_game_ + 1) % game_count_;
                    break;

                case ui::UIAction::Confirm:
                    action_screen_ = true;
                    selected_action_ = 0;
                    break;

                case ui::UIAction::Up:
                case ui::UIAction::Down:
                case ui::UIAction::Back:
                    break;
            }
            break;
        }

        case Action::None:
            break;
    }
}

void Application::render() {
    platform_.begin_frame();

    if (action_screen_) {
        // Game 1 is a temporary checkpoint demonstration fixture; this
        // screen remains on the platform's manual rendering path.
        platform_.draw_game_actions(
            selected_game_, selected_game_ == 1, selected_action_);
    } else if (auto* home =
                   static_cast<app::HomeCarouselScreen*>(screens_.current())) {
        home->set_selected_game(selected_game_);
        screens_.render(ui_renderer_);
    }

    platform_.present();
}

} // namespace kiddos
