#include "kiddos/app/home_carousel_screen.hpp"

#include "kiddos/ui/image.hpp"
#include "kiddos/ui/panel.hpp"
#include "kiddos/ui/renderer.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <cassert>
#include <cstdint>
#include <unistd.h>

namespace {

using kiddos::ui::Color;
using kiddos::ui::Element;
using kiddos::ui::Rect;

bool same_rect(const Rect& a, const Rect& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

bool same_color(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

const Element* find_by_bounds(
    kiddos::ui::Screen& screen,
    const Rect& bounds
) {
    for (std::size_t i = 0; i < screen.root().child_count(); ++i) {
        const Element* child = screen.root().child(i);
        if (child && same_rect(child->bounds(), bounds)) return child;
    }
    return nullptr;
}

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

void test_structure_without_renderer() {
    kiddos::app::HomeCarouselScreen screen(nullptr);

    // Two side cards, two side artworks, center card, glow outline,
    // center artwork, title, and the status indicators.
    assert(screen.root().child_count() == 9);
    assert(screen.selected_game() == 1);

    const Element* title =
        find_by_bounds(screen, {220, 525, 280, 42});
    assert(title);
    assert(title->visible());

    screen.set_selected_game(0);
    assert(screen.selected_game() == 0);
    assert(!title->visible());

    screen.set_selected_game(1);
    assert(title->visible());

    screen.set_selected_game(7); // out of range values are ignored
    assert(screen.selected_game() == 1);
}

void test_renders_carousel() {
    assert(SDL_Init(0) == 0);
    IMG_Init(IMG_INIT_PNG);
    assert(chdir(KIDDOS_SOURCE_DIR) == 0);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, 720, 720, 32, SDL_PIXELFORMAT_RGBA32);
    assert(surface);

    SDL_Renderer* native = SDL_CreateSoftwareRenderer(surface);
    assert(native);
    // Match SDLPlatform::initialize, which enables renderer blending.
    SDL_SetRenderDrawBlendMode(native, SDL_BLENDMODE_BLEND);

    {
        kiddos::ui::Renderer renderer(native);
        kiddos::app::HomeCarouselScreen screen(native);
        screen.set_selected_game(1);

        const Color background{8, 15, 35, 255};

        renderer.clear(background);
        screen.render(renderer);

        // Background outside the cards.
        assert(same_color(pixel(surface, 10, 100), background));

        // Center and side card fills keep their established colors.
        assert(same_color(pixel(surface, 500, 160), {18, 63, 117, 255}));
        assert(same_color(pixel(surface, 60, 300), {16, 41, 75, 255}));
        assert(same_color(pixel(surface, 650, 300), {16, 41, 75, 255}));

        // The middle game shows its title artwork below the main card.
        const Color titled = pixel(surface, 397, 542);
        assert(!same_color(titled, background));

        // Selected artwork is drawn inside the main card.
        const Color with_mushroom = pixel(surface, 360, 307);
        assert(!same_color(with_mushroom, background));

        // Selecting another game changes the artwork and hides the title.
        screen.set_selected_game(0);
        renderer.clear(background);
        screen.render(renderer);

        assert(same_color(pixel(surface, 397, 542), background));
        const Color with_star = pixel(surface, 360, 307);
        assert(!same_color(with_star, with_mushroom));
    }

    SDL_DestroyRenderer(native);
    SDL_FreeSurface(surface);
    IMG_Quit();
    SDL_Quit();
}

} // namespace

int main() {
    test_structure_without_renderer();
    test_renders_carousel();
    return 0;
}
