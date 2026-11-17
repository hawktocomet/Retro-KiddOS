#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
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
    Text heading, subtitle, footer, labels[4];
    Text menuHeading, menuItem, menuFooter;
    Text confirmHeading, confirmBody, confirmFooter;
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

static void destroyText(Text& t) {
    if (t.texture) SDL_DestroyTexture(t.texture);
    t.texture = nullptr;
}

static void closeUI(UI& ui) {
    destroyText(ui.heading);
    destroyText(ui.subtitle);
    destroyText(ui.footer);
    destroyText(ui.menuHeading);
    destroyText(ui.menuItem);
    destroyText(ui.menuFooter);
    destroyText(ui.confirmHeading);
    destroyText(ui.confirmBody);
    destroyText(ui.confirmFooter);
    for (int i = 0; i < 4; ++i) {
        destroyText(ui.labels[i]);
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
    ui.footer = makeText(ui.renderer, ui.footerFont, "D-PAD  MOVE     A  PLAY", muted);

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
        ui.labels[i] = makeText(ui.renderer, ui.labelFont, games[i].name, white);
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

    const SDL_Rect tiles[4] = {
        {105, 175, 235, 180}, {380, 175, 235, 180},
        {105, 395, 235, 180}, {380, 395, 235, 180}
    };

    for (int i = 0; i < 4; ++i) {
        SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &tile);

        int iw = 0, ih = 0;
        SDL_QueryTexture(ui.images[i], nullptr, nullptr, &iw, &ih);
        float scale = std::min(195.0f / iw, 112.0f / ih);
        SDL_Rect image = {
            tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
            tile.y + 8 + (112 - static_cast<int>(ih * scale)) / 2,
            static_cast<int>(iw * scale), static_cast<int>(ih * scale)
        };
        SDL_RenderCopy(r, ui.images[i], nullptr, &image);
        drawText(r, ui.labels[i], tile.x + tile.w / 2, tile.y + 130);

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

static void drawSystemMenu(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 105);
    drawText(r, ui.menuHeading, 360, 245);

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
    enum Screen { Games, SystemMenu, ConfirmShutdown };

    int selected = 0;
    Screen screen = Games;
    bool running = true;

    while (running) {
        UI ui;
        if (!openUI(ui)) return 1;

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

                    if (screen == Games) {
                        switch (key) {
                            case SDLK_UP:    if (selected >= 2) selected -= 2; break;
                            case SDLK_DOWN:  if (selected < 2) selected += 2; break;
                            case SDLK_LEFT:  if (selected % 2) --selected; break;
                            case SDLK_RIGHT: if (!(selected % 2)) ++selected; break;
                            case SDLK_RETURN:
                            case SDLK_SPACE: launch = true; break;
                            case SDLK_s: screen = SystemMenu; break;
                        }
                    } else if (key == SDLK_BACKSPACE) {
                        screen = (screen == ConfirmShutdown) ? SystemMenu : Games;
                    } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        if (screen == SystemMenu) {
                            screen = ConfirmShutdown;
                        } else if (requestShutdown()) {
                            running = false;
                        }
                    }
                }

                if (event.type == SDL_JOYBUTTONDOWN) {
                    const int button = event.jbutton.button;
                    if (screen == Games) {
                        switch (button) {
                            case 8:  if (selected >= 2) selected -= 2; break;
                            case 9:  if (selected < 2) selected += 2; break;
                            case 10: if (selected % 2) --selected; break;
                            case 11: if (!(selected % 2)) ++selected; break;
                            case 1:  launch = true; break;
                            case 13: screen = SystemMenu; break;
                        }
                    } else {
                        if (button == 0) {
                            screen = (screen == ConfirmShutdown) ? SystemMenu : Games;
                        } else if (button == 1) {
                            if (screen == SystemMenu) {
                                screen = ConfirmShutdown;
                            } else if (screen == ConfirmShutdown && requestShutdown()) {
                                running = false;
                            }
                        }
                    }
                }
            }

            if (running && !launch) {
                if (screen == Games) drawUI(ui, selected);
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
