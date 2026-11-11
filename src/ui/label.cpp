#include "kiddos/ui/label.hpp"
#include "kiddos/ui/text.hpp"

#include <SDL_ttf.h>
#include <utility>

namespace kiddos::ui {

Label::Label(std::string text)
    : text_(std::move(text)) {
}

void Label::set_text(std::string text) {
    text_ = std::move(text);
}

const std::string& Label::text() const noexcept {
    return text_;
}

void Label::set_font(const Font* font) noexcept {
    font_ = font;
}

const Font* Label::font() const noexcept {
    return font_;
}

void Label::set_color(Color color) noexcept {
    color_ = color;
}

Color Label::color() const noexcept {
    return color_;
}

void Label::set_alignment(LabelAlignment alignment) noexcept {
    alignment_ = alignment;
}

LabelAlignment Label::alignment() const noexcept {
    return alignment_;
}

void Label::draw(Renderer& renderer) {
    const Rect area = bounds();

    if (!font_ || !font_->valid() || text_.empty() || area.empty()) {
        return;
    }

    int width = 0;
    int height = 0;

    if (TTF_SizeUTF8(font_->native(), text_.c_str(), &width, &height) != 0) {
        return;
    }

    int x = area.x;

    switch (alignment_) {
        case LabelAlignment::left:
            break;
        case LabelAlignment::center:
            x += (area.w - width) / 2;
            break;
        case LabelAlignment::right:
            x += area.w - width;
            break;
    }

    const int y = area.y + (area.h - height) / 2;

    TextRenderer text_renderer(renderer);
    text_renderer.draw(text_, *font_, {x, y}, color_);
}

}
