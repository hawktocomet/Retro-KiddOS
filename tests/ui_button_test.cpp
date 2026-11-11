#include "kiddos/ui/button.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Button button("Play");

    assert(button.label() == "Play");
    assert(button.focusable());
    assert(!button.pressed());
    assert(button.font() == nullptr);

    int activations = 0;
    button.set_on_activate([&activations]() { ++activations; });

    assert(button.activate());
    assert(activations == 1);

    button.set_enabled(false);
    assert(!button.activate());
    assert(activations == 1);

    button.set_enabled(true);
    button.set_visible(false);
    assert(!button.activate());
    assert(activations == 1);

    button.set_visible(true);
    button.set_pressed(true);
    assert(button.pressed());

    button.set_label("Settings");
    assert(button.label() == "Settings");

    button.set_on_activate({});
    assert(!button.activate());
    assert(activations == 1);

    ButtonStyle style;
    style.text = {240, 240, 240, 255};
    button.set_style(style);
    assert(button.style().text.r == 240);

    return 0;
}
