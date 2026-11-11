#include "kiddos/ui/label.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Label label("Game title");

    assert(label.text() == "Game title");
    assert(label.font() == nullptr);
    assert(label.alignment() == LabelAlignment::left);
    assert(label.color().r == 255);
    assert(label.color().g == 255);
    assert(label.color().b == 255);
    assert(label.color().a == 255);

    label.set_text("New title");
    assert(label.text() == "New title");

    label.set_alignment(LabelAlignment::center);
    assert(label.alignment() == LabelAlignment::center);

    label.set_alignment(LabelAlignment::right);
    assert(label.alignment() == LabelAlignment::right);

    const Color custom{10, 20, 30, 200};
    label.set_color(custom);

    assert(label.color().r == 10);
    assert(label.color().g == 20);
    assert(label.color().b == 30);
    assert(label.color().a == 200);

    label.set_bounds({0, 0, 200, 40});
    assert(label.bounds().w == 200);

    label.set_text("");
    assert(label.text().empty());

    label.set_font(nullptr);
    assert(label.font() == nullptr);

    return 0;
}
