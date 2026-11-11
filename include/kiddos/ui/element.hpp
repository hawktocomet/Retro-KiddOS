#pragma once

#include "kiddos/ui/geometry.hpp"
#include "kiddos/ui/renderer.hpp"

namespace kiddos::ui {

class Element {
public:
    virtual ~Element() = default;

    void set_bounds(Rect bounds) noexcept { bounds_ = bounds; }
    Rect bounds() const noexcept { return bounds_; }

    void set_visible(bool visible) noexcept { visible_ = visible; }
    bool visible() const noexcept { return visible_; }

    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    bool enabled() const noexcept { return enabled_; }

    void set_focusable(bool focusable) noexcept {
        focusable_ = focusable;
    }

    bool focusable() const noexcept { return focusable_; }

    void render(Renderer& renderer) {
        if (visible_) draw(renderer);
    }

protected:
    virtual void draw(Renderer& renderer) = 0;

private:
    Rect bounds_{};
    bool visible_ = true;
    bool enabled_ = true;
    bool focusable_ = false;
};

} // namespace kiddos::ui
