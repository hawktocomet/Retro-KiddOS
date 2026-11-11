#pragma once

#include <SDL.h>
#include <string>

namespace kiddos::ui {

class Texture final {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Loads an image file through SDL_image. With magenta_key enabled,
    // bright magenta (255, 0, 255) pixels become transparent.
    bool load(SDL_Renderer* renderer, const std::string& path,
              bool magenta_key = false);
    void reset() noexcept;
    bool valid() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    SDL_Texture* native() const noexcept;

private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}
