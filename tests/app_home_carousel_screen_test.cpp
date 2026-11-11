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

// True when any pixel within `radius` steps of the point matches `color`.
bool any_pixel_near(
    SDL_Surface* surface,
    int x,
    int y,
    int radius,
    const Color& color
) {
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (same_color(pixel(surface, x + dx, y + dy), color)) {
                return true;
            }
        }
    }
    return false;
}

// X of the first non-background pixel on a row, or rect.x + rect.w when
// the row is empty.
int first_content_pixel_x(
    SDL_Surface* surface,
    const Rect& rect,
    int y,
    const Color& color
) {
    for (int x = rect.x; x < rect.x + rect.w; ++x) {
        if (!same_color(pixel(surface, x, y), color)) return x;
    }
    return rect.x + rect.w;
}

// True when any pixel in the rect differs from `color`.
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

// Generated artwork must not leave magenta keying artifacts (pink fringes)
// on screen; a leftover fringe pixel would have strong red and blue with
// almost no green.
bool has_magenta_fringe(SDL_Surface* surface, const Rect& rect) {
    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        for (int x = rect.x; x < rect.x + rect.w; ++x) {
            const Color value = pixel(surface, x, y);
            if (value.r > 150 && value.b > 150 && value.g < 80) return true;
        }
    }
    return false;
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

void test_structure_without_renderer() {
    kiddos::app::HomeCarouselScreen screen(nullptr);

    // Two side cards, two side artworks, center card, glow outline,
    // center artwork, title, wordmark, hints, and the status indicators.
    assert(screen.root().child_count() == 11);
    assert(screen.selected_game() == 1);

    // The title area sits centered directly below the center card and is
    // always visible; its texture follows the selected game.
    const Element* title =
        find_by_bounds(screen, {145, 523, 430, 44});
    assert(title);
    assert(title->visible());

    // The wordmark and the control hints are present.
    const Element* wordmark =
        find_by_bounds(screen, {235, 12, 250, 46});
    assert(wordmark);
    assert(wordmark->visible());

    const Element* hints =
        find_by_bounds(screen, {0, 682, 720, 34});
    assert(hints);
    assert(hints->visible());

    // Selecting a game swaps the shown title texture.
    const auto* title_image =
        static_cast<const kiddos::ui::Image*>(title);
    const kiddos::ui::Texture* middle_title = title_image->texture();
    assert(middle_title);

    screen.set_selected_game(0);
    assert(screen.selected_game() == 0);
    const kiddos::ui::Texture* left_title = title_image->texture();
    assert(left_title != middle_title);

    screen.set_selected_game(2);
    const kiddos::ui::Texture* right_title = title_image->texture();
    assert(right_title != middle_title);
    assert(right_title != left_title);

    screen.set_selected_game(1);
    assert(title_image->texture() == middle_title);

    screen.set_selected_game(7); // out of range values are ignored
    assert(screen.selected_game() == 1);
    assert(title_image->texture() == middle_title);
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

        // The card borders are clean and uniform: the top edge of the
        // center card is exactly the border color, and the side card edge
        // carries its border color blended over the background.
        assert(same_color(pixel(surface, 360, 135), {36, 164, 255, 255}));
        const Color side_edge = pixel(surface, 120, 250);
        assert(side_edge.b > 110 && side_edge.r > 30);

        // The outer glow outline is continuous around the corner arcs.
        const Color glow_over_background{24, 102, 164, 255};
        assert(any_pixel_near(surface, 152, 141, 2, glow_over_background));
        assert(any_pixel_near(surface, 568, 141, 2, glow_over_background));
        assert(any_pixel_near(surface, 152, 499, 2, glow_over_background));
        assert(any_pixel_near(surface, 568, 499, 2, glow_over_background));

        // The selected game's title appears below the main card.
        const Rect title_area{145, 523, 430, 44};
        assert(any_pixel_different(surface, title_area, background));

        // Selected artwork is drawn inside the main card.
        const Color with_mushroom = pixel(surface, 360, 307);
        assert(!same_color(with_mushroom, background));

        // The KiddOS wordmark and the control hints are rendered.
        assert(any_pixel_different(surface, {235, 12, 250, 46}, background));
        assert(any_pixel_different(surface, {0, 682, 720, 34}, background));

        // No magenta keying artifacts remain in the title or artwork.
        assert(!has_magenta_fringe(surface, title_area));
        assert(!has_magenta_fringe(surface, {260, 205, 200, 205}));

        // Selecting another game changes the artwork and the title: the
        // shorter STAR title is horizontally centered, so its text starts
        // clearly further right than the SUPER MARIO title.
        const int super_mario_x = first_content_pixel_x(
            surface, title_area, 540, background);

        screen.set_selected_game(0);
        renderer.clear(background);
        screen.render(renderer);

        const Color with_star = pixel(surface, 360, 307);
        assert(!same_color(with_star, with_mushroom));
        assert(any_pixel_different(surface, title_area, background));

        const int star_x = first_content_pixel_x(
            surface, title_area, 540, background);
        assert(star_x > super_mario_x + 40);
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
