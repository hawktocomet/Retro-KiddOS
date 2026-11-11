#include "kiddos/ui/progress_bar.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    ProgressBar bar;

    assert(bar.value() == 0);
    assert(bar.orientation() == ProgressOrientation::horizontal);
    assert(bar.style().border_thickness == 1);

    bar.set_value(65);
    assert(bar.value() == 65);

    bar.set_value(-10);
    assert(bar.value() == 0);

    bar.set_value(150);
    assert(bar.value() == 100);

    bar.set_orientation(ProgressOrientation::vertical);
    assert(bar.orientation() == ProgressOrientation::vertical);

    ProgressBarStyle style;
    style.background = {1, 2, 3, 255};
    style.fill = {4, 5, 6, 255};
    style.border = {7, 8, 9, 255};
    style.border_thickness = 3;

    bar.set_style(style);

    assert(bar.style().background.r == 1);
    assert(bar.style().fill.g == 5);
    assert(bar.style().border.b == 9);
    assert(bar.style().border_thickness == 3);

    style.border_thickness = -4;
    bar.set_style(style);
    assert(bar.style().border_thickness == 0);

    ProgressBar initial(42);
    assert(initial.value() == 42);

    initial.set_bounds({0, 0, 200, 20});
    assert(initial.bounds().w == 200);

    return 0;
}
