#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"

#define SCREEN_W 720
#define SCREEN_H 720
#define BUFFER_ROWS 40

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_GameController *controller;
static uint8_t draw_buffer[SCREEN_W * BUFFER_ROWS * 2];
static lv_indev_t *keypad;
static lv_group_t *focus_group;
static uint32_t keypad_key;
static bool keypad_pressed;

static void read_controller(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->key = keypad_key;
    data->state = keypad_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void flush_to_sdl(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    SDL_Rect rect = {
        .x = area->x1,
        .y = area->y1,
        .w = area->x2 - area->x1 + 1,
        .h = area->y2 - area->y1 + 1
    };

    if (rect.w > 0 && rect.h > 0) {
        SDL_UpdateTexture(texture, &rect, pixels, rect.w * 2);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    lv_display_flush_ready(display);
}

static void make_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x071426), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, 680, 76);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x102B4A), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 18, 0);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "KiddOS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    focus_group = lv_group_create();
    const char *names[] = {"GAME ONE", "GAME TWO", "GAME THREE"};
    const int widths[] = {170, 230, 170};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = lv_obj_create(screen);
        lv_obj_set_size(card, widths[i], i == 1 ? 330 : 270);
        lv_obj_align(card, LV_ALIGN_CENTER, (i - 1) * 205, 25);
        lv_obj_set_style_bg_color(card,
            i == 1 ? lv_color_hex(0x1769AA) : lv_color_hex(0x19334F), 0);
        lv_obj_set_style_border_width(card, i == 1 ? 4 : 0, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x48B4FF), 0);
        lv_obj_set_style_radius(card, 22, 0);
        lv_obj_set_style_border_width(card, 5, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(card, lv_color_hex(0xFFFFFF), LV_STATE_FOCUSED);
        lv_group_add_obj(focus_group, card);

        lv_obj_t *label = lv_label_create(card);
        lv_label_set_text(label, names[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(label);
    }

    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "SDL2 renderer adapter prototype");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x8FAAC5), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -24);
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("KiddOS LVGL SDL Renderer Adapter",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 7, 20, 38, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_NumJoysticks() > 0) {
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    printf("Controller opened: %s\\n", SDL_GameControllerName(controller));
                    break;
                }
            }
        }
    } else {
        printf("No SDL game controller detected; keyboard arrows remain available.\\n");
    }

    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    lv_delay_set_cb(SDL_Delay);

    lv_display_t *display = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flush_to_sdl);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    make_ui();

    keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, read_controller);
    lv_indev_set_group(keypad, focus_group);
    lv_group_focus_obj(lv_obj_get_child(lv_screen_active(), 1));
    lv_obj_t *test_initial_focus = lv_group_get_focused(focus_group);
    bool test_release_sent = false;
    bool test_result_reported = false;
    uint32_t test_started = SDL_GetTicks();

    SDL_Event test_press = {0};
    test_press.type = SDL_KEYDOWN;
    test_press.key.keysym.sym = SDLK_RIGHT;
    SDL_PushEvent(&test_press);



    printf("Adapter running: LVGL -> custom flush callback -> existing SDL_Renderer\n");
    printf("Press Escape or close the window to exit.\n");

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;

            if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: keypad_key = LV_KEY_PREV; keypad_pressed = true; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: keypad_key = LV_KEY_NEXT; keypad_pressed = true; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: keypad_key = LV_KEY_UP; keypad_pressed = true; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: keypad_key = LV_KEY_DOWN; keypad_pressed = true; break;
                    case SDL_CONTROLLER_BUTTON_A: keypad_key = LV_KEY_ENTER; keypad_pressed = true; break;
                    case SDL_CONTROLLER_BUTTON_B: keypad_key = LV_KEY_ESC; keypad_pressed = true; break;
                    default: break;
                }
            }

            if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                    case SDLK_UP:
                    case SDLK_DOWN:
                    case SDLK_RETURN:
                        keypad_pressed = false;
                        break;
                    default: break;
                }
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: keypad_key = LV_KEY_PREV; keypad_pressed = true; break;
                    case SDLK_RIGHT: keypad_key = LV_KEY_NEXT; keypad_pressed = true; break;
                    case SDLK_UP: keypad_key = LV_KEY_UP; keypad_pressed = true; break;
                    case SDLK_DOWN: keypad_key = LV_KEY_DOWN; keypad_pressed = true; break;
                    case SDLK_RETURN: keypad_key = LV_KEY_ENTER; keypad_pressed = true; break;
                    default: break;
                }
            }
        }
        lv_timer_handler();

        if (!test_release_sent && SDL_GetTicks() - test_started >= 200) {
            SDL_Event test_release = {0};
            test_release.type = SDL_KEYUP;
            test_release.key.keysym.sym = SDLK_RIGHT;
            SDL_PushEvent(&test_release);
            test_release_sent = true;
        }

        if (!test_result_reported && SDL_GetTicks() - test_started >= 800) {
            printf("SYNTHETIC_NEXT_NAV=%s\\n",
                   lv_group_get_focused(focus_group) != test_initial_focus ? "PASS" : "FAIL");
            fflush(stdout);
            test_result_reported = true;
        }

        SDL_Delay(5);
    }

    lv_deinit();
    if (controller) SDL_GameControllerClose(controller);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
