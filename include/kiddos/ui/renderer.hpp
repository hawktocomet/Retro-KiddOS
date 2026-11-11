#pragma once

#include "kiddos/ui/color.hpp"
#include "kiddos/ui/geometry.hpp"

#include <SDL.h>
#include <cstdint>

namespace kiddos::ui {

// Non-owning SDL2 renderer adapter.
// The SDLPlatform remains responsible for creating and destroying SDL_Renderer.
class Renderer final {
public:
    explicit Renderer(SDL_Renderer* renderer = nullptr) noexcept;

    void attach(SDL_Renderer* renderer) noexcept;
    bool valid() const noexcept;

    bool clear(Color color) const;
    bool fill_rect(Rect rect, Color color) const;
    bool outline_rect(Rect rect, Color color) const;
    bool draw_line(Point from, Point to, Color color) const;
    bool draw_point(Point point, Color color) const;
    bool draw_texture(SDL_Texture* texture, Rect source, Rect destination,
                      std::uint8_t opacity = 255) const;
    bool fill_rounded_rect(Rect rect, int radius, Color color) const;
    bool outline_rounded_rect(Rect rect, int radius, Color color) const;

    bool set_clip(Rect rect) const;
    bool clear_clip() const;

    void present() const;

    SDL_Renderer* native() const noexcept;

private:
    SDL_Renderer* renderer_ = nullptr;

    bool set_color(Color color) const;
};

} // namespace kiddos::ui
