#pragma once
#include <SDL_ttf.h>
#include <string>

namespace kiddos::ui {
class Font final {
public:
    Font() = default;
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;
    bool load(const std::string& path, int point_size);
    void reset() noexcept;
    bool valid() const noexcept;
    TTF_Font* native() const noexcept;
private:
    TTF_Font* font_ = nullptr;
};
}
