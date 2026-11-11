#pragma once

#include "kiddos/platform/text_input_event.hpp"
#include "kiddos/platform/sdl_keyboard_policy.hpp"

#include <SDL.h>

#include <optional>

namespace kiddos {

class SDLTextInputTranslator {
public:
    static std::optional<TextInputEvent> translate(
        const SDL_Event& event, bool active) {
        if (!active) return std::nullopt;

        if (event.type == SDL_TEXTINPUT) {
            return TextInputEvent{TextInputEventType::Text, event.text.text};
        }

        if (event.type != SDL_KEYDOWN ||
            !should_process_keydown(event.key.keysym.sym,
                                    event.key.repeat, true)) {
            return std::nullopt;
        }

        switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE: return TextInputEvent{TextInputEventType::Backspace, {}};
            case SDLK_DELETE: return TextInputEvent{TextInputEventType::Delete, {}};
            case SDLK_LEFT: return TextInputEvent{TextInputEventType::Left, {}};
            case SDLK_RIGHT: return TextInputEvent{TextInputEventType::Right, {}};
            case SDLK_HOME: return TextInputEvent{TextInputEventType::Home, {}};
            case SDLK_END: return TextInputEvent{TextInputEventType::End, {}};
            default: return std::nullopt;
        }
    }
};

} // namespace kiddos
