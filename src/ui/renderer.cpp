#include "kiddos/ui/renderer.hpp"

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
