// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../UI.hpp"
#include "../services/MovieCatalog.hpp"

#include <algorithm>
#include <string>

void drawMovies(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 85);

    const SDL_Color normal = {235, 240, 250, 255};
    const SDL_Color muted = {165, 180, 200, 255};
    const int rowHeight = 42;
    constexpr int visibleRows = 8;

    const auto& movieList = KiddosServices::movies();
    const int itemCount = static_cast<int>(movieList.size());

    drawWifiLine(r, ui.labelFont, "CHOOSE A MOVIE", 90, 155, normal);

    if (itemCount == 0) {
        drawWifiLine(r, ui.labelFont, "NO MOVIES FOUND", 90, 270, muted);
    } else {
        for (int visible = 0; visible < visibleRows; ++visible) {
            const int index = ui.movieBrowser.scrollOffset + visible;
            if (index >= itemCount) break;

            SDL_Rect row = {78, 205 + visible * rowHeight, 564, 36};

            if (index == ui.movieBrowser.selectedRow) {
                SDL_SetRenderDrawColor(r, 65, 165, 235, 255);
                for (int n = 0; n < 2; ++n) {
                    SDL_Rect outline = {
                        row.x - n, row.y - n,
                        row.w + n * 2, row.h + n * 2
                    };
                    SDL_RenderDrawRect(r, &outline);
                }
            }

            drawWifiLine(
                r, ui.labelFont, movieList[static_cast<std::size_t>(index)].title,
                row.x + 14, row.y + 7,
                index == ui.movieBrowser.selectedRow ? normal : muted);
        }
    }

    drawBattery(ui);
    drawWifiLine(r, ui.footerFont,
                 "UP/DOWN MOVE   A PLAY   B BACK   START MENU",
                 125, 625, muted);
    SDL_RenderPresent(r);
}
