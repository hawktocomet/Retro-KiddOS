#include "kiddos/ui/panel.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Panel panel;

    assert(panel.style().background.r == 20);
    assert(panel.style().background.g == 30);
    assert(panel.style().background.b == 55);
    assert(panel.style().border_thickness == 2);

    const Color background{10, 20, 30, 255};
    const Color border{100, 150, 200, 255};

    panel.set_background(background);
    panel.set_border(border);
    panel.set_border_thickness(4);

    assert(panel.style().background.r == 10);
    assert(panel.style().border.r == 100);
    assert(panel.style().border_thickness == 4);

    panel.set_border_thickness(-5);
    assert(panel.style().border_thickness == 0);

    PanelStyle custom;
    custom.background = {1, 2, 3, 255};
    custom.border = {4, 5, 6, 255};
    custom.border_thickness = 3;

    panel.set_style(custom);

    assert(panel.style().background.r == 1);
    assert(panel.style().border.g == 5);
    assert(panel.style().border_thickness == 3);

    custom.border_thickness = -1;
    Panel second(custom);
    assert(second.style().border_thickness == 0);

    panel.set_bounds({0, 0, 200, 100});
    assert(panel.bounds().w == 200);

    return 0;
}
