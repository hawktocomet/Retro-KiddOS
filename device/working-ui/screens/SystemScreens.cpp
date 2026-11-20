// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../UI.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

void drawSystemMenu(UI& ui, int selected) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.menuHeading, 360, 245);
    drawBattery(ui);

    for (int i = 0; i < 2; ++i) {
        SDL_Rect item = {170, 320 + i * 105, 380, 82};
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &item);
        if (i == selected) {
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            for (int n = 0; n < 4; ++n) {
                SDL_Rect outline = {item.x - n, item.y - n,
                                    item.w + n * 2, item.h + n * 2};
                SDL_RenderDrawRect(r, &outline);
            }
        }
        drawText(r, ui.menuItems[i], 360, item.y + 25);
    }
    drawText(r, ui.menuFooter, 360, 625);
    SDL_RenderPresent(r);
}

void drawShutdownConfirm(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.confirmHeading, 360, 245);
    drawBattery(ui);
    drawText(r, ui.confirmBody, 360, 345);
    drawText(r, ui.confirmFooter, 360, 625);
    SDL_RenderPresent(r);
}

void drawPinEntry(UI& ui, const char* digits, int position, bool error) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.pinPrompt, 360, 245);
    drawBattery(ui);

    for (int i = 0; i < 4; ++i) {
        SDL_Rect box = {190 + i * 90, 330, 70, 90};
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &box);

        if (i == position) {
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            for (int n = 0; n < 4; ++n) {
                SDL_Rect outline = {box.x - n, box.y - n,
                                    box.w + n * 2, box.h + n * 2};
                SDL_RenderDrawRect(r, &outline);
            }
        }

        char digitText[2] = {digits[i], '\0'};
        SDL_Color digitColor = {245, 248, 255, 255};
        SDL_Surface* digitSurface =
            TTF_RenderUTF8_Blended(ui.labelFont, digitText, digitColor);
        if (digitSurface) {
            SDL_Texture* digitTexture =
                SDL_CreateTextureFromSurface(r, digitSurface);
            if (digitTexture) {
                SDL_Rect digitRect = {
                    box.x + (box.w - digitSurface->w) / 2,
                    box.y + (box.h - digitSurface->h) / 2,
                    digitSurface->w,
                    digitSurface->h
                };
                SDL_RenderCopy(r, digitTexture, nullptr, &digitRect);
                SDL_DestroyTexture(digitTexture);
            }
            SDL_FreeSurface(digitSurface);
        }
    }

    if (error) drawText(r, ui.pinError, 360, 485);
    drawText(r, ui.pinFooter, 360, 625);
    SDL_RenderPresent(r);
}
