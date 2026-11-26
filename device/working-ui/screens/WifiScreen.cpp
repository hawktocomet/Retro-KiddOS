// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../ui/UI.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../services/WifiService.hpp"

void drawWifiSettings(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 85);
    drawText(r, ui.wifiHeading, 360, 155);
    drawText(r, ui.wifiConnectionLabel, 360, 220);

    const SDL_Color normal = {235, 240, 250, 255};
    const SDL_Color muted = {165, 180, 200, 255};
    const int rowHeight = 32;
    constexpr int visibleRows = 8;

    for (int visible = 0; visible < visibleRows; ++visible) {
        const int index = ui.wifi.scrollOffset + visible;
        const int itemCount = 2 + static_cast<int>(ui.wifi.networks.size());
        if (index >= itemCount) break;

        SDL_Rect row = {78, 250 + visible * rowHeight, 564, 28};

        if (index == ui.wifi.selectedRow) {
            SDL_SetRenderDrawColor(r, 65, 165, 235, 255);
            for (int n = 0; n < 2; ++n) {
                SDL_Rect outline = {
                    row.x - n, row.y - n,
                    row.w + n * 2, row.h + n * 2
                };
                SDL_RenderDrawRect(r, &outline);
            }
        }

        std::string label;
        if (index == 0) {
            label = std::string("WI-FI POWER: ") +
                    (ui.wifi.powered ? "ON" : "OFF");
        } else if (index == 1) {
            label = "SCAN FOR NETWORKS";
        } else {
            const WifiNetwork& network = ui.wifi.networks[index - 2];
            label = network.name + "   " +
                    std::to_string(network.strength) + "%   " +
                    (network.security == "open" ? "OPEN" : "SECURED");
            if (network.state == "online")
                label += "   CONNECTED";
        }

        drawWifiLine(r, ui.labelFont, label, row.x + 14, row.y + 5,
                     index == ui.wifi.selectedRow ? normal : muted);
    }

    if (!ui.wifi.listMessage.empty()) {
        int messageWidth = 0;
        TTF_SizeUTF8(ui.footerFont, ui.wifi.listMessage.c_str(),
                     &messageWidth, nullptr);
        drawWifiLine(r, ui.footerFont, ui.wifi.listMessage,
                     (720 - messageWidth) / 2, 540, muted);
    }
    drawBattery(ui);
    drawText(r, ui.wifiFooter, 360, 625);
    SDL_RenderPresent(r);
}

static std::vector<std::vector<std::string>> wifiKeyboardRows(const UI& ui) {
    std::vector<std::vector<std::string>> rows;
    if (ui.wifi.keyboardSymbols) {
        rows = {
            {"1","2","3","4","5","6","7","8","9","0"},
            {"@","#","$","%","&","-","+","(",")"},
            {"!","?",":",";","'","\"",",",".","/"}
        };
    } else {
        rows = {
            {"q","w","e","r","t","y","u","i","o","p"},
            {"a","s","d","f","g","h","j","k","l"},
            {"z","x","c","v","b","n","m"}
        };
        if (ui.wifi.keyboardShift) {
            for (auto& row : rows)
                for (auto& key : row)
                    for (char& ch : key)
                        ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    rows.push_back({"SHIFT","123","SPACE","BKSP","CLEAR","CONNECT"});
    return rows;
}

void moveWifiKeyboard(UI& ui, int dx, int dy) {
    const auto rows = wifiKeyboardRows(ui);
    ui.wifi.keyboardRow =
        (ui.wifi.keyboardRow + dy + static_cast<int>(rows.size())) %
        static_cast<int>(rows.size());
    const int count = static_cast<int>(
        rows[static_cast<size_t>(ui.wifi.keyboardRow)].size());
    ui.wifi.keyboardCol =
        std::max(0, std::min(ui.wifi.keyboardCol + dx, count - 1));
}

bool activateWifiKeyboardKey(UI& ui) {
    const auto rows = wifiKeyboardRows(ui);
    const auto& key = rows[static_cast<size_t>(ui.wifi.keyboardRow)]
                           [static_cast<size_t>(ui.wifi.keyboardCol)];

    if (ui.wifi.keyboardRow == 3) {
        if (key == "SHIFT") {
            ui.wifi.keyboardShift = !ui.wifi.keyboardShift;
        } else if (key == "123") {
            ui.wifi.keyboardSymbols = !ui.wifi.keyboardSymbols;
            ui.wifi.keyboardRow = ui.wifi.keyboardCol = 0;
        } else if (key == "SPACE") {
            if (ui.wifi.password.size() < 63) ui.wifi.password.push_back(' ');
        } else if (key == "BKSP") {
            if (!ui.wifi.password.empty()) ui.wifi.password.pop_back();
        } else if (key == "CLEAR") {
            std::fill(ui.wifi.password.begin(), ui.wifi.password.end(), '\0');
            ui.wifi.password.clear();
        } else if (key == "CONNECT") {
            if (!ui.wifi.password.empty() && ui.wifi.selectedRow >= 2 &&
                ui.wifi.selectedRow - 2 < static_cast<int>(ui.wifi.networks.size())) {
                const auto& network =
                    ui.wifi.networks[static_cast<size_t>(ui.wifi.selectedRow - 2)];
                startWifiConnection(ui, network, ui.wifi.password);
                std::fill(ui.wifi.password.begin(), ui.wifi.password.end(), '\0');
                ui.wifi.password.clear();
                ui.wifi.passwordCursor = 0;
                return true;
            }
        }
    } else if (ui.wifi.password.size() < 63 && !key.empty()) {
        ui.wifi.password.push_back(key[0]);
        if (ui.wifi.keyboardShift && !ui.wifi.keyboardSymbols)
            ui.wifi.keyboardShift = false;
    }

    ui.wifi.passwordCursor = static_cast<int>(ui.wifi.password.size());
    return false;
}

void drawWifiPassword(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);

    auto centered = [&](TTF_Font* font, const std::string& value,
                        int y, SDL_Color color) {
        int w = 0, h = 0;
        if (TTF_SizeUTF8(font, value.c_str(), &w, &h) == 0)
            drawWifiLine(r, font, value, (720 - w) / 2, y, color);
    };

    const SDL_Color white = {245,248,255,255};
    const SDL_Color muted = {155,177,215,255};
    const SDL_Color blue = {80,190,255,255};
    const SDL_Color selected = {35,105,175,255};
    const SDL_Color fill = {22,38,65,255};

    centered(ui.headingFont, "WI-FI PASSWORD", 82, white);
    std::string masked(ui.wifi.password.size(), '*');
    centered(ui.headingFont,
             masked.empty() ? "(ENTER PASSWORD)" : masked, 137, white);

    const auto rows = wifiKeyboardRows(ui);
    for (size_t row = 0; row < rows.size(); ++row) {
        const bool actions = row == 3;
        const int kw = actions ? 94 : 48;
        const int kh = 46;
        const int gap = 7;
        const int total = static_cast<int>(rows[row].size()) * kw +
                          (static_cast<int>(rows[row].size()) - 1) * gap;
        const int x0 = (720 - total) / 2;
        const int y = actions ? 430 : 205 + static_cast<int>(row) * 65;

        for (size_t col = 0; col < rows[row].size(); ++col) {
            SDL_Rect rect{x0 + static_cast<int>(col) * (kw + gap), y, kw, kh};
            const bool active =
                static_cast<int>(row) == ui.wifi.keyboardRow &&
                static_cast<int>(col) == ui.wifi.keyboardCol;

            SDL_SetRenderDrawColor(r,
                active ? selected.r : fill.r,
                active ? selected.g : fill.g,
                active ? selected.b : fill.b, 255);
            SDL_RenderFillRect(r, &rect);
            SDL_SetRenderDrawColor(r, blue.r, blue.g, blue.b,
                                   active ? 255 : 100);
            SDL_RenderDrawRect(r, &rect);

            const auto& label = rows[row][col];
            TTF_Font* font = actions ? ui.footerFont : ui.labelFont;
            int tw = 0, th = 0;
            if (TTF_SizeUTF8(font, label.c_str(), &tw, &th) == 0)
                drawWifiLine(r, font, label,
                    rect.x + (rect.w - tw) / 2,
                    rect.y + (rect.h - th) / 2, white);
        }
    }

    centered(ui.footerFont,
        "D-PAD MOVE   A SELECT   B DELETE/BACK   START CONNECT",
        555, muted);
    drawBattery(ui);
    SDL_RenderPresent(r);
}
