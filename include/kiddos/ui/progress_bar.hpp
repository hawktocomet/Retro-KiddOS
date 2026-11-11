#pragma once

#include "kiddos/ui/element.hpp"

namespace kiddos::ui {

enum class ProgressOrientation {
    horizontal,
    vertical
};

struct ProgressBarStyle {
    Color background{15, 25, 45, 255};
    Color fill{45, 145, 245, 255};
    Color border{65, 100, 150, 255};
    int border_thickness = 1;
};

class ProgressBar final : public Element {
public:
    explicit ProgressBar(int value = 0);

    void set_value(int value) noexcept;
    int value() const noexcept;

    void set_orientation(ProgressOrientation orientation) noexcept;
    ProgressOrientation orientation() const noexcept;

    void set_style(ProgressBarStyle style) noexcept;
    const ProgressBarStyle& style() const noexcept;

private:
    int value_ = 0;
    ProgressOrientation orientation_ = ProgressOrientation::horizontal;
    ProgressBarStyle style_{};

    void draw(Renderer& renderer) override;
};

}
