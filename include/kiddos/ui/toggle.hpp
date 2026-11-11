#pragma once

#include "kiddos/ui/element.hpp"

#include <functional>

namespace kiddos::ui {

struct ToggleStyle {
    Color off{45, 50, 65, 255};
    Color on{35, 125, 220, 255};
    Color knob{245, 248, 255, 255};
    Color focused_border{125, 190, 255, 255};
};

class Toggle final : public Element {
public:
    explicit Toggle(bool checked = false);

    void set_checked(bool checked);
    bool checked() const noexcept;
    bool toggle();

    void set_style(ToggleStyle style) noexcept;
    const ToggleStyle& style() const noexcept;

    void set_on_change(std::function<void(bool)> callback);

private:
    bool checked_ = false;
    ToggleStyle style_{};
    std::function<void(bool)> on_change_;

    void draw(Renderer& renderer) override;
};

}
