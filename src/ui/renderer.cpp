#include "kiddos/ui/renderer.hpp"

#include <algorithm>
#include <cmath>

namespace kiddos::ui {

Renderer::Renderer(SDL_Renderer* renderer) noexcept
    : renderer_(renderer) {
}

void Renderer::attach(SDL_Renderer* renderer) noexcept {
    renderer_ = renderer;
}

bool Renderer::valid() const noexcept {
    return renderer_ != nullptr;
}

SDL_Renderer* Renderer::native() const noexcept {
    return renderer_;
}

bool Renderer::set_color(Color color) const {
    if (!renderer_) return false;

    return SDL_SetRenderDrawColor(
        renderer_,
        color.r,
        color.g,
        color.b,
        color.a
    ) == 0;
}

bool Renderer::clear(Color color) const {
    if (!set_color(color)) return false;
    return SDL_RenderClear(renderer_) == 0;
}

bool Renderer::fill_rect(Rect rect, Color color) const {
    if (!renderer_ || rect.empty() || !set_color(color)) return false;

    SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
    return SDL_RenderFillRect(renderer_, &native_rect) == 0;
}

bool Renderer::outline_rect(Rect rect, Color color) const {
    if (!renderer_ || rect.empty() || !set_color(color)) return false;

    SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
    return SDL_RenderDrawRect(renderer_, &native_rect) == 0;
}

bool Renderer::draw_line(Point from, Point to, Color color) const {
    if (!renderer_ || !set_color(color)) return false;

    return SDL_RenderDrawLine(
        renderer_, from.x, from.y, to.x, to.y
    ) == 0;
}

bool Renderer::draw_point(Point point, Color color) const {
    if (!renderer_ || !set_color(color)) return false;

    return SDL_RenderDrawPoint(
        renderer_, point.x, point.y
    ) == 0;
}

bool Renderer::draw_texture(
    SDL_Texture* texture,
    Rect source,
    Rect destination,
    std::uint8_t opacity
) const {
    if (!renderer_ || !texture || source.empty() || destination.empty()) return false;

    Uint8 previous_opacity = 255;
    if (SDL_GetTextureAlphaMod(texture, &previous_opacity) != 0) return false;
    if (SDL_SetTextureAlphaMod(texture, opacity) != 0) return false;

    SDL_Rect src{source.x, source.y, source.w, source.h};
    SDL_Rect dst{destination.x, destination.y, destination.w, destination.h};
    const bool rendered = SDL_RenderCopy(renderer_, texture, &src, &dst) == 0;
    const bool restored = SDL_SetTextureAlphaMod(texture, previous_opacity) == 0;

    return rendered && restored;
}

bool Renderer::fill_rounded_rect(Rect rect, int radius, Color color) const {
    if (!renderer_ || rect.empty() || !set_color(color)) return false;

    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    if (radius == 0) {
        SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
        return SDL_RenderFillRect(renderer_, &native_rect) == 0;
    }

    for (int y = 0; y < rect.h; ++y) {
        int dy = 0;
        if (y < radius) {
            dy = radius - y;
        } else if (y >= rect.h - radius) {
            dy = y - (rect.h - radius - 1);
        }

        const double inside = std::max(
            0.0,
            static_cast<double>(radius * radius - dy * dy)
        );
        const int inset = static_cast<int>(
            std::ceil(radius - std::sqrt(inside))
        );

        if (SDL_RenderDrawLine(
                renderer_,
                rect.x + inset,
                rect.y + y,
                rect.x + rect.w - 1 - inset,
                rect.y + y
            ) != 0) {
            return false;
        }
    }

    return true;
}

bool Renderer::outline_rounded_rect(Rect rect, int radius, Color color) const {
    if (!renderer_ || rect.empty() || !set_color(color)) return false;

    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    if (radius == 0) {
        SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
        return SDL_RenderDrawRect(renderer_, &native_rect) == 0;
    }

    for (int y = 0; y < rect.h; ++y) {
        int dy = 0;
        if (y < radius) {
            dy = radius - y;
        } else if (y >= rect.h - radius) {
            dy = y - (rect.h - radius - 1);
        }

        const double inside = std::max(
            0.0,
            static_cast<double>(radius * radius - dy * dy)
        );
        const int inset = static_cast<int>(
            std::ceil(radius - std::sqrt(inside))
        );

        if (SDL_RenderDrawPoint(renderer_, rect.x + inset, rect.y + y) != 0 ||
            SDL_RenderDrawPoint(renderer_, rect.x + rect.w - 1 - inset, rect.y + y) != 0) {
            return false;
        }
    }

    if (SDL_RenderDrawLine(
            renderer_, rect.x + radius, rect.y,
            rect.x + rect.w - radius - 1, rect.y
        ) != 0) {
        return false;
    }

    return SDL_RenderDrawLine(
        renderer_, rect.x + radius, rect.y + rect.h - 1,
        rect.x + rect.w - radius - 1, rect.y + rect.h - 1
    ) == 0;
}

bool Renderer::set_clip(Rect rect) const {
    if (!renderer_) return false;

    SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
    return SDL_RenderSetClipRect(renderer_, &native_rect) == 0;
}

bool Renderer::clear_clip() const {
    if (!renderer_) return false;

    return SDL_RenderSetClipRect(renderer_, nullptr) == 0;
}

void Renderer::present() const {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

} // namespace kiddos::ui
