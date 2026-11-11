#pragma once

#include <SDL.h>

namespace kiddos {

inline bool should_process_keydown(
    SDL_Keycode key, Uint8 repeat, bool text_input_active) {
    if (repeat == 0) return true;
    if (!text_input_active) return false;

    switch (key) {
        case SDLK_BACKSPACE:
        case SDLK_DELETE:
        case SDLK_HOME:
        case SDLK_END:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            return true;
        default:
            return false;
    }
}

} // namespace kiddos
