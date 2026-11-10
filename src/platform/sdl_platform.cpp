#include "kiddos/platform/sdl_platform.hpp"

#include <iostream>

namespace kiddos {

SDLPlatform::~SDLPlatform() {
    shutdown();
}

bool SDLPlatform::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL initialization failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    initialized_ = true;

    window_ = SDL_CreateWindow(
        "KiddOS",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        720,
        720,
        SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "Window creation failed: "
                  << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    renderer_ = SDL_CreateRenderer(
        window_,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(
            window_,
            -1,
            SDL_RENDERER_SOFTWARE
        );
    }

    if (!renderer_) {
        std::cerr << "Renderer creation failed: "
                  << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        if (SDL_IsGameController(index)) {
            open_controller(index);
            if (controller_) {
                break;
            }
        }
    }

    return true;
}

void SDLPlatform::open_controller(int device_index) {
    if (controller_) {
        return;
    }

    controller_ = SDL_GameControllerOpen(device_index);

    if (controller_) {
        controller_instance_ = SDL_JoystickInstanceID(
            SDL_GameControllerGetJoystick(controller_)
        );
    }
}

void SDLPlatform::close_controller() {
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }

    controller_instance_ = -1;
}

Action SDLPlatform::map_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT:  return Action::NavigateLeft;
        case SDLK_RIGHT: return Action::NavigateRight;
        case SDLK_UP:    return Action::NavigateUp;
        case SDLK_DOWN:  return Action::NavigateDown;
        case SDLK_RETURN:
        case SDLK_SPACE: return Action::Confirm;
        case SDLK_ESCAPE:return Action::Back;
        case SDLK_q:     return Action::Quit;
        default:         return Action::None;
    }
}

Action SDLPlatform::map_controller_button(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return Action::NavigateLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return Action::NavigateRight;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return Action::NavigateUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return Action::NavigateDown;
        case SDL_CONTROLLER_BUTTON_A:
            return Action::Confirm;
        case SDL_CONTROLLER_BUTTON_B:
            return Action::Back;
        default:
            return Action::None;
    }
}

bool SDLPlatform::poll_action(Action& action) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        action = Action::None;

        switch (event.type) {
            case SDL_QUIT:
                action = Action::Quit;
                return true;

            case SDL_KEYDOWN:
                if (event.key.repeat == 0) {
                    action = map_key(event.key.keysym.sym);
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                if (event.cbutton.which == controller_instance_) {
                    action = map_controller_button(event.cbutton.button);
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                open_controller(event.cdevice.which);
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (event.cdevice.which == controller_instance_) {
                    close_controller();
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    action = Action::Quit;
                    return true;
                }
                break;

            default:
                break;
        }

        if (action != Action::None) {
            return true;
        }
    }

    return false;
}

void SDLPlatform::begin_frame() {
    if (!renderer_) {
        return;
    }

    SDL_SetRenderDrawColor(renderer_, 9, 17, 38, 255);
    SDL_RenderClear(renderer_);
}

void SDLPlatform::present() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

void SDLPlatform::shutdown() {
    close_controller();

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (initialized_) {
        SDL_Quit();
        initialized_ = false;
    }
}

} // namespace kiddos
