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
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.parentHeading, 360, 220);

    SDL_Rect rows[3] = {
        {90, 270, 540, 72},
        {90, 370, 540, 72},
        {90, 470, 540, 72}
    };
    SDL_Rect selectedRow = rows[selected];

    SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
    for (int n = 0; n < 3; ++n) {
        SDL_Rect outline = {
            selectedRow.x - n, selectedRow.y - n,
            selectedRow.w + n * 2, selectedRow.h + n * 2
        };
        SDL_RenderDrawRect(r, &outline);
    }

    drawText(r, ui.parentBody, 360, 286);
    drawText(r, ui.parentUrl, 360, 320);
    drawText(r, ui.parentWifi, 360, 394);
    drawText(r, ui.parentChangePin, 360, 494);
    drawBattery(ui);
    drawText(r, ui.parentFooter, 360, 625);
    SDL_RenderPresent(r);
}
