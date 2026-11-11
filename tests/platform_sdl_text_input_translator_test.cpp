#include "kiddos/platform/sdl_text_input_translator.hpp"

#include <cassert>
#include <cstring>

int main() {
    using kiddos::SDLTextInputTranslator;
    using kiddos::TextInputEventType;

    SDL_Event event{};

    event.type = SDL_TEXTINPUT;
    std::strcpy(event.text.text, "hello");
    auto translated = SDLTextInputTranslator::translate(event, true);
    assert(translated);
    assert(translated->type == TextInputEventType::Text);
    assert(translated->text == "hello");
    assert(!SDLTextInputTranslator::translate(event, false));

    const SDL_Keycode keys[] = {
        SDLK_BACKSPACE, SDLK_DELETE, SDLK_LEFT,
        SDLK_RIGHT, SDLK_HOME, SDLK_END
    };
    const TextInputEventType types[] = {
        TextInputEventType::Backspace, TextInputEventType::Delete,
        TextInputEventType::Left, TextInputEventType::Right,
        TextInputEventType::Home, TextInputEventType::End
    };

    for (int i = 0; i < 6; ++i) {
        event = {};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = keys[i];
        translated = SDLTextInputTranslator::translate(event, true);
        assert(translated);
        assert(translated->type == types[i]);
        assert(!SDLTextInputTranslator::translate(event, false));
    }

    event = {};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_BACKSPACE;
    event.key.repeat = 1;
    assert(SDLTextInputTranslator::translate(event, true));

    event.key.keysym.sym = SDLK_UP;
    assert(!SDLTextInputTranslator::translate(event, true));

    event.type = SDL_KEYUP;
    event.key.keysym.sym = SDLK_LEFT;
    assert(!SDLTextInputTranslator::translate(event, true));

    return 0;
}
