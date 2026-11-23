// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../UI.hpp"
#include "../services/TVShowCatalog.hpp"

#include <algorithm>

void drawTVShows(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.tvSubtitle, 360, 119);
    drawBattery(ui);

    const SDL_Rect tiles[4] = {
        {88, 174, 252, 190}, {380, 174, 252, 190},
        {88, 396, 252, 190}, {380, 396, 252, 190}
    };

    const auto& shows = KiddosServices::tvShows();
    const int count = std::min(4, static_cast<int>(shows.size()));
    const SDL_Color muted = {155, 177, 215, 255};

    for (int i = 0; i < 4; ++i) {
        const SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &tile);

        if (i < count && ui.tvImages[i]) {
            int iw = 0, ih = 0;
            if (SDL_QueryTexture(ui.tvImages[i], nullptr, nullptr,
                                 &iw, &ih) == 0 && iw > 0 && ih > 0) {
                const float scale = std::min(224.0f / iw, 166.0f / ih);
                SDL_Rect image = {
                    tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
                    tile.y + (tile.h - static_cast<int>(ih * scale)) / 2,
                    static_cast<int>(iw * scale),
                    static_cast<int>(ih * scale)
                };
                SDL_RenderCopy(r, ui.tvImages[i], nullptr, &image);
            }
        } else {
            drawWifiLine(r, ui.footerFont,
                i < count ? "NO ARTWORK" : "EMPTY SLOT",
                tile.x + 70, tile.y + 85, muted);
        }

        if (i == ui.tvBrowser.selectedRow && i < count) {
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

    drawText(r, ui.tvFooter, 360, 625);
    SDL_RenderPresent(r);
}
