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

namespace {

// Distance from the rect edge to the rounded-corner boundary at the row or
// column `steps` in from the corner, sampled at the pixel center. Fills use
// this so corner arcs are smooth and evenly shaped.
int corner_inset(int radius, int steps) {
    const double dy = static_cast<double>(radius - steps) - 0.5;
    const double inside = std::max(
        0.0,
        static_cast<double>(radius) * radius - dy * dy
    );
    return radius - static_cast<int>(std::sqrt(inside));
}

} // namespace

bool Renderer::fill_rounded_rect(Rect rect, int radius, Color color) const {
    if (!renderer_ || rect.empty() || !set_color(color)) return false;

    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    if (radius == 0) {
        SDL_Rect native_rect{rect.x, rect.y, rect.w, rect.h};
        return SDL_RenderFillRect(renderer_, &native_rect) == 0;
    }

    for (int y = 0; y < rect.h; ++y) {
        int inset = 0;
        if (y < radius) {
            inset = corner_inset(radius, y);
        } else if (y >= rect.h - radius) {
            inset = corner_inset(radius, rect.h - 1 - y);
        }

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

    // Straight edges between the corner arcs.
    if (SDL_RenderDrawLine(
            renderer_, rect.x + radius, rect.y,
            rect.x + rect.w - 1 - radius, rect.y
        ) != 0 ||
        SDL_RenderDrawLine(
            renderer_, rect.x + radius, rect.y + rect.h - 1,
            rect.x + rect.w - 1 - radius, rect.y + rect.h - 1
        ) != 0 ||
        SDL_RenderDrawLine(
            renderer_, rect.x, rect.y + radius,
            rect.x, rect.y + rect.h - 1 - radius
        ) != 0 ||
        SDL_RenderDrawLine(
            renderer_, rect.x + rect.w - 1, rect.y + radius,
            rect.x + rect.w - 1, rect.y + rect.h - 1 - radius
        ) != 0) {
        return false;
    }

    // Corner arcs, traced along the true circle with dense angle steps so
    // every corner stays continuous with no gaps. Points that land on the
    // straight edge lines are skipped: the lines already cover them, and
    // re-drawing would blend semi-transparent borders twice on one pixel.
    constexpr double kPi = 3.14159265358979323846;
    const int steps = std::max(2, radius * 2);
    const int right = rect.x + rect.w - 1;
    const int bottom = rect.y + rect.h - 1;

    const auto on_edge_line = [&](ui::Point point) {
        if (point.y == rect.y || point.y == bottom) {
            return point.x >= rect.x + radius && point.x <= right - radius;
        }
        if (point.x == rect.x || point.x == right) {
            return point.y >= rect.y + radius && point.y <= bottom - radius;
        }
        return false;
    };

    const auto draw_corner = [&](int center_x, int center_y, int sign_x,
                                 int sign_y) {
        ui::Point previous{
            center_x + sign_x * radius,
            center_y
        };

        for (int step = 1; step < steps; ++step) {
            const double angle = (kPi / 2.0) * step / steps;
            const ui::Point point{
                center_x + sign_x *
                    static_cast<int>(std::lround(radius * std::cos(angle))),
                center_y + sign_y *
                    static_cast<int>(std::lround(radius * std::sin(angle)))
            };

            if ((point.x != previous.x || point.y != previous.y) &&
                !on_edge_line(point)) {
                if (SDL_RenderDrawPoint(
                        renderer_, point.x, point.y
                    ) != 0) {
                    return false;
                }
            }
            previous = point;
        }

        return true;
    };

    if (!draw_corner(rect.x + radius, rect.y + radius, -1, -1) ||
        !draw_corner(
            rect.x + rect.w - 1 - radius, rect.y + radius, 1, -1
        ) ||
        !draw_corner(
            rect.x + radius, rect.y + rect.h - 1 - radius, -1, 1
        ) ||
        !draw_corner(
            rect.x + rect.w - 1 - radius,
            rect.y + rect.h - 1 - radius,
            1,
            1
        )) {
        return false;
    }

    return true;
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
