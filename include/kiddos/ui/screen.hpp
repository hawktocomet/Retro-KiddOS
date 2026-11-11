#pragma once
#include "kiddos/ui/container.hpp"
#include "kiddos/ui/controller.hpp"
#include "kiddos/ui/text_input_interaction.hpp"
#include "kiddos/platform/text_input_event.hpp"

namespace kiddos::ui {
class Screen {
public:
    Screen() noexcept;
    virtual ~Screen() = default;
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    Container& root() noexcept;
    FocusManager& focus() noexcept;
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void on_suspend() {}
    virtual void on_resume() {}
    virtual void update(float) {}
    virtual void render(Renderer& renderer);
    bool handle_action(UIAction action);
    bool handle_text_input(const ::kiddos::TextInputEvent& event);
private:
    Container root_;
    FocusManager focus_;
    ControllerInteraction controller_;
    TextInputInteraction text_input_;
};
}
