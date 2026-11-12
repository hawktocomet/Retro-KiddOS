#include "kiddos/app/home_carousel_screen.hpp"

#include "kiddos/ui/image.hpp"
#include "kiddos/ui/renderer.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <cassert>
#include <cstdint>
#include <unistd.h>

namespace {

using kiddos::ui::Color;
using kiddos::ui::Rect;

Color pixel(SDL_Surface* surface, int x, int y) {
    assert(SDL_LockSurface(surface) == 0);
    const auto* row = static_cast<const std::uint8_t*>(surface->pixels)
        + y * surface->pitch;
    const auto* value = reinterpret_cast<const std::uint32_t*>(row) + x;
    std::uint8_t r = 0, g = 0, b = 0, a = 0;
    SDL_GetRGBA(*value, surface->format, &r, &g, &b, &a);
    SDL_UnlockSurface(surface);
    return {r, g, b, a};
}

bool same_color(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g &&
           a.b == b.b && a.a == b.a;
}

bool any_pixel_different(
    SDL_Surface* surface,
    const Rect& rect,
    const Color& color
) {
    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        for (int x = rect.x; x < rect.x + rect.w; ++x) {
            if (!same_color(pixel(surface, x, y), color)) return true;
        }
    }
    return false;
}

const kiddos::ui::Element* find_by_bounds(
    kiddos::ui::Screen& screen,
    const Rect& bounds
) {
    for (std::size_t i = 0; i < screen.root().child_count(); ++i) {
        const auto* child = screen.root().child(i);
        if (!child) continue;

        const Rect actual = child->bounds();
        if (actual.x == bounds.x && actual.y == bounds.y &&
            actual.w == bounds.w && actual.h == bounds.h) {
            return child;
        }
    }
    return nullptr;
}

void test_structure_and_selection() {
    kiddos::app::HomeCarouselScreen screen(nullptr);

    // Three game-cover slots, wordmark, hints, status, and highlight.
    assert(screen.root().child_count() == 6);
    assert(screen.selected_game() == 1);

    const auto* wordmark = find_by_bounds(screen, {185, 4, 350, 64});
    assert(wordmark);
    assert(wordmark->visible());

    const auto* hints = find_by_bounds(screen, {0, 662, 720, 42});
    assert(hints);
    assert(hints->visible());

    screen.set_selected_game(0);
    assert(screen.selected_game() == 0);
    assert(screen.selected_game() == 0);

    screen.set_selected_game(3);
    assert(screen.selected_game() == 3);

    screen.set_selected_game(4);
    assert(screen.selected_game() == 3);

    screen.set_selected_game(0);
    assert(screen.selected_game() == 0);

    // The transition can be advanced to completion without an SDL renderer.
    screen.update(1.0f);
}

void test_renders_minimal_carousel() {
    assert(SDL_Init(0) == 0);
    IMG_Init(IMG_INIT_PNG);
    assert(chdir(KIDDOS_SOURCE_DIR) == 0);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, 720, 720, 32, SDL_PIXELFORMAT_RGBA32);
    assert(surface);

    SDL_Renderer* native = SDL_CreateSoftwareRenderer(surface);
    assert(native);
    SDL_SetRenderDrawBlendMode(native, SDL_BLENDMODE_BLEND);

    {
        kiddos::ui::Renderer renderer(native);
        kiddos::app::HomeCarouselScreen screen(native);

        const Color background{8, 15, 35, 255};
        renderer.clear(background);
        screen.render(renderer);

        // The screen is no longer composed of large framed panels.
        assert(same_color(pixel(surface, 540, 160), background));

        // Three game-cover positions, wordmark, and hints are visible.
        assert(any_pixel_different(surface, {220, 158, 280, 280}, background));
        assert(any_pixel_different(surface, {24, 268, 128, 128}, background));
        assert(any_pixel_different(surface, {568, 268, 128, 128}, background));
        assert(any_pixel_different(surface, {185, 4, 350, 64}, background));
        assert(any_pixel_different(surface, {0, 662, 720, 42}, background));

        // Selecting another game updates all three visible cover slots.
        screen.set_selected_game(0);
        screen.update(0.05f);
        screen.render(renderer);

        screen.update(1.0f);
        screen.render(renderer);

        assert(any_pixel_different(surface, {220, 158, 280, 280}, background));
    }

    SDL_DestroyRenderer(native);
    SDL_FreeSurface(surface);
    IMG_Quit();
    SDL_Quit();
}

} // namespace

int main() {
    test_structure_and_selection();
    test_renders_minimal_carousel();
    return 0;
}
