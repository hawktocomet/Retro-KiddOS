#include "kiddos/ui/renderer.hpp"
#include "kiddos/ui/panel.hpp"

#include <SDL.h>
#include <cassert>
#include <cstdint>

static kiddos::ui::Color pixel(SDL_Surface* surface, int x, int y) {
    assert(SDL_LockSurface(surface) == 0);
    const auto* row = static_cast<const std::uint8_t*>(surface->pixels)
        + y * surface->pitch;
    const auto* value = reinterpret_cast<const std::uint32_t*>(row) + x;
    std::uint8_t r = 0, g = 0, b = 0, a = 0;
    SDL_GetRGBA(*value, surface->format, &r, &g, &b, &a);
    SDL_UnlockSurface(surface);
    return {r, g, b, a};
}

int main() {
    using namespace kiddos::ui;
    assert(SDL_Init(0) == 0);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, 32, 24, 32, SDL_PIXELFORMAT_RGBA32
    );
    assert(surface);

    SDL_Renderer* native = SDL_CreateSoftwareRenderer(surface);
    assert(native);

    Renderer renderer(native);
    assert(renderer.clear({0, 0, 0, 255}));

    SDL_Texture* texture = SDL_CreateTexture(
        native, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 1, 1
    );
    assert(texture);
    assert(SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) == 0);

    const std::uint32_t white = 0xffffffffu;
    assert(SDL_UpdateTexture(texture, nullptr, &white, sizeof(white)) == 0);

    Uint8 original_alpha = 0;
    assert(SDL_GetTextureAlphaMod(texture, &original_alpha) == 0);
    assert(original_alpha == 255);

    assert(renderer.draw_texture(texture, {0, 0, 1, 1}, {0, 0, 4, 4}, 128));

    Uint8 restored_alpha = 0;
    assert(SDL_GetTextureAlphaMod(texture, &restored_alpha) == 0);
    assert(restored_alpha == original_alpha);

    renderer.present();
    const Color blended = pixel(surface, 2, 2);
    assert(blended.r >= 120 && blended.r <= 136);
    assert(blended.g >= 120 && blended.g <= 136);
    assert(blended.b >= 120 && blended.b <= 136);

    assert(renderer.clear({0, 0, 0, 255}));

    Panel panel;
    PanelStyle style;
    style.background = {220, 20, 30, 255};
    style.border = {20, 80, 240, 255};
    style.border_thickness = 1;
    style.corner_radius = 5;
    panel.set_style(style);
    panel.set_bounds({8, 4, 16, 16});
    panel.render(renderer);
    renderer.present();

    const Color corner = pixel(surface, 8, 4);
    const Color top_border = pixel(surface, 16, 4);
    const Color center = pixel(surface, 16, 12);

    assert(corner.r == 0 && corner.g == 0 && corner.b == 0);
    assert(top_border.b > top_border.r);
    assert(center.r > center.b);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(native);
    SDL_FreeSurface(surface);
    SDL_Quit();
    return 0;
}
