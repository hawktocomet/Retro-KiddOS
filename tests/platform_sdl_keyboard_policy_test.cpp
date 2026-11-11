#include "kiddos/platform/sdl_keyboard_policy.hpp"

#include <cassert>

int main() {
    using kiddos::should_process_keydown;

    assert(should_process_keydown(SDLK_a, 0, false));
    assert(!should_process_keydown(SDLK_a, 1, false));
    assert(!should_process_keydown(SDLK_a, 1, true));

    assert(should_process_keydown(SDLK_BACKSPACE, 0, true));
    assert(should_process_keydown(SDLK_BACKSPACE, 1, true));
    assert(!should_process_keydown(SDLK_BACKSPACE, 1, false));

    assert(should_process_keydown(SDLK_DELETE, 1, true));
    assert(should_process_keydown(SDLK_HOME, 1, true));
    assert(should_process_keydown(SDLK_END, 1, true));
    assert(should_process_keydown(SDLK_LEFT, 1, true));
    assert(should_process_keydown(SDLK_RIGHT, 1, true));

    assert(!should_process_keydown(SDLK_UP, 1, true));
    assert(!should_process_keydown(SDLK_DOWN, 1, true));

    return 0;
}
