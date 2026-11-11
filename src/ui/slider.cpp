#include "kiddos/ui/slider.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

Slider::Slider(int value, int minimum, int maximum) {
    set_range(minimum, maximum);
    set_value(value);
    set_focusable(true);
}

void Slider::set_value(int value) {
    const int next = std::clamp(value, minimum_, maximum_);
    if (next == value_) return;

    value_ = next;
    if (on_change_) on_change_(value_);
}

int Slider::value() const noexcept {
    return value_;
}

void Slider::set_range(int minimum, int maximum) {
    if (maximum < minimum) std::swap(minimum, maximum);

    minimum_ = minimum;
    maximum_ = maximum;
    set_value(value_);
}

int Slider::minimum() const noexcept {
    return minimum_;
}

int Slider::maximum() const noexcept {
    return maximum_;
}

void Slider::set_step(int step) noexcept {
    step_ = std::max(1, step);
}

int Slider::step() const noexcept {
    return step_;
}

bool Slider::adjust(int direction) {
    if (!visible() || !enabled() || direction == 0) return false;

    const int delta = direction > 0 ? step_ : -step_;
    const long long next = static_cast<long long>(value_) + delta;
    set_value(static_cast<int>(std::clamp(
        next,
        static_cast<long long>(minimum_),
        static_cast<long long>(maximum_)
    )));
    return true;
}

void Slider::set_style(SliderStyle style) noexcept {
    style_ = style;
}

const SliderStyle& Slider::style() const noexcept {
    return style_;
}

void Slider::set_on_change(std::function<void(int)> callback) {
    on_change_ = std::move(callback);
}

void Slider::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    const int track_height = std::min(area.h, std::max(1, area.h / 5));
    const int track_y = area.y + (area.h - track_height) / 2;
    const Rect track{area.x, track_y, area.w, track_height};

    renderer.fill_rect(track, style_.track);

    const long long range =
        static_cast<long long>(maximum_) - minimum_;
    const long long offset =
        static_cast<long long>(value_) - minimum_;

    const int position = range == 0
        ? 0
        : static_cast<int>(offset * area.w / range);

    if (position > 0) {
        renderer.fill_rect(
            {area.x, track_y, position, track_height},
            style_.fill
        );
    }

    const int knob_size = std::min(
        area.w,
        std::min(area.h, std::max(6, area.h / 2))
    );
    const int knob_x = std::clamp(
        area.x + position - knob_size / 2,
        area.x,
        area.x + area.w - knob_size
    );

    renderer.fill_rect(
        {knob_x, area.y + (area.h - knob_size) / 2, knob_size, knob_size},
        style_.knob
    );

    if (focused()) {
        renderer.outline_rect(area, style_.focused_border);
    }
}

}
