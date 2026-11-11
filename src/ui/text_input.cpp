#include "kiddos/ui/text_input.hpp"
#include "kiddos/ui/text.hpp"

#include <algorithm>
#include <utility>

namespace kiddos::ui {

TextInput::TextInput(std::string text) {
    set_focusable(true);
    set_text(std::move(text));
}

std::size_t TextInput::previous_boundary(const std::string& text,
                                         std::size_t position) noexcept {
    if (position == 0) return 0;
    --position;
    while (position > 0 &&
           (static_cast<unsigned char>(text[position]) & 0xC0) == 0x80)
        --position;
    return position;
}

std::size_t TextInput::next_boundary(const std::string& text,
                                     std::size_t position) noexcept {
    if (position >= text.size()) return text.size();
    ++position;
    while (position < text.size() &&
           (static_cast<unsigned char>(text[position]) & 0xC0) == 0x80)
        ++position;
    return position;
}

std::size_t TextInput::count_codepoints(const std::string& text) noexcept {
    std::size_t count = 0;
    for (unsigned char c : text)
        if ((c & 0xC0) != 0x80) ++count;
    return count;
}

void TextInput::set_text(std::string text) {
    if (count_codepoints(text) > max_length_) {
        std::size_t position = 0;
        for (std::size_t i = 0; i < max_length_; ++i)
            position = next_boundary(text, position);
        text.resize(position);
    }
    if (text_ == text) return;
    text_ = std::move(text);
    cursor_ = text_.size();
    notify_change();
}

const std::string& TextInput::text() const noexcept {
    return text_;
}

void TextInput::set_placeholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
}

const std::string& TextInput::placeholder() const noexcept {
    return placeholder_;
}

void TextInput::set_max_length(std::size_t length) {
    max_length_ = length;
    set_text(text_);
}

std::size_t TextInput::max_length() const noexcept {
    return max_length_;
}

bool TextInput::insert_text(const std::string& value) {
    if (!enabled() || !visible() || value.empty()) return false;

    const std::size_t current = count_codepoints(text_);
    const std::size_t available =
        current >= max_length_ ? 0 : max_length_ - current;
    const std::size_t incoming = count_codepoints(value);
    if (available == 0) return false;

    std::size_t end = value.size();
    if (incoming > available) {
        end = 0;
        for (std::size_t i = 0; i < available; ++i)
            end = next_boundary(value, end);
    }

    text_.insert(cursor_, value, 0, end);
    cursor_ += end;
    notify_change();
    return true;
}

bool TextInput::backspace() {
    if (!enabled() || !visible() || cursor_ == 0) return false;
    const std::size_t start = previous_boundary(text_, cursor_);
    text_.erase(start, cursor_ - start);
    cursor_ = start;
    notify_change();
    return true;
}

bool TextInput::delete_forward() {
    if (!enabled() || !visible() || cursor_ >= text_.size()) return false;
    const std::size_t end = next_boundary(text_, cursor_);
    text_.erase(cursor_, end - cursor_);
    notify_change();
    return true;
}

void TextInput::move_cursor_left() noexcept {
    cursor_ = previous_boundary(text_, cursor_);
}

void TextInput::move_cursor_right() noexcept {
    cursor_ = next_boundary(text_, cursor_);
}

void TextInput::move_cursor_home() noexcept {
    cursor_ = 0;
}

void TextInput::move_cursor_end() noexcept {
    cursor_ = text_.size();
}

std::size_t TextInput::cursor_position() const noexcept {
    return cursor_;
}

void TextInput::set_cursor_position(std::size_t position) noexcept {
    position = std::min(position, text_.size());
    while (position > 0 && position < text_.size() &&
           (static_cast<unsigned char>(text_[position]) & 0xC0) == 0x80)
        --position;
    cursor_ = position;
}

void TextInput::set_font(const Font* font) noexcept {
    font_ = font;
}

const Font* TextInput::font() const noexcept {
    return font_;
}

void TextInput::set_style(TextInputStyle style) noexcept {
    style_ = style;
}

const TextInputStyle& TextInput::style() const noexcept {
    return style_;
}

void TextInput::set_on_change(
    std::function<void(const std::string&)> callback) {
    on_change_ = std::move(callback);
}

void TextInput::notify_change() {
    if (on_change_) on_change_(text_);
}

void TextInput::draw(Renderer& renderer) {
    const Rect area = bounds();
    if (area.empty()) return;

    renderer.fill_rect(area, style_.background);
    renderer.outline_rect(
        area, focused() ? style_.focused_border : style_.border);

    if (!font_ || !font_->valid()) return;

    const std::string& displayed = text_.empty() ? placeholder_ : text_;
    const Color foreground = !enabled()
        ? style_.disabled_text
        : (text_.empty() ? style_.placeholder : style_.text);

    const int padding = 8;
    const Rect text_area{
        area.x + padding, area.y + padding / 2,
        std::max(0, area.w - padding * 2),
        std::max(0, area.h - padding)
    };

    if (!displayed.empty()) {
        TextRenderer text_renderer(renderer);
        text_renderer.draw(displayed, *font_,
                           {text_area.x, text_area.y}, foreground);
    }

    if (focused() && enabled()) {
        const std::string before = text_.substr(0, cursor_);
        int cursor_width = 0;
        int cursor_height = 0;
        if (TTF_SizeUTF8(font_->native(), before.c_str(),
                         &cursor_width, &cursor_height) == 0) {
            const int x = text_area.x + cursor_width;
            const int y = text_area.y;
            renderer.draw_line(
                {x, y}, {x, y + std::min(cursor_height, text_area.h)},
                style_.cursor);
        }
    }
}

}
