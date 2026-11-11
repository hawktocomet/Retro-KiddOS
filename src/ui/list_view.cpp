#include "kiddos/ui/list_view.hpp"
#include "kiddos/ui/text.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

ListView::ListView() {
    set_focusable(true);
}

void ListView::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    selected_index_ = items_.empty() ? -1 : 0;
    scroll_offset_ = 0;
}

const std::vector<std::string>& ListView::items() const noexcept {
    return items_;
}

int ListView::selected_index() const noexcept {
    return selected_index_;
}

bool ListView::set_selected_index(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) return false;
    if (selected_index_ == index) return true;
    selected_index_ = index;
    ensure_selection_visible();
    return true;
}

bool ListView::move_selection(int direction) {
    if (!visible() || !enabled() || direction == 0 || items_.empty()) {
        return false;
    }

    const int next = selected_index_ + (direction > 0 ? 1 : -1);
    if (next < 0 || next >= static_cast<int>(items_.size())) return false;

    selected_index_ = next;
    ensure_selection_visible();
    return true;
}

bool ListView::activate_selected() {
    if (!visible() || !enabled() || selected_index_ < 0 ||
        selected_index_ >= static_cast<int>(items_.size()) || !on_activate_) {
        return false;
    }

    on_activate_(selected_index_);
    return true;
}

void ListView::set_row_height(int height) noexcept {
    row_height_ = std::max(1, height);
    ensure_selection_visible();
}

int ListView::row_height() const noexcept {
    return row_height_;
}

void ListView::set_font(const Font* font) noexcept {
    font_ = font;
}

void ListView::set_style(ListViewStyle style) noexcept {
    style_ = style;
}

const ListViewStyle& ListView::style() const noexcept {
    return style_;
}

void ListView::set_on_activate(std::function<void(int)> callback) {
    on_activate_ = std::move(callback);
}

void ListView::ensure_selection_visible() noexcept {
    const Rect area = bounds();
    if (area.h <= 0 || row_height_ <= 0 || selected_index_ < 0) {
        scroll_offset_ = 0;
        return;
    }

    const int visible_rows = std::max(1, area.h / row_height_);
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + visible_rows) {
        scroll_offset_ = selected_index_ - visible_rows + 1;
    }

    const int maximum_offset = std::max(
        0, static_cast<int>(items_.size()) - visible_rows
    );
    scroll_offset_ = std::clamp(scroll_offset_, 0, maximum_offset);
}

void ListView::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    renderer.fill_rect(area, style_.background);
    renderer.outline_rect(area, style_.border);

    const int visible_rows = (area.h + row_height_ - 1) / row_height_;
    const int end = std::min(
        static_cast<int>(items_.size()), scroll_offset_ + visible_rows
    );

    renderer.set_clip(area);
    TextRenderer text(renderer);

    for (int index = scroll_offset_; index < end; ++index) {
        const int row_y = area.y + (index - scroll_offset_) * row_height_;
        const Rect row{area.x + 1, row_y, std::max(0, area.w - 2), row_height_};

        if (index == selected_index_) {
            renderer.fill_rect(row, style_.selected);
        }

        if (font_ && font_->valid() && !items_[index].empty()) {
            const Color color = enabled() ? style_.text : style_.disabled_text;
            text.draw(items_[index], *font_,
                      {row.x + 12, row.y + std::max(0, (row.h - 20) / 2)},
                      color);
        }
    }

    renderer.clear_clip();
    if (focused()) renderer.outline_rect(area, style_.selected);
}

}
