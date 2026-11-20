// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../UI.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../models/Games.hpp"

void drawUI(UI& ui, int selected) {
    SDL_Renderer* r = ui.renderer;
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderClear(r);

    SDL_Rect panel = {48, 48, 624, 624};
    SDL_SetRenderDrawColor(r, 35, 105, 220, 255);
    SDL_RenderFillRect(r, &panel);
    SDL_Rect inner = {58, 58, 604, 604};
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderFillRect(r, &inner);

    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.subtitle, 360, 119);
    drawBattery(ui);

    const SDL_Rect tiles[4] = {
        {88, 174, 252, 190}, {380, 174, 252, 190},
        {88, 396, 252, 190}, {380, 396, 252, 190}
    };

    for (int i = 0; i < 4; ++i) {
        SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &tile);

        if (ui.images[i]) {
            int iw = 0, ih = 0;
            if (SDL_QueryTexture(ui.images[i], nullptr, nullptr, &iw, &ih) == 0 &&
                iw > 0 && ih > 0) {
                const float scale = std::min(224.0f / iw, 166.0f / ih);
                SDL_Rect image = {
                    tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
                    tile.y + (tile.h - static_cast<int>(ih * scale)) / 2,
                    static_cast<int>(iw * scale), static_cast<int>(ih * scale)
                };
                SDL_RenderCopy(r, ui.images[i], nullptr, &image);
            }
        } else {
            drawText(r, ui.slotLabels[i], tile.x + tile.w / 2,
                     tile.y + tile.h / 2 - ui.slotLabels[i].h / 2);
        }

        if (i == selected) {
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            for (int n = 0; n < 5; ++n) {
                SDL_Rect outline = {tile.x - n, tile.y - n,
                                    tile.w + n * 2, tile.h + n * 2};
                SDL_RenderDrawRect(r, &outline);
            }
        }
    }

    drawText(r, ui.footer, 360, 625);
    SDL_RenderPresent(r);
}

static const char* platformName(const std::string& platform) {
    if (platform == "gb") return "GAME BOY";
    if (platform == "gbc") return "GAME BOY COLOR";
    return "GAME";
}

void updateSelectedGameText(UI& ui, int selected) {
    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};

    destroyText(ui.selectedTitle);
    destroyText(ui.selectedPlatform);
    const bool available = games[selected].available;
    ui.selectedTitle = makeText(
        ui.renderer, ui.labelFont,
        available ? games[selected].name.c_str() : "EMPTY SLOT", white);
    ui.selectedPlatform = makeText(
        ui.renderer, ui.footerFont,
        available ? platformName(games[selected].platform) : "", muted);
}


void refreshGameArtwork(UI& ui) {
    SDL_Texture* nextImages[4] = {};
    Text nextLabels[4];
    const SDL_Color labelColor = {155, 177, 215, 255};

    // Prepare all replacement resources before touching the visible ones.
    for (int i = 0; i < 4; ++i) {
        if (games[i].available && !games[i].image.empty()) {
            nextImages[i] =
                IMG_LoadTexture(ui.renderer, games[i].image.c_str());

            if (!nextImages[i]) {
                std::fprintf(stderr,
                    "KiddOS: refreshed artwork unavailable for %s: %s\n",
                    games[i].name.c_str(), IMG_GetError());
            }
        }

        if (!nextImages[i]) {
            const char* fallback =
                games[i].available ? "NO ARTWORK" : "EMPTY SLOT";
            nextLabels[i] = makeText(
                ui.renderer, ui.footerFont, fallback, labelColor);
        }
    }

    // Swap the complete set on the SDL/UI thread.
    for (int i = 0; i < 4; ++i) {
        if (ui.images[i])
            SDL_DestroyTexture(ui.images[i]);
        destroyText(ui.slotLabels[i]);

        ui.images[i] = nextImages[i];
        ui.slotLabels[i] = nextLabels[i];
    }

    std::puts("KiddOS: game carousel artwork refreshed.");
    std::fflush(stdout);
}

void drawSimplePanel(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderClear(r);
    SDL_Rect panel = {48, 48, 624, 624};
    SDL_SetRenderDrawColor(r, 35, 105, 220, 255);
    SDL_RenderFillRect(r, &panel);
    SDL_Rect inner = {58, 58, 604, 604};
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderFillRect(r, &inner);
}

void drawSelectedGame(UI& ui, int selected, int action) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 78);
    drawText(r, ui.selectedTitle, 360, 133);
    drawBattery(ui);

    if (ui.images[selected]) {
        int iw = 0, ih = 0;
        if (SDL_QueryTexture(ui.images[selected], nullptr, nullptr, &iw, &ih) == 0 &&
            iw > 0 && ih > 0) {
            const float scale = std::min(280.0f / iw, 350.0f / ih);
            SDL_Rect image = {
                88 + (280 - static_cast<int>(iw * scale)) / 2,
                205 + (350 - static_cast<int>(ih * scale)) / 2,
                static_cast<int>(iw * scale), static_cast<int>(ih * scale)
            };
            SDL_RenderCopy(r, ui.images[selected], nullptr, &image);
        }
    } else {
        drawText(r, ui.slotLabels[selected], 228,
                 380 - ui.slotLabels[selected].h / 2);
    }

    const SDL_Rect items[2] = {
        {408, 286, 205, 76},
        {408, 388, 205, 76}
    };

    for (int i = 0; i < 2; ++i) {
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &items[i]);

        if (i == action) {
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            for (int n = 0; n < 5; ++n) {
                SDL_Rect outline = {
                    items[i].x - n, items[i].y - n,
                    items[i].w + n * 2, items[i].h + n * 2
                };
                SDL_RenderDrawRect(r, &outline);
            }
        }
        drawText(r, ui.actionLabels[i], 510, items[i].y + 25);
    }

    drawText(r, ui.selectedFooter, 360, 625);
    SDL_RenderPresent(r);
}

void drawGameInfo(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 92);
    drawText(r, ui.selectedTitle, 360, 235);
    drawText(r, ui.selectedPlatform, 360, 295);
    drawText(r, ui.infoBody, 360, 390);
    drawBattery(ui);
    drawText(r, ui.menuFooter, 360, 625);
    SDL_RenderPresent(r);
}
