#include "kiddos/ui/font.hpp"
#include <utility>

namespace kiddos::ui {
Font::~Font() { reset(); }
Font::Font(Font&& other) noexcept
    : font_(std::exchange(other.font_, nullptr)) {}
Font& Font::operator=(Font&& other) noexcept {
    if (this != &other) {
        reset();
        font_ = std::exchange(other.font_, nullptr);
    }
    return *this;
}
bool Font::load(const std::string& path, int point_size) {
    reset();
    if (path.empty() || point_size <= 0) return false;
    font_ = TTF_OpenFont(path.c_str(), point_size);
    return font_ != nullptr;
}
void Font::reset() noexcept {
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
}
bool Font::valid() const noexcept { return font_ != nullptr; }
TTF_Font* Font::native() const noexcept { return font_; }
}
