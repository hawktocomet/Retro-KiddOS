#pragma once

#include "kiddos/ui/element.hpp"

#include <functional>

namespace kiddos::ui {

struct SliderStyle {
    Color track{35, 45, 65, 255};
    Color fill{45, 145, 245, 255};
    Color knob{245, 248, 255, 255};
    Color focused_border{125, 190, 255, 255};
};

class Slider final : public Element {
public:
    explicit Slider(int value = 0, int minimum = 0, int maximum = 100);

    void set_value(int value);
    int value() const noexcept;

    void set_range(int minimum, int maximum);
    int minimum() const noexcept;
    int maximum() const noexcept;

    void set_step(int step) noexcept;
    int step() const noexcept;

    bool adjust(int direction);

    void set_style(SliderStyle style) noexcept;
    const SliderStyle& style() const noexcept;

    void set_on_change(std::function<void(int)> callback);

private:
    int value_ = 0;
    int minimum_ = 0;
    int maximum_ = 100;
    int step_ = 1;
    SliderStyle style_{};
    std::function<void(int)> on_change_;

    void draw(Renderer& renderer) override;
};

}
