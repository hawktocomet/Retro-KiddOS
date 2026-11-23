// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "../UI.hpp"
#include "../services/MusicCatalog.hpp"
#include <algorithm>
#include <string>

void drawMusic(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.musicSubtitle, 360, 119);
    drawBattery(ui);
    const SDL_Rect tiles[4] = {
        {88,174,252,190}, {380,174,252,190},
        {88,396,252,190}, {380,396,252,190}
    };
    const auto& tracks = KiddosServices::music();
    const int count = std::min(4, static_cast<int>(tracks.size()));
    const SDL_Color muted = {155,177,215,255};
    for (int i=0; i<4; ++i) {
        SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r,28,38,68,255);
        SDL_RenderFillRect(r,&tile);
        std::string label = i < count ? tracks[static_cast<std::size_t>(i)].title : "EMPTY SLOT";
        if (label.size() > 25) label = label.substr(0,22) + "...";
        drawWifiLine(r, ui.labelFont, label, tile.x+18, tile.y+82, muted);
        if (i == ui.musicBrowser.selectedRow && i < count) {
            SDL_SetRenderDrawColor(r,80,190,255,255);
            for (int n=0; n<5; ++n) {
                SDL_Rect outline={tile.x-n,tile.y-n,tile.w+n*2,tile.h+n*2};
                SDL_RenderDrawRect(r,&outline);
            }
        }
    }
    drawText(r, ui.musicFooter, 360, 625);
    SDL_RenderPresent(r);
}


void drawNowPlaying(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawBattery(ui);

    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};
    Text nowPlaying = makeText(r, ui.labelFont, "NOW PLAYING", white);
    Text indicator = makeText(r, ui.footerFont, "♪  PLAYING", {110, 230, 160, 255});
    drawText(r, nowPlaying, 360, 230);
    drawText(r, ui.musicNowPlayingTitle, 360, 315);
    drawText(r, indicator, 360, 390);

    Text footer = makeText(r, ui.footerFont,
        "B  STOP AND RETURN     START  MENU", muted);
    drawText(r, footer, 360, 625);
    destroyText(nowPlaying);
    destroyText(indicator);
    destroyText(footer);
    SDL_RenderPresent(r);
}
