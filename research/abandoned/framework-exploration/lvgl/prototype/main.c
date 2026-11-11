#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl.h"
#include "lvgl/drivers/sdl/lv_sdl_window.h"

#include <SDL.h>

#define SCREEN_W 720
#define SCREEN_H 720
#define GAME_COUNT 3

static const char *game_names[GAME_COUNT] = {
    "Super Mario World",
    "Sonic the Hedgehog",
    "The Legend of Zelda"
};

static lv_obj_t *cards[GAME_COUNT];
static lv_obj_t *card_titles[GAME_COUNT];
static lv_obj_t *selected_title;
static lv_group_t *game_group;
static int selected_index = 0;

static bool controller_left;
static bool controller_right;
static bool controller_confirm;
static bool controller_back;

static int controller_event_watch(void *userdata, SDL_Event *event)
{
    (void)userdata;

    if(event->type == SDL_CONTROLLERBUTTONDOWN ||
       event->type == SDL_CONTROLLERBUTTONUP) {
        bool down = event->type == SDL_CONTROLLERBUTTONDOWN;
        switch(event->cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  controller_left = down; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: controller_right = down; break;
            case SDL_CONTROLLER_BUTTON_A:          controller_confirm = down; break;
            case SDL_CONTROLLER_BUTTON_B:          controller_back = down; break;
            default: break;
        }
    }

    if(event->type == SDL_CONTROLLERAXISMOTION &&
       event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
        controller_left = event->caxis.value < -12000;
        controller_right = event->caxis.value > 12000;
    }

    return 1;
}

static void controller_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static bool left_sent;
    static bool right_sent;

    data->state = LV_INDEV_STATE_RELEASED;
    data->key = 0;

    if(controller_left && !left_sent) {
        data->key = LV_KEY_LEFT;
        data->state = LV_INDEV_STATE_PRESSED;
        left_sent = true;
    } else if(!controller_left) {
        left_sent = false;
    }

    if(controller_right && !right_sent) {
        data->key = LV_KEY_RIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
        right_sent = true;
    } else if(!controller_right) {
        right_sent = false;
    }

    if(controller_confirm) {
        data->key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
    }

    if(controller_back) {
        data->key = LV_KEY_ESC;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

static void refresh_carousel(void)
{
    const int left = (selected_index + GAME_COUNT - 1) % GAME_COUNT;
    const int right = (selected_index + 1) % GAME_COUNT;

    lv_obj_set_pos(cards[selected_index], 220, 145);
    lv_obj_set_size(cards[selected_index], 280, 370);
    lv_obj_set_style_bg_color(cards[selected_index], lv_color_hex(0x1769D2), 0);
    lv_obj_set_style_border_width(cards[selected_index], 4, 0);
    lv_obj_set_style_border_color(cards[selected_index], lv_color_hex(0x55A8FF), 0);

    lv_obj_set_pos(cards[left], 20, 195);
    lv_obj_set_size(cards[left], 190, 270);
    lv_obj_set_style_bg_color(cards[left], lv_color_hex(0x172B49), 0);
    lv_obj_set_style_border_width(cards[left], 1, 0);

    lv_obj_set_pos(cards[right], 510, 195);
    lv_obj_set_size(cards[right], 190, 270);
    lv_obj_set_style_bg_color(cards[right], lv_color_hex(0x172B49), 0);
    lv_obj_set_style_border_width(cards[right], 1, 0);

    lv_label_set_text(selected_title, game_names[selected_index]);
    lv_obj_set_style_text_color(selected_title, lv_color_hex(0xFFFFFF), 0);
}

static void card_focused(lv_event_t *event)
{
    int index = (int)(intptr_t)lv_event_get_user_data(event);
    selected_index = index;
    refresh_carousel();
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x081426), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *heading = lv_label_create(screen);
    lv_label_set_text(heading, "KIDDOS");
    lv_obj_set_style_text_color(heading, lv_color_hex(0x65B5FF), 0);
    lv_obj_set_style_text_font(heading, LV_FONT_DEFAULT, 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *sound = lv_label_create(screen);
    lv_label_set_text(sound, "♪");
    lv_obj_set_style_text_color(sound, lv_color_hex(0x8DBEFF), 0);
    lv_obj_align(sound, LV_ALIGN_TOP_LEFT, 25, 25);

    lv_obj_t *battery = lv_label_create(screen);
    lv_label_set_text(battery, "▰");
    lv_obj_set_style_text_color(battery, lv_color_hex(0x8DBEFF), 0);
    lv_obj_align(battery, LV_ALIGN_TOP_RIGHT, -25, 25);

    selected_title = lv_label_create(screen);
    lv_obj_set_style_text_font(selected_title, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(selected_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(selected_title, LV_ALIGN_BOTTOM_MID, 0, -65);

    game_group = lv_group_create();
    lv_group_set_wrap(game_group, true);

    for(int i = 0; i < GAME_COUNT; i++) {
        cards[i] = lv_button_create(screen);
        lv_obj_set_style_radius(cards[i], 20, 0);
        lv_obj_set_style_bg_opa(cards[i], LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_width(cards[i], 18, 0);
        lv_obj_set_style_shadow_opa(cards[i], LV_OPA_30, 0);
        lv_obj_set_style_pad_all(cards[i], 12, 0);

        card_titles[i] = lv_label_create(cards[i]);
        lv_label_set_text(card_titles[i], game_names[i]);
        lv_obj_set_width(card_titles[i], LV_PCT(100));
        lv_label_set_long_mode(card_titles[i], LV_LABEL_LONG_WRAP);
        lv_obj_center(card_titles[i]);

        lv_obj_add_event_cb(cards[i], card_focused, LV_EVENT_FOCUSED,
                            (void *)(intptr_t)i);
        lv_group_add_obj(game_group, cards[i]);
    }

    refresh_carousel();
    lv_group_focus_obj(cards[selected_index]);
}

int main(int argc, char **argv)
{
    bool smoke_test = argc > 1 && strcmp(argv[1], "--smoke-test") == 0;

    lv_init();

    lv_display_t *display = lv_sdl_window_create(SCREEN_W, SCREEN_H);
    if(!display) {
        fprintf(stderr, "Could not create LVGL SDL display\n");
        return 1;
    }

    lv_sdl_window_set_title(display, "Kiddos - LVGL Prototype");
    lv_sdl_window_set_resizeable(display, false);


    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerEventState(SDL_ENABLE);
    SDL_AddEventWatch(controller_event_watch, NULL);

    for(int i = 0; i < SDL_NumJoysticks(); i++) {
        if(SDL_IsGameController(i)) {
            SDL_GameControllerOpen(i);
        }
    }

    build_ui();

    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, controller_read);
    lv_indev_set_group(keypad, game_group);

    if(smoke_test) {
        uint32_t start = SDL_GetTicks();
        while(SDL_GetTicks() - start < 1200) {
            lv_timer_handler();
            SDL_Delay(5);
        }
    } else {
        while(true) {
            lv_timer_handler();
            SDL_Delay(5);
        }
    }

    SDL_DelEventWatch(controller_event_watch, NULL);
    lv_sdl_quit();
    return 0;
}
