#include "kiddos/ui/geometry.hpp"

#include <cassert>

int main() {
    using kiddos::ui::Point;
    using kiddos::ui::Rect;

    const Rect card{100, 100, 200, 150};

    assert(!card.empty());
    assert(card.contains(Point{100, 100}));
    assert(card.contains(Point{299, 249}));
    assert(!card.contains(Point{300, 249}));
    assert(!card.contains(Point{100, 250}));
    assert(!card.contains(Point{99, 100}));

    const Rect empty{0, 0, 0, 20};
    assert(empty.empty());
    assert(!empty.contains(Point{0, 0}));

    return 0;
}
