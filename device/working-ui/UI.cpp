// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "UI.hpp"


#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include "models/Games.hpp"
#include "services/BatteryService.hpp"
#include "services/WifiService.hpp"
#include "services/ParentManagementService.hpp"
#include "services/MovieCatalog.hpp"

static const char* BASE = "/storage/roms/.kiddos-ui-test-20261116";
static const char* FONT = "/storage/.config/emulationstation/themes/rp-pocket/art/fonts/Cabin-Bold.ttf";

static bool loadHomeIcons(UI& ui) {
    const char* paths[4] = {
        "/storage/roms/.kiddos-ui-assets/games.png",
        "/storage/roms/.kiddos-ui-assets/music.png",
        "/storage/roms/.kiddos-ui-assets/movies.png",
        "/storage/roms/.kiddos-ui-assets/tv-shows.png"
    };

    for (int i = 0; i < 4; ++i) {
        ui.homeIcons[i] = IMG_LoadTexture(ui.renderer, paths[i]);
        if (!ui.homeIcons[i]) {
            std::fprintf(stderr, "KiddOS: Home icon %s failed to load: %s\n",
                         paths[i], IMG_GetError());
            return false;
        }
        SDL_SetTextureColorMod(ui.homeIcons[i], 80, 190, 255);
    }
    std::fprintf(stderr, "KiddOS: All four CC0 home icons loaded\n");
    return true;
}

Text makeText(SDL_Renderer* r, TTF_Font* f, const char* value, SDL_Color color) {
    Text t;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, value, color);
    if (!s) {
        std::fprintf(stderr, "KiddOS: text render failed: %s\n", TTF_GetError());
        return t;
    }
    t.w = s->w;
    t.h = s->h;
    t.texture = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    return t;
}

void drawText(SDL_Renderer* r, const Text& t, int cx, int y) {
    if (!t.texture) return;
    SDL_Rect dst = {cx - t.w / 2, y, t.w, t.h};
    SDL_RenderCopy(r, t.texture, nullptr, &dst);
}

void drawWifiLine(SDL_Renderer* r, TTF_Font* font,
                         const std::string& value, int x, int y,
                         SDL_Color color) {
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended(font, value.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(r, surface);
    if (texture) {
        SDL_Rect destination = {x, y, surface->w, surface->h};
        SDL_RenderCopy(r, texture, nullptr, &destination);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void destroyText(Text& t);
void updateWifiStatus(UI& ui) {
    const KiddosServices::WifiReading reading =
        KiddosServices::readWifiStatus();

    ui.wifi.status = reading.status;
    ui.wifi.powerKnown = reading.powerKnown;
    if (reading.powerKnown)
        ui.wifi.powered = reading.powered;
}

bool loadWifiNetworks(UI& ui, bool scan) {
    char helperPath[512];
    std::snprintf(helperPath, sizeof(helperPath),
                  "%s/wifi_helper.py", BASE);

    const KiddosServices::WifiScanResult result =
        KiddosServices::loadWifiNetworks(helperPath, scan);

    if (!result.success()) {
        switch (result.error) {
            case KiddosServices::WifiScanError::StartFailed:
                ui.wifi.listMessage = "COULD NOT START WI-FI HELPER";
                break;
            case KiddosServices::WifiScanError::HelperFailed:
                ui.wifi.listMessage = "WI-FI HELPER FAILED";
                break;
            case KiddosServices::WifiScanError::ScanFailed:
                ui.wifi.listMessage = "WI-FI SCAN FAILED";
                break;
            case KiddosServices::WifiScanError::None:
                break;
        }
        return false;
    }

    ui.wifi.networks = result.networks;
    ui.wifi.selectedRow = 0;
    ui.wifi.scrollOffset = 0;
    ui.wifi.listMessage = ui.wifi.networks.empty()
        ? "NO NETWORKS FOUND"
        : "";

    return true;
}

void startWifiConnection(UI& ui, const WifiNetwork& network,
                                const std::string& password) {
    ui.wifi.needsPassword = false;

    char helperPath[512];
    std::snprintf(helperPath, sizeof(helperPath),
                  "%s/wifi_helper.py", BASE);

    if (!KiddosServices::startWifiConnection(
            ui.wifi.connection, helperPath, network, password)) {
        ui.wifi.listMessage = "COULD NOT START CONNECTION";
        return;
    }

    ui.wifi.listMessage = "CONNECTING...";
}

void refreshWifiMenuText(UI& ui);

void pollWifiConnection(UI& ui) {
    const KiddosServices::WifiConnectionResult result =
        KiddosServices::pollWifiConnection(ui.wifi.connection);

    if (result == KiddosServices::WifiConnectionResult::Running ||
        result == KiddosServices::WifiConnectionResult::Idle)
        return;

    if (result == KiddosServices::WifiConnectionResult::NeedsPassword) {
        ui.wifi.needsPassword = true;
        ui.wifi.listMessage = "PASSWORD REQUIRED";
        return;
    }

    const int selectedRow = ui.wifi.selectedRow;
    const int scrollOffset = ui.wifi.scrollOffset;

    updateWifiStatus(ui);
    refreshWifiMenuText(ui);

    char helperPath[512];
    std::snprintf(helperPath, sizeof(helperPath),
                  "%s/wifi_helper.py", BASE);

    if (loadWifiNetworks(ui, false)) {
        const int itemCount = 2 + static_cast<int>(ui.wifi.networks.size());
        constexpr int visibleRows = 8;

        ui.wifi.selectedRow = std::min(
            selectedRow, std::max(0, itemCount - 1));
        ui.wifi.scrollOffset = std::min(
            scrollOffset, std::max(0, itemCount - visibleRows));

        if (ui.wifi.selectedRow < ui.wifi.scrollOffset)
            ui.wifi.scrollOffset = ui.wifi.selectedRow;
        if (ui.wifi.selectedRow >= ui.wifi.scrollOffset + visibleRows)
            ui.wifi.scrollOffset =
                ui.wifi.selectedRow - visibleRows + 1;
    }

    if (result == KiddosServices::WifiConnectionResult::Connected)
        ui.wifi.listMessage = "CONNECTED";
    else
        ui.wifi.listMessage = "CONNECTION FAILED - CHECK PASSWORD";
}

void cancelWifiConnection(UI& ui) {
    KiddosServices::cancelWifiConnection(ui.wifi.connection);
}

void moveWifiSelection(UI& ui, int direction) {
    const int itemCount = 2 + static_cast<int>(ui.wifi.networks.size());
    ui.wifi.selectedRow += direction;

    if (ui.wifi.selectedRow < 0)
        ui.wifi.selectedRow = 0;
    if (ui.wifi.selectedRow >= itemCount)
        ui.wifi.selectedRow = itemCount - 1;

    constexpr int visibleRows = 8;
    if (ui.wifi.selectedRow < ui.wifi.scrollOffset)
        ui.wifi.scrollOffset = ui.wifi.selectedRow;
    if (ui.wifi.selectedRow >= ui.wifi.scrollOffset + visibleRows)
        ui.wifi.scrollOffset = ui.wifi.selectedRow - visibleRows + 1;
}

void refreshWifiMenuText(UI& ui) {
    const SDL_Color white = {245, 247, 255, 255};
    const SDL_Color muted = {190, 195, 210, 255};
    const SDL_Color green = {110, 230, 160, 255};
    const SDL_Color red = {255, 115, 115, 255};

    destroyText(ui.wifiPowerLabel);
    destroyText(ui.wifiConnectionLabel);
    destroyText(ui.wifiActionLabel);

    const char* power = !ui.wifi.powerKnown ? "WI-FI POWER: UNKNOWN" :
                        ui.wifi.powered ? "WI-FI POWER: ON" :
                                         "WI-FI POWER: OFF";
    const char* connection =
        ui.wifi.status == WifiStatus::Connected ? "CONNECTION: CONNECTED" :
        ui.wifi.status == WifiStatus::Disconnected ? "CONNECTION: NOT CONNECTED" :
                                                    "CONNECTION: UNKNOWN";
    const char* action = ui.wifi.commandError ? "WI-FI CHANGE FAILED" :
                         !ui.wifi.powerKnown ? "WI-FI POWER UNAVAILABLE" :
                         ui.wifi.powered ? "A  TURN WI-FI OFF" :
                                          "A  TURN WI-FI ON";

    ui.wifiPowerLabel = makeText(ui.renderer, ui.labelFont, power,
        !ui.wifi.powerKnown ? muted : ui.wifi.powered ? green : muted);
    ui.wifiConnectionLabel = makeText(ui.renderer, ui.labelFont,
        connection, ui.wifi.status == WifiStatus::Connected ? green : muted);
    ui.wifiActionLabel = makeText(ui.renderer, ui.labelFont, action,
        ui.wifi.commandError ? red : white);
}

void refreshWifiSettings(UI& ui) {
    updateWifiStatus(ui);
    refreshWifiMenuText(ui);
}

void toggleWifiPower(UI& ui) {
    if (!ui.wifi.powerKnown) {
        ui.wifi.commandError = true;
        refreshWifiMenuText(ui);
        return;
    }

    const bool requestedPower = !ui.wifi.powered;
    ui.wifi.commandError = !KiddosServices::setWifiPower(requestedPower);

    // ConnMan updates its reported technology state asynchronously.
    for (int attempt = 0; attempt < 10; ++attempt) {
        SDL_Delay(100);
        updateWifiStatus(ui);
        if (ui.wifi.powerKnown &&
            ui.wifi.powered == requestedPower) {
            break;
        }
    }

    ui.status.lastPoll = 0;
    updateWifiStatus(ui);
    refreshWifiMenuText(ui);
}

void updateBattery(UI& ui) {
    Uint32 now = SDL_GetTicks();
    if (ui.status.lastPoll != 0 && now - ui.status.lastPoll < 5000)
        return;
    ui.status.lastPoll = now;
    updateWifiStatus(ui);

    const KiddosServices::BatteryReading reading =
        KiddosServices::readBattery();
    if (reading.capacityValid)
        ui.status.batteryPercent = reading.percent;
    ui.status.externalPower = reading.externalPower;

    destroyText(ui.batteryLabel);
    if (ui.status.batteryPercent >= 0) {
        char label[16];
        std::snprintf(label, sizeof(label), "%d%%", ui.status.batteryPercent);
        SDL_Color color = ui.status.batteryPercent <= 15
            ? SDL_Color{255, 90, 90, 255}
            : SDL_Color{245, 248, 255, 255};
        ui.batteryLabel = makeText(
            ui.renderer, ui.footerFont, label, color);
    }
}

static void drawWifiStatus(UI& ui);

void drawBattery(UI& ui) {
    drawWifiStatus(ui);
    if (ui.status.batteryPercent < 0) return;

    SDL_Renderer* r = ui.renderer;
    const int x = 574;
    const int y = 78;

    SDL_Color color = ui.status.batteryPercent <= 15
        ? SDL_Color{255, 90, 90, 255}
        : SDL_Color{155, 177, 215, 255};

    SDL_Rect outline = {x, y, 27, 14};
    SDL_Rect cap = {x + 27, y + 4, 3, 6};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderDrawRect(r, &outline);
    SDL_RenderFillRect(r, &cap);

    int fillWidth = (ui.status.batteryPercent * 21) / 100;
    if (fillWidth > 0) {
        SDL_Rect fill = {x + 3, y + 3, fillWidth, 8};
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(r, &fill);
    }

    if (ui.status.externalPower) {
        // Bright, larger charging bolt for visibility at a glance.
        SDL_SetRenderDrawColor(r, 255, 225, 70, 255);
        SDL_RenderDrawLine(r, x + 17, y + 1, x + 10, y + 8);
        SDL_RenderDrawLine(r, x + 10, y + 8, x + 15, y + 8);
        SDL_RenderDrawLine(r, x + 15, y + 8, x + 10, y + 14);
        SDL_RenderDrawLine(r, x + 18, y + 1, x + 11, y + 8);
    }

    if (ui.batteryLabel.texture) {
        SDL_Rect dst = {
            615, y - 2, ui.batteryLabel.w, ui.batteryLabel.h
        };
        SDL_RenderCopy(r, ui.batteryLabel.texture, nullptr, &dst);
    }
}

static void drawWifiStatus(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    SDL_Color color = {155, 177, 215, 255};

    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    // Compact Wi-Fi indicator matching the battery's blue-gray styling.
    const SDL_Point outer[] = {
        {544, 82}, {548, 79}, {553, 78}, {558, 79}, {562, 82}
    };
    const SDL_Point middle[] = {
        {548, 86}, {552, 83}, {555, 83}, {559, 86}
    };
    const SDL_Point inner[] = {
        {551, 90}, {553, 88}, {556, 90}
    };

    SDL_RenderDrawLines(r, outer, sizeof(outer) / sizeof(outer[0]));
    SDL_RenderDrawLines(r, middle, sizeof(middle) / sizeof(middle[0]));
    SDL_RenderDrawLines(r, inner, sizeof(inner) / sizeof(inner[0]));

    SDL_Rect dot = {553, 92, 3, 3};
    SDL_RenderFillRect(r, &dot);

    if (ui.wifi.status == WifiStatus::Disconnected) {
        SDL_SetRenderDrawColor(r, 255, 90, 90, 255);
        SDL_RenderDrawLine(r, 544, 78, 562, 95);
    }
}

void destroyText(Text& t) {
    if (t.texture) SDL_DestroyTexture(t.texture);
    t.texture = nullptr;
}

static void setParentMessage(UI& ui, const char* message, SDL_Color color) {
    destroyText(ui.parentBody);
    ui.parentBody = makeText(ui.renderer, ui.labelFont, message, color);
}

bool refreshParentStatus(UI& ui) {
    char result[32];
    if (!KiddosServices::managementControl("status", result, sizeof(result))) {
        setParentMessage(ui, "REMOTE MANAGEMENT: ERROR",
                         {255, 115, 115, 255});
        return false;
    }

    result[std::strcspn(result, "\r\n")] = '\0';
    if (std::strcmp(result, "running") == 0) {
        setParentMessage(ui, "REMOTE MANAGEMENT: ON",
                         {110, 230, 160, 255});
        return true;
    }

    if (std::strcmp(result, "stopped") == 0) {
        setParentMessage(ui, "REMOTE MANAGEMENT: OFF",
                         {190, 195, 210, 255});
        return false;
    }

    setParentMessage(ui, "REMOTE MANAGEMENT: ERROR",
                     {255, 115, 115, 255});
    return false;
}

void toggleParentManagement(UI& ui) {
    char result[32];
    if (!KiddosServices::managementControl("status", result, sizeof(result))) {
        setParentMessage(ui, "REMOTE MANAGEMENT: ERROR",
                         {255, 115, 115, 255});
        return;
    }

    result[std::strcspn(result, "\r\n")] = '\0';
    const bool wasRunning = std::strcmp(result, "running") == 0;

    if (!wasRunning && std::strcmp(result, "stopped") != 0) {
        setParentMessage(ui, "REMOTE MANAGEMENT: ERROR",
                         {255, 115, 115, 255});
        return;
    }

    if (!KiddosServices::managementControl(wasRunning ? "stop" : "start",
                           result, sizeof(result))) {
        setParentMessage(ui, "REMOTE MANAGEMENT: ERROR",
                         {255, 115, 115, 255});
        return;
    }

    if (wasRunning) {
        refreshParentStatus(ui);
        return;
    }

    for (int attempt = 0; attempt < 20; ++attempt) {
        SDL_Delay(100);
        char statusResult[32];
        if (KiddosServices::managementControl("status", statusResult, sizeof(statusResult))) {
            statusResult[std::strcspn(statusResult, "\r\n")] = '\0';
            if (std::strcmp(statusResult, "running") == 0) {
                refreshParentStatus(ui);
                return;
            }
        }
    }

    setParentMessage(ui, "REMOTE MANAGEMENT: STARTING",
                     {255, 210, 110, 255});
}

void closeUI(UI& ui) {
    destroyText(ui.heading);
    destroyText(ui.homeSubtitle);
    destroyText(ui.movieSubtitle);
    destroyText(ui.movieFooter);
    destroyText(ui.musicSubtitle);
    destroyText(ui.musicFooter);
    for (int i = 0; i < 4; ++i) destroyText(ui.homeLabels[i]);
    for (int i = 0; i < 2; ++i) destroyText(ui.homeComingSoon[i]);
    destroyText(ui.subtitle);
    destroyText(ui.footer);
    destroyText(ui.selectedFooter);
    destroyText(ui.selectedTitle);
    destroyText(ui.selectedPlatform);
    destroyText(ui.infoBody);
    for (int i = 0; i < 2; ++i) destroyText(ui.actionLabels[i]);
    destroyText(ui.menuHeading);
    for (int i = 0; i < 2; ++i) destroyText(ui.menuItems[i]);
    destroyText(ui.menuFooter);
    destroyText(ui.confirmHeading);
    destroyText(ui.confirmBody);
    destroyText(ui.confirmFooter);
    destroyText(ui.pinPrompt);
    destroyText(ui.pinFooter);
    destroyText(ui.pinError);
    destroyText(ui.parentHeading);
    destroyText(ui.parentBody);
    destroyText(ui.parentWifi);
    destroyText(ui.parentChangePin);
    destroyText(ui.parentFooter);
    destroyText(ui.wifiHeading);
    destroyText(ui.wifiPowerLabel);
    destroyText(ui.wifiConnectionLabel);
    destroyText(ui.wifiActionLabel);
    destroyText(ui.wifiFooter);
    destroyText(ui.batteryLabel);
    for (int i = 0; i < 4; ++i) {
        destroyText(ui.slotLabels[i]);
        if (ui.images[i]) SDL_DestroyTexture(ui.images[i]);
        if (ui.homeIcons[i]) SDL_DestroyTexture(ui.homeIcons[i]);
        if (ui.movieImages[i]) SDL_DestroyTexture(ui.movieImages[i]);
    }
    if (ui.headingFont) TTF_CloseFont(ui.headingFont);
    if (ui.labelFont) TTF_CloseFont(ui.labelFont);
    if (ui.footerFont) TTF_CloseFont(ui.footerFont);
    if (ui.renderer) SDL_DestroyRenderer(ui.renderer);
    if (ui.window) SDL_DestroyWindow(ui.window);
    if (ui.joystick) SDL_JoystickClose(ui.joystick);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    ui = {};
}

bool openUI(UI& ui) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "KiddOS: SDL init failed: %s\n", SDL_GetError());
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::fprintf(stderr, "KiddOS: PNG init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return false;
    }
    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        std::fprintf(stderr, "KiddOS: JPEG artwork unavailable: %s\n", IMG_GetError());
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "KiddOS: TTF init failed: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    SDL_JoystickEventState(SDL_ENABLE);
    if (SDL_NumJoysticks() > 0) {
        ui.joystick = SDL_JoystickOpen(0);
        if (ui.joystick)
            std::printf("KiddOS: controller: %s\n", SDL_JoystickName(ui.joystick));
    }

    ui.window = SDL_CreateWindow("Retro KiddOS", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 720, 720, SDL_WINDOW_SHOWN);
    if (!ui.window) {
        std::fprintf(stderr, "KiddOS: window failed: %s\n", SDL_GetError());
        closeUI(ui);
        return false;
    }

    ui.renderer = SDL_CreateRenderer(ui.window, -1, SDL_RENDERER_ACCELERATED);
    if (!ui.renderer)
        ui.renderer = SDL_CreateRenderer(ui.window, -1, SDL_RENDERER_SOFTWARE);
    if (!ui.renderer) {
        std::fprintf(stderr, "KiddOS: renderer failed: %s\n", SDL_GetError());
        closeUI(ui);
        return false;
    }

    if (!loadHomeIcons(ui)) {
        closeUI(ui);
        return false;
    }

    ui.headingFont = TTF_OpenFont(FONT, 38);
    ui.labelFont = TTF_OpenFont(FONT, 20);
    ui.footerFont = TTF_OpenFont(FONT, 17);
    if (!ui.headingFont || !ui.labelFont || !ui.footerFont) {
        std::fprintf(stderr, "KiddOS: font open failed: %s\n", TTF_GetError());
        closeUI(ui);
        return false;
    }

    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};
    ui.heading = makeText(ui.renderer, ui.headingFont, "Retro KiddOS", white);
    ui.subtitle = makeText(ui.renderer, ui.footerFont, "CHOOSE A GAME", muted);
    ui.movieSubtitle = makeText(ui.renderer, ui.footerFont, "CHOOSE A MOVIE", muted);
    ui.movieFooter = makeText(ui.renderer, ui.footerFont,
        "D-PAD MOVE   A PLAY   B BACK   START MENU", muted);
    ui.musicSubtitle = makeText(ui.renderer, ui.footerFont, "CHOOSE A SONG", muted);
    ui.musicFooter = makeText(ui.renderer, ui.footerFont,
        "D-PAD MOVE   A PLAY   B BACK   START MENU", muted);
    ui.homeSubtitle = makeText(ui.renderer, ui.footerFont,
        "CHOOSE SOMETHING TO PLAY", muted);
    ui.homeLabels[0] = makeText(ui.renderer, ui.labelFont, "GAMES", white);
    ui.homeLabels[1] = makeText(ui.renderer, ui.labelFont, "MUSIC", white);
    ui.homeLabels[2] = makeText(ui.renderer, ui.labelFont, "MOVIES", white);
    ui.homeLabels[3] = makeText(ui.renderer, ui.labelFont, "TV SHOWS", white);
    ui.homeComingSoon[0] = makeText(ui.renderer, ui.footerFont,
        "", muted);
    ui.homeComingSoon[1] = makeText(ui.renderer, ui.footerFont,
        "COMING SOON", muted);
    ui.footer = makeText(ui.renderer, ui.footerFont,
        "D-PAD  MOVE     A  SELECT     START  MENU", muted);
    ui.selectedFooter = makeText(ui.renderer, ui.footerFont,
        "D-PAD MOVE   A SELECT   B BACK   START MENU", muted);
    ui.actionLabels[0] = makeText(ui.renderer, ui.labelFont, "PLAY", white);
    ui.actionLabels[1] = makeText(ui.renderer, ui.labelFont, "GAME INFO", white);
    ui.infoBody = makeText(ui.renderer, ui.labelFont,
        "Choose Play to start this game.", muted);

    ui.menuHeading = makeText(ui.renderer, ui.headingFont, "SYSTEM MENU", white);
    ui.menuItems[0] = makeText(ui.renderer, ui.labelFont, "Parent Mode", white);
    ui.menuItems[1] = makeText(ui.renderer, ui.labelFont, "Shut Down", white);
    ui.menuFooter = makeText(ui.renderer, ui.footerFont, "A  SELECT     B  BACK", muted);
    ui.confirmHeading = makeText(ui.renderer, ui.headingFont, "SHUT DOWN?", white);
    ui.confirmBody = makeText(ui.renderer, ui.labelFont,
        "Are you sure you want to turn off the system?", white);
    ui.confirmFooter = makeText(ui.renderer, ui.footerFont,
        "A  YES     B  CANCEL", muted);
    ui.pinPrompt = makeText(ui.renderer, ui.labelFont,
        "ENTER PARENT PIN", white);
    ui.pinFooter = makeText(ui.renderer, ui.footerFont,
        "D-PAD  EDIT     A  UNLOCK     B  BACK", muted);
    ui.pinError = makeText(ui.renderer, ui.footerFont,
        "INCORRECT PIN - TRY AGAIN", {255, 115, 115, 255});
    ui.parentHeading = makeText(ui.renderer, ui.headingFont,
        "PARENT MODE", white);
    ui.parentBody = makeText(ui.renderer, ui.labelFont,
        "REMOTE MANAGEMENT: UNKNOWN", muted);
    ui.parentWifi = makeText(ui.renderer, ui.labelFont,
        "WI-FI SETTINGS", white);
    ui.parentChangePin = makeText(ui.renderer, ui.labelFont,
        "CHANGE PARENT PIN", white);
    ui.parentFooter = makeText(ui.renderer, ui.footerFont,
        "UP/DOWN MOVE   A SELECT   B BACK", muted);
    ui.wifiHeading = makeText(ui.renderer, ui.headingFont,
        "WI-FI SETTINGS", white);
    ui.wifiPowerLabel = makeText(ui.renderer, ui.labelFont,
        "WI-FI POWER: UNKNOWN", muted);
    ui.wifiConnectionLabel = makeText(ui.renderer, ui.labelFont,
        "CONNECTION: UNKNOWN", muted);
    ui.wifiActionLabel = makeText(ui.renderer, ui.labelFont,
        "WI-FI POWER UNAVAILABLE", muted);
    ui.wifiFooter = makeText(ui.renderer, ui.footerFont,
        "UP/DOWN MOVE   A SELECT   B BACK", muted);

    const SDL_Color slotLabelColor = {155, 177, 215, 255};
    for (int i = 0; i < 4; ++i) {
        if (games[i].available && !games[i].image.empty()) {
            ui.images[i] = IMG_LoadTexture(ui.renderer, games[i].image.c_str());
            if (!ui.images[i]) {
                std::fprintf(stderr, "KiddOS: artwork unavailable for %s: %s\n",
                             games[i].name.c_str(), IMG_GetError());
            }
        }
        const char* fallback = games[i].available ? "NO ARTWORK" : "EMPTY SLOT";
        if (!ui.images[i]) {
            ui.slotLabels[i] = makeText(
                ui.renderer, ui.footerFont, fallback, slotLabelColor);
        }
    }
    const auto& movieList = KiddosServices::movies();
    for (int i = 0; i < 4 && i < static_cast<int>(movieList.size()); ++i) {
        std::string stem = movieList[static_cast<std::size_t>(i)].path;
        const std::size_t extension = stem.find_last_of('.');
        if (extension != std::string::npos)
            stem.resize(extension);

        const std::string candidates[] = {stem + ".jpg", stem + ".png"};
        for (const auto& candidate : candidates) {
            ui.movieImages[i] = IMG_LoadTexture(ui.renderer, candidate.c_str());
            if (ui.movieImages[i]) break;
        }

        // The initial library contains Totoro; support its fixed artwork name
        // even when the movie file itself has a longer filename.
        if (!ui.movieImages[i] && i == 0)
            ui.movieImages[i] =
                IMG_LoadTexture(ui.renderer, "/storage/roms/movies/Totoro.jpg");

        if (ui.movieImages[i]) {
            std::printf("KiddOS: movie artwork loaded for %s\\n",
                        movieList[static_cast<std::size_t>(i)].title.c_str());
        } else {
            std::fprintf(stderr, "KiddOS: movie artwork unavailable for %s: %s\\n",
                         movieList[static_cast<std::size_t>(i)].title.c_str(),
                         IMG_GetError());
        }
    }

    std::puts("KiddOS: game artwork loading complete.");
    std::fflush(stdout);
    return true;
}
