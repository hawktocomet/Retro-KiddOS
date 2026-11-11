#include "kiddos/ui/slider.hpp"
#include "kiddos/ui/controller.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Slider slider(25, 0, 100);

    assert(slider.value() == 25);
    assert(slider.minimum() == 0);
    assert(slider.maximum() == 100);
    assert(slider.step() == 1);
    assert(slider.focusable());

    int callback_count = 0;
    int callback_value = -1;
    slider.set_on_change([&](int value) {
        ++callback_count;
        callback_value = value;
    });

    slider.set_value(40);
    assert(slider.value() == 40);
    assert(callback_count == 1);
    assert(callback_value == 40);

    slider.set_step(5);
    assert(slider.adjust(1));
    assert(slider.value() == 45);
    assert(slider.adjust(-1));
    assert(slider.value() == 40);

    slider.set_value(500);
    assert(slider.value() == 100);
    slider.set_value(-500);
    assert(slider.value() == 0);

    slider.set_range(20, 80);
    assert(slider.minimum() == 20);
    assert(slider.maximum() == 80);
    assert(slider.value() == 20);

    slider.set_range(90, 10);
    assert(slider.minimum() == 10);
    assert(slider.maximum() == 90);

    slider.set_step(0);
    assert(slider.step() == 1);

    slider.set_enabled(false);
    assert(!slider.adjust(1));
    slider.set_enabled(true);
    slider.set_visible(false);
    assert(!slider.adjust(1));
    slider.set_visible(true);

    SliderStyle style;
    style.track = {1, 2, 3, 255};
    style.fill = {4, 5, 6, 255};
    slider.set_style(style);
    assert(slider.style().track.r == 1);
    assert(slider.style().fill.g == 5);

    Container root;
    auto& controlled = root.add<Slider>(50, 0, 100);
    controlled.set_step(10);
    FocusManager focus(root);
    ControllerInteraction controller(focus);

    assert(focus.focus_first());
    assert(controller.handle(UIAction::Right));
    assert(controlled.value() == 60);
    assert(controller.handle(UIAction::Left));
    assert(controlled.value() == 50);

    return 0;
}
