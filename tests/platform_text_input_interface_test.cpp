#include "kiddos/platform/platform.hpp"

#include <cassert>

class MinimalPlatform final : public kiddos::Platform {
public:
    bool initialize() override { return true; }
    bool poll_action(kiddos::Action&) override { return false; }
    void begin_frame() override {}
    void draw_carousel(int) override {}
    void draw_game_actions(int, bool, int) override {}
    void present() override {}
    void shutdown() override {}
};

int main() {
    MinimalPlatform platform;
    kiddos::TextInputEvent event;

    assert(!platform.poll_text_input_event(event));
    platform.set_text_input_active(true);
    platform.set_text_input_active(false);
    return 0;
}
