#pragma once

#include "kiddos/core/action.hpp"
#include "kiddos/platform/text_input_event.hpp"

struct SDL_Renderer;

namespace kiddos {

class Platform {
public:
    virtual ~Platform() = default;

    // Non-owning access to the platform's native SDL renderer, if one
    // exists. The platform retains ownership; callers must not destroy it.
    virtual SDL_Renderer* native_renderer() const noexcept { return nullptr; }

    virtual bool initialize() = 0;
    virtual bool poll_action(Action& action) = 0;
    virtual bool poll_text_input_event(TextInputEvent&) { return false; }
    virtual void set_text_input_active(bool) {}
    virtual void begin_frame() = 0;
    virtual void draw_carousel(int selected_index) = 0;
    virtual void draw_game_actions(int selected_index, bool has_checkpoint, int selected_action) = 0;
    virtual void present() = 0;
    virtual void shutdown() = 0;
};

} // namespace kiddos
