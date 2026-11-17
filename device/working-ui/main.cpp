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
    Text menuHeading, menuItem, menuFooter;
    Text confirmHeading, confirmBody, confirmFooter;
    Text batteryLabel;
    int batteryPercent = -1;
    bool externalPower = false;
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

static void updateBattery(UI& ui) {
    Uint32 now = SDL_GetTicks();
    if (ui.batteryLastPoll != 0 && now - ui.batteryLastPoll < 5000)
        return;
    ui.batteryLastPoll = now;

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

static void drawBattery(UI& ui) {
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

static void destroyText(Text& t) {
    if (t.texture) SDL_DestroyTexture(t.texture);
    t.texture = nullptr;
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
    destroyText(ui.menuItem);
    destroyText(ui.menuFooter);
    destroyText(ui.confirmHeading);
    destroyText(ui.confirmBody);
    destroyText(ui.confirmFooter);
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
    ui.menuItem = makeText(ui.renderer, ui.labelFont, "Shut Down", white);
    ui.menuFooter = makeText(ui.renderer, ui.footerFont, "A  SELECT     B  BACK", muted);
    ui.confirmHeading = makeText(ui.renderer, ui.headingFont, "SHUT DOWN?", white);
    ui.confirmBody = makeText(ui.renderer, ui.labelFont,
        "Are you sure you want to turn off the system?", white);
    ui.confirmFooter = makeText(ui.renderer, ui.footerFont,
        "A  YES     B  CANCEL", muted);

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

static void drawSystemMenu(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.menuHeading, 360, 245);
    drawBattery(ui);

    SDL_Rect item = {170, 335, 380, 82};
    SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
    SDL_RenderFillRect(r, &item);
    SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
    for (int n = 0; n < 4; ++n) {
        SDL_Rect outline = {item.x - n, item.y - n,
                            item.w + n * 2, item.h + n * 2};
        SDL_RenderDrawRect(r, &outline);
    }
    drawText(r, ui.menuItem, 360, 363);
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
    enum Screen { Games, SelectedGame, GameInfo, SystemMenu, ConfirmShutdown };

    int selected = 0;
    int action = 0;
    Screen screen = Games;
    Screen menuReturnScreen = Games;
    bool running = true;

    while (running) {
        UI ui;
        if (!openUI(ui)) return 1;
        updateSelectedGameText(ui, selected);

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
                        if (screen == ConfirmShutdown) screen = SystemMenu;
                        else if (screen == SystemMenu) screen = menuReturnScreen;
                        else if (screen == GameInfo) screen = SelectedGame;
                        else if (screen == SelectedGame) screen = Games;
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
                        if (key == SDLK_UP) action = (action + 1) % 2;
                        else if (key == SDLK_DOWN) action = (action + 1) % 2;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (action == 0) launch = true;
                            else if (action == 1) screen = GameInfo;
                        }
                    } else if (screen == GameInfo) {
                        if (key == SDLK_RETURN || key == SDLK_SPACE)
                            screen = SelectedGame;
                    } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        if (screen == SystemMenu) screen = ConfirmShutdown;
                        else if (requestShutdown()) running = false;
                    }
                }

                if (event.type == SDL_JOYBUTTONDOWN) {
                    const int button = event.jbutton.button;

                    if (button == 0) { // B / Back
                        if (screen == ConfirmShutdown) screen = SystemMenu;
                        else if (screen == SystemMenu) screen = menuReturnScreen;
                        else if (screen == GameInfo) screen = SelectedGame;
                        else if (screen == SelectedGame) screen = Games;
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
                            screen = ConfirmShutdown;
                        } else if (screen == ConfirmShutdown && requestShutdown()) {
                            running = false;
                        }
                    } else if (button == 8) { // Up
                        if (screen == Games) {
                            if (selected >= 2) selected -= 2;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) action = (action + 1) % 2;
                    } else if (button == 9) { // Down
                        if (screen == Games) {
                            if (selected < 2) selected += 2;
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) action = (action + 1) % 2;
                    } else if (button == 10 && screen == Games) { // Left
                        if (selected % 2) --selected;
                        updateSelectedGameText(ui, selected);
                    } else if (button == 11 && screen == Games) { // Right
                        if (!(selected % 2)) ++selected;
                        updateSelectedGameText(ui, selected);
                    } else if (button == 13 &&
                               (screen == Games || screen == SelectedGame || screen == GameInfo)) { // Start
                        menuReturnScreen = screen;
                        screen = SystemMenu;
                    }
                }
            }

            if (running && !launch) {
                updateBattery(ui);
                if (screen == Games) drawUI(ui, selected);
                else if (screen == SelectedGame) drawSelectedGame(ui, selected, action);
                else if (screen == GameInfo) drawGameInfo(ui);
                else if (screen == SystemMenu) drawSystemMenu(ui);
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
