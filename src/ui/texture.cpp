#include "kiddos/ui/texture.hpp"

#include <SDL_image.h>
#include <utility>

namespace kiddos::ui {

Texture::~Texture() { reset(); }

Texture::Texture(Texture&& other) noexcept
    : texture_(std::exchange(other.texture_, nullptr)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        reset();
        texture_ = std::exchange(other.texture_, nullptr);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
    }
    return *this;
}

bool Texture::load(SDL_Renderer* renderer, const std::string& path) {
    reset();
    if (!renderer || path.empty()) return false;

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) return false;

    SDL_Texture* loaded = SDL_CreateTextureFromSurface(renderer, surface);
    const int w = surface->w;
    const int h = surface->h;
    SDL_FreeSurface(surface);

    if (!loaded) return false;

    texture_ = loaded;
    width_ = w;
    height_ = h;
    return true;
}

void Texture::reset() noexcept {
    if (texture_) SDL_DestroyTexture(texture_);
    texture_ = nullptr;
    width_ = 0;
    height_ = 0;
}

bool Texture::valid() const noexcept { return texture_ != nullptr; }
int Texture::width() const noexcept { return width_; }
int Texture::height() const noexcept { return height_; }
SDL_Texture* Texture::native() const noexcept { return texture_; }

}
