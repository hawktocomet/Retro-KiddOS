#pragma once

#include "kiddos/ui/container.hpp"

namespace kiddos::ui {

enum class Direction {
    Left,
    Right,
    Up,
    Down
};

class FocusManager final {
public:
    explicit FocusManager(Container& root) noexcept;

    Element* focused() const noexcept;
    bool focus_first() noexcept;
    bool move(Direction direction) noexcept;
    void clear() noexcept;

private:
    Container& root_;
    Element* focused_ = nullptr;

    bool eligible(const Element* element) const noexcept;
};

} // namespace kiddos::ui
