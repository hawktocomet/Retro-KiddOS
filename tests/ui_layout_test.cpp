#include "kiddos/ui/layout.hpp"

#include <cassert>

class TestElement final : public kiddos::ui::Element {
private:
    void draw(kiddos::ui::Renderer&) override {}
};

int main() {
    using namespace kiddos::ui;

    Container row;

    auto& first = row.add<TestElement>();
    auto& second = row.add<TestElement>();
    auto& third = row.add<TestElement>();

    StackLayout::apply(
        row,
        {0, 0, 720, 200},
        Axis::Horizontal,
        10,
        {10, 20, 10, 20}
    );

    assert(first.bounds().x == 10);
    assert(first.bounds().y == 20);
    assert(first.bounds().w == 227);
    assert(first.bounds().h == 160);

    assert(second.bounds().x == 247);
    assert(second.bounds().w == 226);

    assert(third.bounds().x == 483);
    assert(third.bounds().w == 227);

    second.set_visible(false);

    StackLayout::apply(
        row,
        {0, 0, 600, 100},
        Axis::Vertical,
        10
    );

    assert(first.bounds().h == 45);
    assert(third.bounds().y == 55);
    assert(third.bounds().h == 45);

    assert(row.child_count() == 3);
    assert(row.child(3) == nullptr);

    Container empty;
    StackLayout::apply(empty, {0, 0, 100, 100}, Axis::Vertical);
    assert(empty.child_count() == 0);

    return 0;
}
