// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../ui/UI.hpp"
#include "../services/MovieCatalog.hpp"

#include <algorithm>

void drawMovies(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.movieSubtitle, 360, 119);
    drawBattery(ui);

    const SDL_Rect tiles[4] = {
        {88, 174, 252, 190}, {380, 174, 252, 190},
        {88, 396, 252, 190}, {380, 396, 252, 190}
    };

    const auto& movieList = KiddosServices::movies();
    const int itemCount = std::min(4, static_cast<int>(movieList.size()));
    const SDL_Color muted = {155, 177, 215, 255};

    for (int i = 0; i < 4; ++i) {
        const SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &tile);

        const bool occupied = i < itemCount &&
            !movieList[static_cast<std::size_t>(i)].path.empty();
        if (occupied && ui.movieImages[i]) {
            int iw = 0, ih = 0;
            if (SDL_QueryTexture(ui.movieImages[i], nullptr, nullptr,
                                 &iw, &ih) == 0 && iw > 0 && ih > 0) {
                const float scale = std::min(224.0f / iw, 166.0f / ih);
                SDL_Rect image = {
                    tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
                    tile.y + (tile.h - static_cast<int>(ih * scale)) / 2,
                    static_cast<int>(iw * scale),
                    static_cast<int>(ih * scale)
                };
                SDL_RenderCopy(r, ui.movieImages[i], nullptr, &image);
            }
        } else {
            const char* label = occupied ? "NO ARTWORK" : "EMPTY SLOT";
            drawWifiLine(r, ui.footerFont, label,
                         tile.x + 70, tile.y + 85, muted);
        }

        if (i == ui.mediaBrowser->movies.selectedRow && occupied) {
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            for (int n = 0; n < 5; ++n) {
                SDL_Rect outline = {
                    tile.x - n, tile.y - n,
                    tile.w + n * 2, tile.h + n * 2
                };
                SDL_RenderDrawRect(r, &outline);
            }
        }
    }

    drawText(r, ui.movieFooter, 360, 625);
    SDL_RenderPresent(r);
}
