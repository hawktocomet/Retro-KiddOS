#include "kiddos/ui/text.hpp"

namespace kiddos::ui {
TextRenderer::TextRenderer(Renderer& renderer) noexcept
    : renderer_(renderer) {}

bool TextRenderer::draw(const std::string& text, const Font& font,
                        Point position, Color color) {
    if (!renderer_.valid() || !font.valid() || text.empty()) return false;
    SDL_Color tint{color.r, color.g, color.b, color.a};
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended(font.native(), text.c_str(), tint);
    if (!surface) return false;
    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer_.native(), surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return false;
    }
    SDL_Rect destination{position.x, position.y, surface->w, surface->h};
    const int result =
        SDL_RenderCopy(renderer_.native(), texture, nullptr, &destination);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    return result == 0;
}

bool TextRenderer::draw_centered(const std::string& text, const Font& font,
                                 Rect area, Color color) {
    if (!font.valid() || text.empty() || area.empty()) return false;
    int width = 0;
    int height = 0;
    if (TTF_SizeUTF8(font.native(), text.c_str(), &width, &height) != 0)
        return false;
    return draw(text, font,
        {area.x + (area.w - width) / 2,
         area.y + (area.h - height) / 2}, color);
}
}
