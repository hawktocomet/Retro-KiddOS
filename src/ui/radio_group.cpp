#include "kiddos/ui/radio_group.hpp"
#include "kiddos/ui/text.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

RadioGroup::RadioGroup(std::vector<std::string> options, int selected) {
    set_focusable(true);
    set_options(std::move(options));
    if (!options_.empty()) set_selected(selected);
}

void RadioGroup::set_options(std::vector<std::string> options) {
    options_ = std::move(options);
    selected_ = options_.empty() ? -1 : 0;
}

const std::vector<std::string>& RadioGroup::options() const noexcept {
    return options_;
}

bool RadioGroup::set_selected(int index) {
    if (!visible() || !enabled() || index < 0 ||
        index >= static_cast<int>(options_.size())) return false;
    if (selected_ == index) return true;
    selected_ = index;
    if (on_change_) on_change_(selected_);
    return true;
}

int RadioGroup::selected() const noexcept {
    return selected_;
}

bool RadioGroup::select_next() {
    if (options_.empty() || !visible() || !enabled()) return false;
    return set_selected((selected_ + 1) % static_cast<int>(options_.size()));
}

bool RadioGroup::select_previous() {
    if (options_.empty() || !visible() || !enabled()) return false;
    const int count = static_cast<int>(options_.size());
    return set_selected((selected_ - 1 + count) % count);
}

void RadioGroup::set_font(const Font* font) noexcept {
    font_ = font;
}

const Font* RadioGroup::font() const noexcept {
    return font_;
}

void RadioGroup::set_style(RadioGroupStyle style) noexcept {
    style_ = style;
}

const RadioGroupStyle& RadioGroup::style() const noexcept {
    return style_;
}

void RadioGroup::set_on_change(std::function<void(int)> callback) {
    on_change_ = std::move(callback);
}

void RadioGroup::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty() || options_.empty()) return;

    const int row_height = std::max(1, area.h / static_cast<int>(options_.size()));
    const int radius = std::max(2, std::min(10, row_height / 4));

    for (int i = 0; i < static_cast<int>(options_.size()); ++i) {
        const Rect row{area.x, area.y + i * row_height,
                       area.w, i == static_cast<int>(options_.size()) - 1
                                   ? area.h - i * row_height : row_height};
        if (row.empty()) continue;

        const int cy = row.y + row.h / 2;
        const int cx = row.x + radius + 5;
        const Rect indicator{cx - radius, cy - radius, radius * 2, radius * 2};
        renderer.fill_rect(row, style_.background);
        renderer.outline_rect(indicator,
            focused() ? style_.focused_border : style_.border);

        if (i == selected_) {
            const int inner = std::max(1, radius / 2);
            renderer.fill_rect(
                {cx - inner, cy - inner, inner * 2, inner * 2},
                enabled() ? style_.selected : style_.disabled_text
            );
        }

        if (font_ && font_->valid()) {
            const Rect text_area{cx + radius + 10, row.y,
                                 row.w - (cx - row.x) - radius - 10, row.h};
            if (!text_area.empty()) {
                TextRenderer text(renderer);
                text.draw_centered(options_[i], *font_, text_area,
                    enabled() ? style_.text : style_.disabled_text);
            }
        }
    }
}

}
