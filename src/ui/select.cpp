#include "kiddos/ui/select.hpp"
#include "kiddos/ui/text.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

Select::Select(std::vector<std::string> options, int selected) {
    set_focusable(true);
    set_options(std::move(options));
    if (!options_.empty()) set_selected(selected);
}

void Select::set_options(std::vector<std::string> options) {
    options_ = std::move(options);
    selected_ = options_.empty() ? -1 : 0;
}

const std::vector<std::string>& Select::options() const noexcept {
    return options_;
}

bool Select::set_selected(int index) {
    if (!visible() || !enabled() || index < 0 ||
        index >= static_cast<int>(options_.size())) return false;
    if (selected_ == index) return true;

    selected_ = index;
    if (on_change_) on_change_(selected_);
    return true;
}

int Select::selected() const noexcept {
    return selected_;
}

const std::string* Select::selected_option() const noexcept {
    if (selected_ < 0 || selected_ >= static_cast<int>(options_.size()))
        return nullptr;
    return &options_[selected_];
}

bool Select::select_next() {
    if (options_.empty() || !visible() || !enabled()) return false;
    return set_selected((selected_ + 1) % static_cast<int>(options_.size()));
}

bool Select::select_previous() {
    if (options_.empty() || !visible() || !enabled()) return false;
    const int count = static_cast<int>(options_.size());
    return set_selected((selected_ - 1 + count) % count);
}

void Select::set_font(const Font* font) noexcept {
    font_ = font;
}

const Font* Select::font() const noexcept {
    return font_;
}

void Select::set_style(SelectStyle style) noexcept {
    style_ = style;
}

const SelectStyle& Select::style() const noexcept {
    return style_;
}

void Select::set_on_change(std::function<void(int)> callback) {
    on_change_ = std::move(callback);
}

void Select::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    renderer.fill_rect(area, style_.background);
    renderer.outline_rect(
        area, focused() ? style_.focused_border : style_.border);

    const Color foreground = enabled() ? style_.text : style_.disabled_text;
    const int mid_y = area.y + area.h / 2;
    const int arrow_size = std::max(2, std::min(7, area.h / 5));
    const int left_x = area.x + std::max(5, area.h / 3);
    const int right_x = area.x + area.w - std::max(5, area.h / 3);

    renderer.draw_line(
        {left_x + arrow_size, mid_y - arrow_size},
        {left_x, mid_y}, style_.arrow);
    renderer.draw_line(
        {left_x, mid_y},
        {left_x + arrow_size, mid_y + arrow_size}, style_.arrow);

    renderer.draw_line(
        {right_x - arrow_size, mid_y - arrow_size},
        {right_x, mid_y}, style_.arrow);
    renderer.draw_line(
        {right_x, mid_y},
        {right_x - arrow_size, mid_y + arrow_size}, style_.arrow);

    if (font_ && font_->valid()) {
        const Rect text_area{
            area.x + arrow_size + 12,
            area.y,
            area.w - (arrow_size * 2 + 24),
            area.h
        };
        if (!text_area.empty() && selected_option()) {
            TextRenderer text(renderer);
            text.draw_centered(*selected_option(), *font_, text_area,
                               foreground);
        }
    }
}

}
