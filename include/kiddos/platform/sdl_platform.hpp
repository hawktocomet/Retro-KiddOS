#pragma once

#include "kiddos/platform/platform.hpp"

#include <SDL.h>

namespace kiddos {

class SDLPlatform final : public Platform {
public:
    SDLPlatform() = default;
    ~SDLPlatform() override;

    bool initialize() override;
    bool poll_action(Action& action) override;
    void begin_frame() override;
    void present() override;
    void shutdown() override;

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_GameController* controller_ = nullptr;
    SDL_JoystickID controller_instance_ = -1;
    bool initialized_ = false;

    void open_controller(int device_index);
    void close_controller();
    static Action map_key(SDL_Keycode key);
    static Action map_controller_button(Uint8 button);
};

} // namespace kiddos
