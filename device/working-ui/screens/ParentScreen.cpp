// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../UI.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

void drawParentCenter(UI& ui, int selected) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 92);
    drawText(r, ui.parentHeading, 360, 155);

    const char* labels[7] = {
        "REMOTE MANAGEMENT", "GAMES", "MOVIES", "MUSIC PLAYLISTS",
        "TV SHOWS", "WI-FI SETTINGS", "CHANGE PARENT PIN"
    };
    for (int i = 0; i < 7; ++i) {
        const int y = 205 + i * 57;
        SDL_Rect row{90, y - 7, 540, 45};
        if (i == selected) {
            SDL_SetRenderDrawColor(r, 28, 65, 105, 255);
            SDL_RenderFillRect(r, &row);
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            SDL_RenderDrawRect(r, &row);
        }
        drawWifiLine(r, ui.labelFont, labels[i], 120, y, {245,248,255,255});
    }
    drawBattery(ui);
    drawText(r, ui.parentFooter, 360, 625);
    SDL_RenderPresent(r);
}
