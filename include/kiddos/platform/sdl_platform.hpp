#pragma once

#include "kiddos/platform/platform.hpp"
#include "kiddos/platform/text_input_event.hpp"

#include <SDL.h>

#include <deque>

namespace kiddos {

class SDLPlatform final : public Platform {
public:
    SDLPlatform() = default;
    ~SDLPlatform() override;

    bool initialize() override;
    bool poll_action(Action& action) override;
    bool poll_text_input_event(TextInputEvent& event) override;
    void set_text_input_active(bool active) override;
    void begin_frame() override;
    void draw_carousel(int selected_index) override;
    void draw_game_actions(int selected_index, bool has_checkpoint, int selected_action) override;
    void present() override;
    void shutdown() override;

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_GameController* controller_ = nullptr;
    SDL_JoystickID controller_instance_ = -1;
    bool initialized_ = false;
    bool text_input_active_ = false;
    std::deque<Action> pending_actions_;
    std::deque<TextInputEvent> pending_text_events_;
    bool pump_events();

    void open_controller(int device_index);
    void close_controller();
    static Action map_key(SDL_Keycode key);
    static Action map_controller_button(Uint8 button);
};

} // namespace kiddos
