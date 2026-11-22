// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <string>

#include "screens/ScreenState.hpp"

struct Text {
    SDL_Texture* texture = nullptr;
    int w = 0;
    int h = 0;
};

// Owns SDL resources and the screen-specific state required by the renderer.
struct UI {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_Texture* images[4] = {};
    SDL_Texture* homeIcons[4] = {};
    SDL_Texture* movieImages[4] = {};
    Text slotLabels[4];
    Text homeLabels[4];
    Text homeComingSoon[2];
    Text homeSubtitle, movieSubtitle, movieFooter;
    TTF_Font* headingFont = nullptr;
    TTF_Font* labelFont = nullptr;
    TTF_Font* footerFont = nullptr;

    Text heading, subtitle, footer, selectedFooter;
    Text selectedTitle, selectedPlatform, actionLabels[2], infoBody;
    Text menuHeading, menuItems[2], menuFooter;
    Text confirmHeading, confirmBody, confirmFooter;
    Text pinPrompt, pinFooter, pinError;
    Text parentHeading, parentBody, parentWifi, parentChangePin, parentFooter;
    Text wifiHeading, wifiPowerLabel, wifiConnectionLabel;
    Text wifiActionLabel, wifiFooter;
    Text batteryLabel;

    KiddosScreens::MovieBrowserState movieBrowser;
    KiddosScreens::WifiState wifi;
    KiddosScreens::StatusBarState status;
};

Text makeText(SDL_Renderer* renderer, TTF_Font* font,
              const char* value, SDL_Color color);
void destroyText(Text& text);
void drawText(SDL_Renderer* renderer, const Text& text, int centerX, int y);
void drawWifiLine(SDL_Renderer* renderer, TTF_Font* font,
                  const std::string& value, int x, int y, SDL_Color color);
void drawBattery(UI& ui);
void drawSimplePanel(SDL_Renderer* renderer);

bool openUI(UI& ui);
void closeUI(UI& ui);

void updateWifiStatus(UI& ui);
bool loadWifiNetworks(UI& ui, bool scan);
void startWifiConnection(UI& ui, const WifiNetwork& network,
                         const std::string& password);
void pollWifiConnection(UI& ui);
void cancelWifiConnection(UI& ui);
void moveWifiSelection(UI& ui, int direction);
void refreshWifiMenuText(UI& ui);
void refreshWifiSettings(UI& ui);
void toggleWifiPower(UI& ui);
void moveWifiKeyboard(UI& ui, int dx, int dy);
bool activateWifiKeyboardKey(UI& ui);

void updateBattery(UI& ui);
bool refreshParentStatus(UI& ui);
void toggleParentManagement(UI& ui);
void updateSelectedGameText(UI& ui, int selected);
void refreshGameArtwork(UI& ui);

void drawHome(UI& ui, int selected);
void drawUI(UI& ui, int selected);
void drawMovies(UI& ui);
void drawSelectedGame(UI& ui, int selected, int action);
void drawGameInfo(UI& ui);
void drawSystemMenu(UI& ui, int selected);
void drawShutdownConfirm(UI& ui);
void drawPinEntry(UI& ui, const char* digits, int position, bool error);
void drawParentCenter(UI& ui, int selected);
void drawWifiSettings(UI& ui);
void drawWifiPassword(UI& ui);
