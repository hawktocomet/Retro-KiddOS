// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pin_store.h"

static const char* BASE = "/storage/roms/.kiddos-ui-test-20261116";
static const char* CORE = "/storage/roms/.kiddos-ui-test-20261116/gambatte-link_libretro.so";
static const char* FONT = "/storage/.config/emulationstation/themes/rp-pocket/art/fonts/Cabin-Bold.ttf";

struct Game {
    const char* name;
    const char* rom;
    const char* platform;
    const char* image;
};

static const Game games[] = {
    {"Mario Tennis", "Mario Tennis.gbc", "gbc", "Mario Tennis.png"},
    {"Pokemon Red", "Pokemon Red Version.gb", "gb", "Pokemon Red Version.png"},
    {"Super Mario Bros. Deluxe", "Super Mario Bros. Deluxe.gbc", "gbc", "Super Mario Bros. Deluxe.png"},
    {"Tetris", "Tetris.gb", "gb", "Tetris.png"}
};

struct Text {
    SDL_Texture* texture = nullptr;
    int w = 0, h = 0;
};

enum class WifiStatus {
    Unknown,
    Disconnected,
    Connected
};

struct UI {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_Texture* images[4] = {};
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
    int batteryPercent = -1;
    bool externalPower = false;
    WifiStatus wifiStatus = WifiStatus::Unknown;
    bool wifiPowered = false;
    bool wifiPowerKnown = false;
    bool wifiCommandError = false;
    Uint32 batteryLastPoll = 0;
};

static Text makeText(SDL_Renderer* r, TTF_Font* f, const char* value, SDL_Color color) {
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

static void drawText(SDL_Renderer* r, const Text& t, int cx, int y) {
    if (!t.texture) return;
    SDL_Rect dst = {cx - t.w / 2, y, t.w, t.h};
    SDL_RenderCopy(r, t.texture, nullptr, &dst);
}

static void destroyText(Text& t);
static bool readSysfs(const char* path, char* value, size_t size) {
    FILE* file = std::fopen(path, "r");
    if (!file) return false;
    bool ok = std::fgets(value, static_cast<int>(size), file) != nullptr;
    std::fclose(file);
    if (ok) value[std::strcspn(value, "\r\n")] = 0;
    return ok;
}

static void updateWifiStatus(UI& ui) {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        ui.wifiStatus = WifiStatus::Unknown;
        ui.wifiPowerKnown = false;
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        ui.wifiStatus = WifiStatus::Unknown;
        ui.wifiPowerKnown = false;
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
        close(pipefd[1]);
        execl("/usr/bin/connmanctl", "connmanctl", "technologies",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);
    char output[4096];
    size_t used = 0;
    while (used + 1 < sizeof(output)) {
        ssize_t n = read(pipefd[0], output + used,
                         sizeof(output) - used - 1);
        if (n <= 0) break;
        used += static_cast<size_t>(n);
    }
    close(pipefd[0]);
    output[used] = '\0';

    int status = 0;
    if (waitpid(pid, &status, 0) < 0 ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        ui.wifiStatus = WifiStatus::Unknown;
        ui.wifiPowerKnown = false;
        return;
    }

    const char* wifi = std::strstr(
        output, "/net/connman/technology/wifi");
    if (!wifi) {
        ui.wifiStatus = WifiStatus::Unknown;
        ui.wifiPowerKnown = false;
        return;
    }

    const char* nextTechnology = std::strstr(wifi + 1, "\n/");
    const char* sectionEnd = nextTechnology ? nextTechnology : output + used;

    const char* poweredOn = std::strstr(wifi, "Powered = True");
    const char* poweredOff = std::strstr(wifi, "Powered = False");
    const char* connected = std::strstr(wifi, "Connected = True");
    const char* disconnected = std::strstr(wifi, "Connected = False");

    ui.wifiPowerKnown = false;
    if (poweredOn && poweredOn < sectionEnd) {
        ui.wifiPowered = true;
        ui.wifiPowerKnown = true;
    } else if (poweredOff && poweredOff < sectionEnd) {
        ui.wifiPowered = false;
        ui.wifiPowerKnown = true;
    }

    if (connected && connected < sectionEnd) {
        ui.wifiStatus = WifiStatus::Connected;
    } else if (disconnected && disconnected < sectionEnd) {
        ui.wifiStatus = WifiStatus::Disconnected;
    } else {
        ui.wifiStatus = WifiStatus::Unknown;
    }
}

static bool setWifiPower(bool enabled) {
    pid_t pid = fork();
    if (pid < 0) return false;

    if (pid == 0) {
        execl("/usr/bin/connmanctl", "connmanctl",
              enabled ? "enable" : "disable", "wifi",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    return waitpid(pid, &status, 0) >= 0 &&
           WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void refreshWifiMenuText(UI& ui) {
    const SDL_Color white = {245, 247, 255, 255};
    const SDL_Color muted = {190, 195, 210, 255};
    const SDL_Color green = {110, 230, 160, 255};
    const SDL_Color red = {255, 115, 115, 255};

    destroyText(ui.wifiPowerLabel);
    destroyText(ui.wifiConnectionLabel);
    destroyText(ui.wifiActionLabel);

    const char* power = !ui.wifiPowerKnown ? "WI-FI POWER: UNKNOWN" :
                        ui.wifiPowered ? "WI-FI POWER: ON" :
                                         "WI-FI POWER: OFF";
    const char* connection =
        ui.wifiStatus == WifiStatus::Connected ? "CONNECTION: CONNECTED" :
        ui.wifiStatus == WifiStatus::Disconnected ? "CONNECTION: NOT CONNECTED" :
                                                    "CONNECTION: UNKNOWN";
    const char* action = ui.wifiCommandError ? "WI-FI CHANGE FAILED" :
                         !ui.wifiPowerKnown ? "WI-FI POWER UNAVAILABLE" :
                         ui.wifiPowered ? "A  TURN WI-FI OFF" :
                                          "A  TURN WI-FI ON";

    ui.wifiPowerLabel = makeText(ui.renderer, ui.labelFont, power,
        !ui.wifiPowerKnown ? muted : ui.wifiPowered ? green : muted);
    ui.wifiConnectionLabel = makeText(ui.renderer, ui.labelFont,
        connection, ui.wifiStatus == WifiStatus::Connected ? green : muted);
    ui.wifiActionLabel = makeText(ui.renderer, ui.labelFont, action,
        ui.wifiCommandError ? red : white);
}

static void toggleWifiPower(UI& ui) {
    if (!ui.wifiPowerKnown) {
        ui.wifiCommandError = true;
        refreshWifiMenuText(ui);
        return;
    }

    const bool requestedPower = !ui.wifiPowered;
    ui.wifiCommandError = !setWifiPower(requestedPower);

    // ConnMan updates its reported technology state asynchronously.
    for (int attempt = 0; attempt < 10; ++attempt) {
        SDL_Delay(100);
        updateWifiStatus(ui);
        if (ui.wifiPowerKnown &&
            ui.wifiPowered == requestedPower) {
            break;
        }
    }

    ui.batteryLastPoll = 0;
    updateWifiStatus(ui);
    refreshWifiMenuText(ui);
}

static void updateBattery(UI& ui) {
    Uint32 now = SDL_GetTicks();
    if (ui.batteryLastPoll != 0 && now - ui.batteryLastPoll < 5000)
        return;
    ui.batteryLastPoll = now;
    updateWifiStatus(ui);

    const char* base =
        "/sys/devices/platform/ff180000.i2c/i2c-0/0-0020/";
    char path[256];
    char value[64];

    std::snprintf(path, sizeof(path),
        "%srk817-battery/power_supply/battery/capacity", base);
    if (readSysfs(path, value, sizeof(value))) {
        int percent = std::atoi(value);
        if (percent >= 0 && percent <= 100)
            ui.batteryPercent = percent;
    }

    ui.externalPower = false;
    const char* sources[] = {"ac", "usb"};
    for (const char* source : sources) {
        std::snprintf(path, sizeof(path),
            "%srk817-charger/power_supply/%s/online", base, source);
        if (readSysfs(path, value, sizeof(value)) &&
            std::atoi(value) > 0)
            ui.externalPower = true;
    }

    destroyText(ui.batteryLabel);
    if (ui.batteryPercent >= 0) {
        char label[16];
        std::snprintf(label, sizeof(label), "%d%%", ui.batteryPercent);
        SDL_Color color = ui.batteryPercent <= 15
            ? SDL_Color{255, 90, 90, 255}
            : SDL_Color{245, 248, 255, 255};
        ui.batteryLabel = makeText(
            ui.renderer, ui.footerFont, label, color);
    }
}

static void drawWifiStatus(UI& ui);

static void drawBattery(UI& ui) {
    drawWifiStatus(ui);
    if (ui.batteryPercent < 0) return;

    SDL_Renderer* r = ui.renderer;
    const int x = 574;
    const int y = 78;

    SDL_Color color = ui.batteryPercent <= 15
        ? SDL_Color{255, 90, 90, 255}
        : SDL_Color{155, 177, 215, 255};

    SDL_Rect outline = {x, y, 27, 14};
    SDL_Rect cap = {x + 27, y + 4, 3, 6};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderDrawRect(r, &outline);
    SDL_RenderFillRect(r, &cap);

    int fillWidth = (ui.batteryPercent * 21) / 100;
    if (fillWidth > 0) {
        SDL_Rect fill = {x + 3, y + 3, fillWidth, 8};
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(r, &fill);
    }

    if (ui.externalPower) {
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

    if (ui.wifiStatus == WifiStatus::Disconnected) {
        SDL_SetRenderDrawColor(r, 255, 90, 90, 255);
        SDL_RenderDrawLine(r, 544, 78, 562, 95);
    }
}

static void destroyText(Text& t) {
    if (t.texture) SDL_DestroyTexture(t.texture);
    t.texture = nullptr;
}

static bool managementControl(
    const char* action, char* output, size_t outputSize) {
    if (!output || outputSize == 0) return false;
    output[0] = '\0';

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        std::perror("KiddOS: management pipe");
        return false;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
        close(pipefd[1]);
        execl("/storage/.kiddos-management/control.sh",
              "control.sh", action, (char*)nullptr);
        _exit(127);
    }

    close(pipefd[1]);
    if (pid < 0) {
        close(pipefd[0]);
        std::perror("KiddOS: management fork");
        return false;
    }

    size_t used = 0;
    while (used + 1 < outputSize) {
        ssize_t n = read(pipefd[0], output + used, outputSize - used - 1);
        if (n <= 0) break;
        used += static_cast<size_t>(n);
    }
    close(pipefd[0]);
    output[used] = '\0';

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("KiddOS: management wait");
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void setParentMessage(UI& ui, const char* message, SDL_Color color) {
    destroyText(ui.parentBody);
    ui.parentBody = makeText(ui.renderer, ui.labelFont, message, color);
}

static bool refreshParentStatus(UI& ui) {
    char result[32];
    if (!managementControl("status", result, sizeof(result))) {
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

static void toggleParentManagement(UI& ui) {
    char result[32];
    if (!managementControl("status", result, sizeof(result))) {
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

    if (!managementControl(wasRunning ? "stop" : "start",
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
        if (managementControl("status", statusResult, sizeof(statusResult))) {
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

static void closeUI(UI& ui) {
    destroyText(ui.heading);
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
        if (ui.images[i]) SDL_DestroyTexture(ui.images[i]);
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

static bool openUI(UI& ui) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "KiddOS: SDL init failed: %s\n", SDL_GetError());
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::fprintf(stderr, "KiddOS: PNG init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return false;
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
        "UP/DOWN  SELECT     A  OPEN     B  BACK", muted);
    ui.wifiHeading = makeText(ui.renderer, ui.headingFont,
        "WI-FI SETTINGS", white);
    ui.wifiPowerLabel = makeText(ui.renderer, ui.labelFont,
        "WI-FI POWER: UNKNOWN", muted);
    ui.wifiConnectionLabel = makeText(ui.renderer, ui.labelFont,
        "CONNECTION: UNKNOWN", muted);
    ui.wifiActionLabel = makeText(ui.renderer, ui.labelFont,
        "WI-FI POWER UNAVAILABLE", muted);
    ui.wifiFooter = makeText(ui.renderer, ui.footerFont,
        "A  TOGGLE WI-FI     B  BACK", muted);

    for (int i = 0; i < 4; ++i) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s", BASE, games[i].image);
        ui.images[i] = IMG_LoadTexture(ui.renderer, path);
        if (!ui.images[i]) {
            std::fprintf(stderr, "KiddOS: artwork missing for %s: %s\n",
                         games[i].name, IMG_GetError());
            closeUI(ui);
            return false;
        }
    }
    std::puts("KiddOS: all four game images loaded.");
    std::fflush(stdout);
    return true;
}

static void drawUI(UI& ui, int selected) {
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

        int iw = 0, ih = 0;
        SDL_QueryTexture(ui.images[i], nullptr, nullptr, &iw, &ih);
        float scale = std::min(224.0f / iw, 166.0f / ih);
        SDL_Rect image = {
            tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
            tile.y + (tile.h - static_cast<int>(ih * scale)) / 2,
            static_cast<int>(iw * scale), static_cast<int>(ih * scale)
        };
        SDL_RenderCopy(r, ui.images[i], nullptr, &image);

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

static const char* platformName(const char* platform) {
    if (std::strcmp(platform, "gb") == 0) return "GAME BOY";
    if (std::strcmp(platform, "gbc") == 0) return "GAME BOY COLOR";
    return "GAME";
}

static void updateSelectedGameText(UI& ui, int selected) {
    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};

    destroyText(ui.selectedTitle);
    destroyText(ui.selectedPlatform);
    ui.selectedTitle = makeText(
        ui.renderer, ui.labelFont, games[selected].name, white);
    ui.selectedPlatform = makeText(
        ui.renderer, ui.footerFont, platformName(games[selected].platform), muted);
}

static void drawSimplePanel(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderClear(r);
    SDL_Rect panel = {48, 48, 624, 624};
    SDL_SetRenderDrawColor(r, 35, 105, 220, 255);
    SDL_RenderFillRect(r, &panel);
    SDL_Rect inner = {58, 58, 604, 604};
    SDL_SetRenderDrawColor(r, 12, 18, 42, 255);
    SDL_RenderFillRect(r, &inner);
}

static void drawSelectedGame(UI& ui, int selected, int action) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 78);
    drawText(r, ui.selectedTitle, 360, 133);
    drawBattery(ui);

    int iw = 0, ih = 0;
    SDL_QueryTexture(ui.images[selected], nullptr, nullptr, &iw, &ih);
    float scale = std::min(280.0f / iw, 350.0f / ih);
    SDL_Rect image = {
        88 + (280 - static_cast<int>(iw * scale)) / 2,
        205 + (350 - static_cast<int>(ih * scale)) / 2,
        static_cast<int>(iw * scale), static_cast<int>(ih * scale)
    };
    SDL_RenderCopy(r, ui.images[selected], nullptr, &image);

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

static void drawGameInfo(UI& ui) {
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

static void drawSystemMenu(UI& ui, int selected) {
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

static void drawShutdownConfirm(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.confirmHeading, 360, 245);
    drawBattery(ui);
    drawText(r, ui.confirmBody, 360, 345);
    drawText(r, ui.confirmFooter, 360, 625);
    SDL_RenderPresent(r);
}

static void drawPinEntry(UI& ui, const char* digits, int position, bool error) {
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

static void drawParentCenter(UI& ui, int selected) {
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

    drawText(r, ui.parentBody, 360, 294);
    drawText(r, ui.parentWifi, 360, 394);
    drawText(r, ui.parentChangePin, 360, 494);
    drawBattery(ui);
    drawText(r, ui.parentFooter, 360, 625);
    SDL_RenderPresent(r);
}

static void drawWifiSettings(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.wifiHeading, 360, 220);
    drawText(r, ui.wifiPowerLabel, 360, 300);
    drawText(r, ui.wifiConnectionLabel, 360, 370);

    SDL_Rect actionRow = {90, 440, 540, 82};
    SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
    for (int n = 0; n < 3; ++n) {
        SDL_Rect outline = {
            actionRow.x - n, actionRow.y - n,
            actionRow.w + n * 2, actionRow.h + n * 2
        };
        SDL_RenderDrawRect(r, &outline);
    }

    drawText(r, ui.wifiActionLabel, 360, 470);
    drawBattery(ui);
    drawText(r, ui.wifiFooter, 360, 625);
    SDL_RenderPresent(r);
}

static bool requestShutdown() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/systemctl", "systemctl", "poweroff", (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("Retro KiddOS: shutdown fork failed");
        return false;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("Retro KiddOS: shutdown wait failed");
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "Retro KiddOS: systemctl poweroff failed (status %d)\n", status);
        return false;
    }
    return true;
}

static bool launchGame(int index) {
    char rom[512];
    std::snprintf(rom, sizeof(rom), "%s/%s", BASE, games[index].rom);

    std::printf("KiddOS: launching %s\n", games[index].name);
    std::fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        execlp("cp", "cp", CORE, "/tmp/cores/gambatte_libretro.so", (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("KiddOS: fork for core copy");
        return false;
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::fprintf(stderr, "KiddOS: patched core staging failed\n");
        return false;
    }

    pid = fork();
    if (pid == 0) {
        execl("/usr/bin/runemu.py", "runemu.py",
              "--rom", rom,
              "--platform", games[index].platform,
              "--emulator", "retroarch",
              "--core", "gambatte",
              (char*)nullptr);
        _exit(127);
    }
    if (pid < 0) {
        std::perror("KiddOS: fork for game");
        return false;
    }
    if (waitpid(pid, &status, 0) < 0) {
        std::perror("KiddOS: wait for game");
        return false;
    }
    std::printf("KiddOS: game exited (status %d); returning to menu\n", status);
    std::fflush(stdout);
    return true;
}

int main() {
    enum Screen {
        Games, SelectedGame, GameInfo, SystemMenu,
        PinEntry, ParentCenter, WifiSettings, ConfirmShutdown
    };

    int selected = 0;
    int action = 0;
    int menuSelected = 0;
    Screen screen = Games;
    Screen menuReturnScreen = Games;
    char pinDigits[5] = "0000";
    int pinPosition = 0;
    bool pinError = false;
    enum class PinTask { Unlock, Current, New, Confirm };
    PinTask pinTask = PinTask::Unlock;
    char currentPin[5] = "";
    char newPin[5] = "";
    int parentSelected = 0;
    bool running = true;

    if (!KiddosPin::initializePinStore("/storage/.config/kiddos")) {
        std::fprintf(stderr,
                     "KiddOS: PIN store initialization failed; refusing to start.\n");
        return 1;
    }

    while (running) {
        UI ui;
        if (!openUI(ui)) return 1;
        updateSelectedGameText(ui, selected);

        const SDL_Color pinWhite = {245, 248, 255, 255};
        const SDL_Color pinRed = {255, 115, 115, 255};

        auto setPinPrompt = [&](const char* prompt, const char* error) {
            destroyText(ui.pinPrompt);
            destroyText(ui.pinError);
            ui.pinPrompt = makeText(ui.renderer, ui.labelFont,
                                    prompt, pinWhite);
            ui.pinError = makeText(ui.renderer, ui.footerFont,
                                   error, pinRed);
        };

        auto resetPinDigits = [&]() {
            pinDigits[0] = pinDigits[1] = pinDigits[2] = pinDigits[3] = '0';
            pinDigits[4] = '\0';
            pinPosition = 0;
            pinError = false;
        };

        auto beginPinChange = [&]() {
            pinTask = PinTask::Current;
            resetPinDigits();
            setPinPrompt("ENTER CURRENT PIN", "INCORRECT PIN - TRY AGAIN");
            screen = PinEntry;
        };

        auto cancelPinChange = [&]() {
            pinTask = PinTask::Unlock;
            currentPin[0] = newPin[0] = '\0';
            resetPinDigits();
            screen = ParentCenter;
            refreshParentStatus(ui);
        };

        auto submitPin = [&]() {
            if (pinTask == PinTask::Unlock) {
                if (KiddosPin::verifyPin("/storage/.config/kiddos", pinDigits)) {
                    screen = ParentCenter;
                    parentSelected = 0;
                    pinError = false;
                    refreshParentStatus(ui);
                } else {
                    resetPinDigits();
                    pinError = true;
                }
                return;
            }

            if (pinTask == PinTask::Current) {
                if (!KiddosPin::verifyPin("/storage/.config/kiddos", pinDigits)) {
                    resetPinDigits();
                    pinError = true;
                    return;
                }
                std::memcpy(currentPin, pinDigits, sizeof(currentPin));
                pinTask = PinTask::New;
                resetPinDigits();
                setPinPrompt("ENTER NEW PIN", " ");
                return;
            }

            if (pinTask == PinTask::New) {
                std::memcpy(newPin, pinDigits, sizeof(newPin));
                pinTask = PinTask::Confirm;
                resetPinDigits();
                setPinPrompt("CONFIRM NEW PIN", " ");
                return;
            }

            if (std::strcmp(newPin, pinDigits) != 0) {
                pinTask = PinTask::New;
                resetPinDigits();
                setPinPrompt("ENTER NEW PIN", "PINS DID NOT MATCH - TRY AGAIN");
                pinError = true;
                return;
            }

            if (KiddosPin::changePin(
                    "/storage/.config/kiddos", currentPin, newPin)) {
                pinTask = PinTask::Unlock;
                currentPin[0] = newPin[0] = '\0';
                resetPinDigits();
                screen = ParentCenter;
                parentSelected = 2;
                refreshParentStatus(ui);
            } else {
                pinTask = PinTask::New;
                resetPinDigits();
                setPinPrompt("ENTER NEW PIN", "COULD NOT SAVE PIN - TRY AGAIN");
                pinError = true;
            }
        };

        bool launch = false;
        while (running && !launch) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    continue;
                }

                if (event.type == SDL_KEYDOWN) {
                    const SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_ESCAPE) {
                        running = false;
                        continue;
                    }

                    if (key == SDLK_BACKSPACE) {
                        if (screen == WifiSettings)
                            screen = ParentCenter;
                        else if (screen == PinEntry && pinTask != PinTask::Unlock)
                            cancelPinChange();
                        else if (screen == ConfirmShutdown ||
                                 screen == PinEntry || screen == ParentCenter)
                            screen = SystemMenu;
                        else if (screen == SystemMenu)
                            screen = menuReturnScreen;
                        else if (screen == GameInfo)
                            screen = SelectedGame;
                        else if (screen == SelectedGame)
                            screen = Games;
                        continue;
                    }

                    if (screen == PinEntry) {
                        if (key == SDLK_UP) {
                            pinDigits[pinPosition] = static_cast<char>(
                                '0' + (pinDigits[pinPosition] - '0' + 1) % 10);
                        } else if (key == SDLK_DOWN) {
                            pinDigits[pinPosition] = static_cast<char>(
                                '0' + (pinDigits[pinPosition] - '0' + 9) % 10);
                        } else if (key == SDLK_LEFT) {
                            pinPosition = (pinPosition + 3) % 4;
                        } else if (key == SDLK_RIGHT) {
                            pinPosition = (pinPosition + 1) % 4;
                        } else if (key >= SDLK_0 && key <= SDLK_9) {
                            pinDigits[pinPosition] =
                                static_cast<char>('0' + (key - SDLK_0));
                            if (pinPosition < 3) ++pinPosition;
                            pinError = false;
                        } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            submitPin();
                        }
                        continue;
                    }

                    if (screen == WifiSettings) {
                        if (key == SDLK_RETURN || key == SDLK_SPACE)
                            toggleWifiPower(ui);
                        continue;
                    }

                    if (screen == ParentCenter) {
                        if (key == SDLK_UP)
                            parentSelected = (parentSelected + 2) % 3;
                        else if (key == SDLK_DOWN)
                            parentSelected = (parentSelected + 1) % 3;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (parentSelected == 0)
                                toggleParentManagement(ui);
                            else if (parentSelected == 1) {
                                updateWifiStatus(ui);
                                ui.wifiCommandError = false;
                                refreshWifiMenuText(ui);
                                screen = WifiSettings;
                            } else
                                beginPinChange();
                        }
                        continue;
                    }

                    if (screen == Games) {
                        switch (key) {
                            case SDLK_UP:    if (selected >= 2) selected -= 2; break;
                            case SDLK_DOWN:  if (selected < 2) selected += 2; break;
                            case SDLK_LEFT:  if (selected % 2) --selected; break;
                            case SDLK_RIGHT: if (!(selected % 2)) ++selected; break;
                            case SDLK_RETURN:
                            case SDLK_SPACE:
                                screen = SelectedGame;
                                action = 0;
                                break;
                            case SDLK_s:
                                menuReturnScreen = screen;
                                screen = SystemMenu;
                                break;
                        }
                        updateSelectedGameText(ui, selected);
                    } else if (screen == SelectedGame) {
                        if (key == SDLK_UP || key == SDLK_DOWN)
                            action = (action + 1) % 2;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (action == 0) launch = true;
                            else if (action == 1) screen = GameInfo;
                        }
                    } else if (screen == GameInfo) {
                        if (key == SDLK_RETURN || key == SDLK_SPACE)
                            screen = SelectedGame;
                    } else if (screen == SystemMenu) {
                        if (key == SDLK_UP || key == SDLK_DOWN)
                            menuSelected = (menuSelected + 1) % 2;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (menuSelected == 1) {
                                screen = ConfirmShutdown;
                            } else {
                                pinTask = PinTask::Unlock;
                                currentPin[0] = newPin[0] = '\0';
                                resetPinDigits();
                                setPinPrompt("ENTER PARENT PIN",
                                             "INCORRECT PIN - TRY AGAIN");
                                screen = PinEntry;
                            }
                        }
                    } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        if (requestShutdown()) running = false;
                    }
                }

                if (event.type == SDL_JOYBUTTONDOWN) {
                    const int button = event.jbutton.button;

                    if (button == 0) { // B / Back
                        if (screen == WifiSettings)
                            screen = ParentCenter;
                        else if (screen == PinEntry && pinTask != PinTask::Unlock)
                            cancelPinChange();
                        else if (screen == ConfirmShutdown ||
                                 screen == PinEntry || screen == ParentCenter)
                            screen = SystemMenu;
                        else if (screen == SystemMenu)
                            screen = menuReturnScreen;
                        else if (screen == GameInfo)
                            screen = SelectedGame;
                        else if (screen == SelectedGame)
                            screen = Games;
                    } else if (button == 1) { // A / Select
                        if (screen == Games) {
                            screen = SelectedGame;
                            action = 0;
                        } else if (screen == SelectedGame) {
                            if (action == 0) launch = true;
                            else if (action == 1) screen = GameInfo;
                        } else if (screen == GameInfo) {
                            screen = SelectedGame;
                        } else if (screen == SystemMenu) {
                            if (menuSelected == 1) {
                                screen = ConfirmShutdown;
                            } else {
                                pinTask = PinTask::Unlock;
                                currentPin[0] = newPin[0] = '\0';
                                resetPinDigits();
                                setPinPrompt("ENTER PARENT PIN",
                                             "INCORRECT PIN - TRY AGAIN");
                                screen = PinEntry;
                            }
                        } else if (screen == PinEntry) {
                            submitPin();
                        } else if (screen == ParentCenter) {
                            if (parentSelected == 0)
                                toggleParentManagement(ui);
                            else if (parentSelected == 1) {
                                updateWifiStatus(ui);
                                ui.wifiCommandError = false;
                                refreshWifiMenuText(ui);
                                screen = WifiSettings;
                            } else
                                beginPinChange();
                        } else if (screen == WifiSettings) {
                            toggleWifiPower(ui);
                        } else if (screen == ConfirmShutdown &&
                                   requestShutdown()) {
                            running = false;
                        }
                    } else if (button == 8) { // Up
                        if (screen == Games) {
                            if (selected >= 2) selected -= 2;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 2) % 3;
                        } else if (screen == WifiSettings) {
                            // Wi-Fi Settings has one action; no vertical navigation.
                        } else if (screen == PinEntry) {
                            pinDigits[pinPosition] = static_cast<char>(
                                '0' + (pinDigits[pinPosition] - '0' + 1) % 10);
                        }
                    } else if (button == 9) { // Down
                        if (screen == Games) {
                            if (selected < 2) selected += 2;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 1) % 3;
                        } else if (screen == WifiSettings) {
                            // Wi-Fi Settings has one action; no vertical navigation.
                        } else if (screen == PinEntry) {
                            pinDigits[pinPosition] = static_cast<char>(
                                '0' + (pinDigits[pinPosition] - '0' + 9) % 10);
                        }
                    } else if (button == 10) { // Left
                        if (screen == Games) {
                            if (selected % 2) --selected;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == PinEntry) {
                            pinPosition = (pinPosition + 3) % 4;
                        }
                    } else if (button == 11) { // Right
                        if (screen == Games) {
                            if (!(selected % 2)) ++selected;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == PinEntry) {
                            pinPosition = (pinPosition + 1) % 4;
                        }
                    } else if (button == 13 &&
                               (screen == Games || screen == SelectedGame ||
                                screen == GameInfo)) { // Start
                        menuReturnScreen = screen;
                        screen = SystemMenu;
                    }
                }

                if (event.type == SDL_JOYHATMOTION &&
                    screen == PinEntry) {
                    const Uint8 hat = event.jhat.value;

                    if (hat & SDL_HAT_UP) {
                        pinDigits[pinPosition] = static_cast<char>(
                            '0' + (pinDigits[pinPosition] - '0' + 1) % 10);
                    }
                    if (hat & SDL_HAT_DOWN) {
                        pinDigits[pinPosition] = static_cast<char>(
                            '0' + (pinDigits[pinPosition] - '0' + 9) % 10);
                    }
                    if (hat & SDL_HAT_LEFT) {
                        pinPosition = (pinPosition + 3) % 4;
                    }
                    if (hat & SDL_HAT_RIGHT) {
                        pinPosition = (pinPosition + 1) % 4;
                    }
                }
            }

            if (running && !launch) {
                updateBattery(ui);
                if (screen == Games) drawUI(ui, selected);
                else if (screen == SelectedGame) drawSelectedGame(ui, selected, action);
                else if (screen == GameInfo) drawGameInfo(ui);
                else if (screen == SystemMenu) drawSystemMenu(ui, menuSelected);
                else if (screen == PinEntry)
                    drawPinEntry(ui, pinDigits, pinPosition, pinError);
                else if (screen == ParentCenter)
                    drawParentCenter(ui, parentSelected);
                else if (screen == WifiSettings)
                    drawWifiSettings(ui);
                else drawShutdownConfirm(ui);
                SDL_Delay(16);
            }
        }

        closeUI(ui);
        if (running && launch) {
            if (!launchGame(selected))
                SDL_Delay(1500);
            screen = Games;
        }
    }

    return 0;
}
