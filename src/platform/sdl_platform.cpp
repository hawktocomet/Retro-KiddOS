#include "kiddos/platform/sdl_platform.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace kiddos {

namespace {

struct Color {
    Uint8 r, g, b, a;
};

std::array<SDL_Texture*, 3> artwork_textures{};
std::array<SDL_Texture*, 3> action_label_textures{};

SDL_Texture* load_bitmap_texture(SDL_Renderer* renderer,
                                 const char* path,
                                 bool magenta_key = false) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) {
        std::cerr << "Could not load " << path << ": "
                  << SDL_GetError() << '\\n';
        return nullptr;
    }

    if (magenta_key) {
        const Uint32 key = SDL_MapRGB(surface->format, 255, 0, 255);
        SDL_SetColorKey(surface, SDL_TRUE, key);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    } else {
        std::cerr << "Could not create texture for " << path << ": "
                  << SDL_GetError() << '\\n';
    }
    return texture;
}

void color(SDL_Renderer* r, Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

void rectangle(SDL_Renderer* r, SDL_Rect rect, Color c) {
    color(r, c);
    SDL_RenderFillRect(r, &rect);
}

void outline(SDL_Renderer* r, SDL_Rect rect, Color c) {
    color(r, c);
    SDL_RenderDrawRect(r, &rect);
}

void rounded_fill(SDL_Renderer* r, SDL_Rect rect, int radius, Color c) {
    if (rect.w <= 0 || rect.h <= 0) return;
    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));
    color(r, c);

    for (int y = 0; y < rect.h; ++y) {
        int inset = 0;
        if (y < radius) {
            const double dy = radius - y - 0.5;
            inset = radius - static_cast<int>(
                std::sqrt(std::max(0.0, radius * radius - dy * dy)));
        } else if (y >= rect.h - radius) {
            const double dy = y - (rect.h - radius) + 0.5;
            inset = radius - static_cast<int>(
                std::sqrt(std::max(0.0, radius * radius - dy * dy)));
        }
        SDL_RenderDrawLine(r, rect.x + inset, rect.y + y,
                           rect.x + rect.w - 1 - inset, rect.y + y);
    }
}

void rounded_outline(SDL_Renderer* r, SDL_Rect rect, int radius, Color c) {
    if (rect.w <= 0 || rect.h <= 0) return;
    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));
    color(r, c);

    SDL_RenderDrawLine(r, rect.x + radius, rect.y,
                       rect.x + rect.w - radius - 1, rect.y);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y + rect.h - 1,
                       rect.x + rect.w - radius - 1, rect.y + rect.h - 1);
    SDL_RenderDrawLine(r, rect.x, rect.y + radius,
                       rect.x, rect.y + rect.h - radius - 1);
    SDL_RenderDrawLine(r, rect.x + rect.w - 1, rect.y + radius,
                       rect.x + rect.w - 1, rect.y + rect.h - radius - 1);

    for (int i = 0; i <= radius * 2; ++i) {
        const double angle = (3.14159265358979323846 / 2.0) *
                             static_cast<double>(i) /
                             std::max(1, radius * 2);
        const int dx = static_cast<int>(std::round(radius * std::cos(angle)));
        const int dy = static_cast<int>(std::round(radius * std::sin(angle)));
        SDL_RenderDrawPoint(r, rect.x + radius - dx,
                            rect.y + radius - dy);
        SDL_RenderDrawPoint(r, rect.x + rect.w - radius - 1 + dx,
                            rect.y + radius - dy);
        SDL_RenderDrawPoint(r, rect.x + radius - dx,
                            rect.y + rect.h - radius - 1 + dy);
        SDL_RenderDrawPoint(r, rect.x + rect.w - radius - 1 + dx,
                            rect.y + rect.h - radius - 1 + dy);
    }
}

void mask_artwork_corners(SDL_Renderer* r, SDL_Rect rect,
                          int radius, Color background) {
    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));
    color(r, background);

    for (int y = 0; y < radius; ++y) {
        const double dy = radius - y - 0.5;
        const int inset = radius - static_cast<int>(
            std::sqrt(std::max(0.0, radius * radius - dy * dy)));
        SDL_RenderDrawLine(r, rect.x, rect.y + y,
                           rect.x + inset, rect.y + y);
        SDL_RenderDrawLine(r, rect.x + rect.w - 1 - inset, rect.y + y,
                           rect.x + rect.w - 1, rect.y + y);

        const int bottom_y = rect.y + rect.h - 1 - y;
        SDL_RenderDrawLine(r, rect.x, bottom_y,
                           rect.x + inset, bottom_y);
        SDL_RenderDrawLine(r, rect.x + rect.w - 1 - inset, bottom_y,
                           rect.x + rect.w - 1, bottom_y);
    }
}

void circle(SDL_Renderer* r, int cx, int cy, int radius, Color c) {
    color(r, c);

    for (int y = -radius; y <= radius; ++y) {
        const int x = static_cast<int>(
            std::sqrt(static_cast<double>(
                radius * radius - y * y
            ))
        );
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
    }
}

void triangle(SDL_Renderer* r, int x1, int y1,
              int x2, int y2, int x3, int y3, Color c) {
    color(r, c);

    const int min_y = std::min({y1, y2, y3});
    const int max_y = std::max({y1, y2, y3});

    for (int y = min_y; y <= max_y; ++y) {
        int intersections[3];
        int count = 0;

        const int xs[] = {x1, x2, x3};
        const int ys[] = {y1, y2, y3};

        for (int i = 0; i < 3; ++i) {
            const int j = (i + 1) % 3;

            if ((ys[i] <= y && ys[j] > y) ||
                (ys[j] <= y && ys[i] > y)) {
                const int x = xs[i] +
                    (y - ys[i]) * (xs[j] - xs[i]) /
                    (ys[j] - ys[i]);

                intersections[count++] = x;
            }
        }

        if (count == 2) {
            SDL_RenderDrawLine(
                r,
                std::min(intersections[0], intersections[1]),
                y,
                std::max(intersections[0], intersections[1]),
                y
            );
        }
    }
}

void draw_space_world(SDL_Renderer* r, SDL_Rect a, Uint8 alpha) {
    auto c = [alpha](Uint8 red, Uint8 green, Uint8 blue) {
        return Color{red, green, blue, alpha};
    };

    rectangle(r, a, c(17, 24, 67));

    circle(r, a.x + a.w * 68 / 100,
           a.y + a.h * 29 / 100,
           a.w / 5, c(91, 99, 255));

    circle(r, a.x + a.w * 68 / 100,
           a.y + a.h * 29 / 100,
           a.w / 7, c(168, 154, 255));

    for (int i = 0; i < 12; ++i) {
        const int x = a.x + (i * 47 + 19) % a.w;
        const int y = a.y + (i * 71 + 31) % (a.h * 65 / 100);
        circle(r, x, y, 2, c(220, 231, 255));
    }

    triangle(r, a.x, a.y + a.h,
             a.x + a.w / 2, a.y + a.h / 3,
             a.x + a.w, a.y + a.h, c(39, 63, 139));

    triangle(r, a.x, a.y + a.h,
             a.x + a.w / 3, a.y + a.h * 60 / 100,
             a.x + a.w * 72 / 100, a.y + a.h, c(26, 43, 103));
}

void draw_forest_world(SDL_Renderer* r, SDL_Rect a, Uint8 alpha) {
    auto c = [alpha](Uint8 red, Uint8 green, Uint8 blue) {
        return Color{red, green, blue, alpha};
    };

    rectangle(r, a, c(17, 73, 87));

    circle(r, a.x + a.w * 70 / 100,
           a.y + a.h * 25 / 100,
           a.w / 7, c(113, 224, 203));

    triangle(r, a.x, a.y + a.h * 80 / 100,
             a.x + a.w / 2, a.y + a.h / 3,
             a.x + a.w, a.y + a.h * 80 / 100,
             c(32, 119, 107));

    triangle(r, a.x, a.y + a.h,
             a.x + a.w / 2, a.y + a.h / 2,
             a.x + a.w, a.y + a.h,
             c(16, 69, 78));

    for (int i = 0; i < 6; ++i) {
        const int x = a.x + (i * 61 + 23) % a.w;
        rectangle(r, {x, a.y + a.h * 65 / 100, 8, a.h / 4},
                  c(85, 62, 49));
        triangle(r, x - 19, a.y + a.h * 68 / 100,
                 x + 4, a.y + a.h * 39 / 100,
                 x + 27, a.y + a.h * 68 / 100,
                 c(42, 160, 120));
    }
}

void draw_ocean_world(SDL_Renderer* r, SDL_Rect a, Uint8 alpha) {
    auto c = [alpha](Uint8 red, Uint8 green, Uint8 blue) {
        return Color{red, green, blue, alpha};
    };

    rectangle(r, a, c(25, 91, 157));

    circle(r, a.x + a.w / 2,
           a.y + a.h / 3,
           a.w / 6, c(255, 207, 124));

    rectangle(r,
              {a.x, a.y + a.h * 57 / 100,
               a.w, a.h * 43 / 100},
              c(19, 111, 184));

    for (int i = 0; i < 5; ++i) {
        const int y = a.y + a.h * 63 / 100 + i * a.h / 14;
        color(r, c(108, 206, 239));
        for (int x = a.x + 8; x < a.x + a.w - 8; x += 2) {
            const int wave_y = y +
                static_cast<int>(std::sin(x * 0.055 + i) * 5);
            SDL_RenderDrawPoint(r, x, wave_y);
        }
    }

    triangle(r, a.x, a.y + a.h * 60 / 100,
             a.x + a.w / 3, a.y + a.h * 43 / 100,
             a.x + a.w * 55 / 100, a.y + a.h * 60 / 100,
             c(39, 139, 166));
}

void draw_art(SDL_Renderer* r, int index,
              SDL_Rect area, Uint8 alpha) {
    if (index < 0 || index >= static_cast<int>(artwork_textures.size())) return;
    SDL_Texture* texture = artwork_textures[static_cast<std::size_t>(index)];
    if (!texture) return;

    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_RenderCopy(r, texture, nullptr, &area);
    SDL_SetTextureAlphaMod(texture, 255);
}

void draw_card(SDL_Renderer* r, int index,
               SDL_Rect rect, bool selected) {
    const Uint8 alpha = selected ? 255 : 145;

    rounded_fill(r,
                 {rect.x - 5, rect.y - 5, rect.w + 10, rect.h + 10},
                 30, {7, 13, 32, static_cast<Uint8>(selected ? 255 : 170)});

    rounded_fill(r, rect, 25, {20, 34, 65, alpha});

    SDL_Rect art{
        rect.x + 9,
        rect.y + 9,
        rect.w - 18,
        rect.h - 18
    };

    SDL_RenderSetClipRect(r, &art);
    draw_art(r, index, art, alpha);
    SDL_RenderSetClipRect(r, nullptr);
    mask_artwork_corners(r, art, 18, {20, 34, 65, alpha});

    rounded_outline(r, rect, 25,
            selected
                ? Color{69, 151, 255, 255}
                : Color{80, 117, 170, 110});

    if (selected) {
        rounded_outline(r,
                        {rect.x - 2, rect.y - 2, rect.w + 4, rect.h + 4},
                        27, {43, 105, 210, 170});
    }
}

void draw_status(SDL_Renderer* r) {
    const Color white{205, 222, 255, 220};
    color(r, white);

    // Sound status: speaker and sound waves.
    triangle(r, 31, 34, 43, 25, 43, 51, white);
    rectangle(r, {25, 33, 10, 12}, white);

    color(r, {80, 157, 255, 220});
    SDL_RenderDrawLine(r, 49, 29, 54, 34);
    SDL_RenderDrawLine(r, 54, 34, 54, 42);
    SDL_RenderDrawLine(r, 54, 42, 49, 47);

    // Battery status: outline, terminal, charge fill.
    outline(r, {657, 27, 34, 19}, white);
    rectangle(r, {691, 33, 4, 7}, white);
    rectangle(r, {661, 31, 22, 11},
              {67, 157, 255, 240});
}

} // namespace

SDLPlatform::~SDLPlatform() {
    shutdown();
}

bool SDLPlatform::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL initialization failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    initialized_ = true;

    window_ = SDL_CreateWindow(
        "Retro KiddOS - Kid Mode",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        720,
        720,
        SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "Window creation failed: "
                  << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(
            window_, -1, SDL_RENDERER_SOFTWARE
        );
    }

    if (!renderer_) {
        std::cerr << "Renderer creation failed: "
                  << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    const char* artwork_paths[] = {
        "assets/artwork/generated/space.bmp",
        "assets/artwork/generated/forest.bmp",
        "assets/artwork/generated/ocean.bmp"
    };
    const char* label_paths[] = {
        "assets/artwork/generated/label-play.bmp",
        "assets/artwork/generated/label-resume.bmp",
        "assets/artwork/generated/label-start-over.bmp"
    };

    for (std::size_t i = 0; i < artwork_textures.size(); ++i) {
        artwork_textures[i] = load_bitmap_texture(renderer_, artwork_paths[i]);
        if (!artwork_textures[i]) {
            shutdown();
            return false;
        }
    }

    for (std::size_t i = 0; i < action_label_textures.size(); ++i) {
        action_label_textures[i] =
            load_bitmap_texture(renderer_, label_paths[i], true);
        if (!action_label_textures[i]) {
            shutdown();
            return false;
        }
    }

    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        if (SDL_IsGameController(index)) {
            open_controller(index);
            if (controller_) {
                break;
            }
        }
    }

    return true;
}

void SDLPlatform::open_controller(int device_index) {
    if (controller_) return;

    controller_ = SDL_GameControllerOpen(device_index);

    if (controller_) {
        controller_instance_ = SDL_JoystickInstanceID(
            SDL_GameControllerGetJoystick(controller_)
        );
    }
}

void SDLPlatform::close_controller() {
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }

    controller_instance_ = -1;
}

Action SDLPlatform::map_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT: return Action::NavigateLeft;
        case SDLK_RIGHT: return Action::NavigateRight;
        case SDLK_UP: return Action::NavigateUp;
        case SDLK_DOWN: return Action::NavigateDown;
        case SDLK_RETURN:
        case SDLK_SPACE: return Action::Confirm;
        case SDLK_ESCAPE: return Action::Back;
        case SDLK_q: return Action::Quit;
        default: return Action::None;
    }
}

Action SDLPlatform::map_controller_button(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return Action::NavigateLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return Action::NavigateRight;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return Action::NavigateUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return Action::NavigateDown;
        case SDL_CONTROLLER_BUTTON_A:
            return Action::Confirm;
        case SDL_CONTROLLER_BUTTON_B:
            return Action::Back;
        default:
            return Action::None;
    }
}

bool SDLPlatform::poll_action(Action& action) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        action = Action::None;

        switch (event.type) {
            case SDL_QUIT:
                action = Action::Quit;
                return true;

            case SDL_KEYDOWN:
                if (event.key.repeat == 0) {
                    action = map_key(event.key.keysym.sym);
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                if (event.cbutton.which == controller_instance_) {
                    action = map_controller_button(event.cbutton.button);
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                open_controller(event.cdevice.which);
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (event.cdevice.which == controller_instance_) {
                    close_controller();
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    action = Action::Quit;
                    return true;
                }
                break;

            default:
                break;
        }

        if (action != Action::None) return true;
    }

    return false;
}

void SDLPlatform::begin_frame() {
    if (!renderer_) return;

    color(renderer_, {8, 15, 35, 255});
    SDL_RenderClear(renderer_);
}

void SDLPlatform::draw_carousel(int selected_index) {
    if (!renderer_) return;

    const int left = (selected_index + 2) % 3;
    const int right = (selected_index + 1) % 3;

    // Neighboring game previews.
    draw_card(renderer_, left,
              {25, 235, 195, 275}, false);

    draw_card(renderer_, right,
              {500, 235, 195, 275}, false);

    // Selected game: large, centered artwork.
    draw_card(renderer_, selected_index,
              {205, 155, 310, 410}, true);

    draw_status(renderer_);
}


namespace {

void draw_action_label(SDL_Renderer* r, int index,
                       int x, int y) {
    if (index < 0 || index >= static_cast<int>(action_label_textures.size())) return;
    SDL_Texture* texture = action_label_textures[static_cast<std::size_t>(index)];
    if (!texture) return;

    SDL_Rect destination{x, y, 350, 72};
    SDL_RenderCopy(r, texture, nullptr, &destination);
}

void draw_play_symbol(SDL_Renderer* r, int x, int y) {
    triangle(r, x, y - 34, x, y + 34,
             x + 58, y, {220, 237, 255, 255});
}

void draw_resume_symbol(SDL_Renderer* r, int x, int y) {
    circle(r, x, y, 30, {220, 237, 255, 255});
    circle(r, x, y, 19, {15, 29, 62, 255});
    triangle(r, x + 10, y - 36,
             x + 42, y - 12,
             x + 7, y - 8,
             {220, 237, 255, 255});
}

void draw_restart_symbol(SDL_Renderer* r, int x, int y) {
    circle(r, x, y, 29, {220, 237, 255, 255});
    circle(r, x, y, 18, {15, 29, 62, 255});
    triangle(r, x - 27, y - 30,
             x + 10, y - 31,
             x - 14, y + 2,
             {220, 237, 255, 255});
}

} // namespace

void SDLPlatform::draw_game_actions(
    int selected_index, bool has_checkpoint, int selected_action) {

    if (!renderer_) return;

    draw_status(renderer_);

    // Keep the selected game's artwork prominent.
    draw_card(renderer_, selected_index,
              {235, 48, 250, 310}, true);

    const int action_count = has_checkpoint ? 2 : 1;
    const int box_height = 112;
    const int gap = 17;
    const int total_height =
        action_count * box_height + (action_count - 1) * gap;
    const int start_y = 405 + (210 - total_height) / 2;

    for (int i = 0; i < action_count; ++i) {
        const int y = start_y + i * (box_height + gap);
        const bool selected = i == selected_action;

        SDL_Rect box{65, y, 590, box_height};

        rounded_fill(renderer_, box, 22,
                     selected
                       ? Color{24, 69, 139, 255}
                       : Color{16, 30, 65, 255});

        rounded_outline(renderer_, box, 22,
                        selected
                          ? Color{75, 165, 255, 255}
                          : Color{65, 93, 145, 190});

        const int symbol_x = 145;
        const int symbol_y = y + box_height / 2;

        if (!has_checkpoint || i == 0) {
            if (has_checkpoint) {
                draw_resume_symbol(renderer_, symbol_x, symbol_y);
                draw_action_label(renderer_, 1, 230, y + 20);
            } else {
                draw_play_symbol(renderer_, symbol_x - 20, symbol_y);
                draw_action_label(renderer_, 0, 230, y + 20);
            }
        } else {
            draw_restart_symbol(renderer_, symbol_x, symbol_y);
            draw_action_label(renderer_, 2, 230, y + 20);
        }

        if (selected) {
            rounded_outline(renderer_,
                            {box.x - 4, box.y - 4,
                             box.w + 8, box.h + 8},
                            26, {43, 126, 231, 180});
        }
    }
}


void SDLPlatform::present() {
    if (renderer_) SDL_RenderPresent(renderer_);
}

void SDLPlatform::shutdown() {
    close_controller();

    for (auto& texture : artwork_textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
    for (auto& texture : action_label_textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (initialized_) {
        SDL_Quit();
        initialized_ = false;
    }
}

} // namespace kiddos
