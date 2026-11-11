#include "kiddos/ui/focus.hpp"

#include <cassert>

class TestElement final : public kiddos::ui::Element {
public:
    TestElement() { set_focusable(true); }

private:
    void draw(kiddos::ui::Renderer&) override {}
};

int main() {
    using namespace kiddos::ui;

    Container root;
    auto& left = root.add<TestElement>();
    auto& right = root.add<TestElement>();
    auto& below = root.add<TestElement>();
    auto& hidden = root.add<TestElement>();
    auto& disabled = root.add<TestElement>();

    left.set_bounds({0, 0, 100, 100});
    right.set_bounds({200, 0, 100, 100});
    below.set_bounds({0, 200, 100, 100});
    hidden.set_bounds({400, 0, 100, 100});
    disabled.set_bounds({600, 0, 100, 100});

    hidden.set_visible(false);
    disabled.set_enabled(false);

    FocusManager focus(root);

    assert(focus.focused() == nullptr);
    assert(focus.focus_first());
    assert(focus.focused() == &left);
    assert(left.focused());

    assert(focus.move(Direction::Right));
    assert(focus.focused() == &right);
    assert(!left.focused());
    assert(right.focused());

    assert(!focus.move(Direction::Right));
    assert(focus.focused() == &right);

    assert(focus.move(Direction::Down));
    assert(focus.focused() == &below);

    assert(focus.move(Direction::Up));
    assert(focus.focused() == &left);

    focus.clear();
    assert(focus.focused() == nullptr);
    assert(!left.focused());

    Container empty;
    FocusManager empty_focus(empty);
    assert(!empty_focus.focus_first());
    assert(!empty_focus.move(Direction::Right));

    return 0;
}
