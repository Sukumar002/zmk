/* Universal dongle OLED status; SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/universal_dongle_oled_status.h>

#define UD_OLED_UNKNOWN (-1)
#define UD_OLED_NAME_MS 2500

static lv_obj_t *keyboard_icon;
static lv_obj_t *keyboard_name;
static lv_obj_t *battery_left;
static lv_obj_t *battery_right;
static struct k_work_delayable hide_name_work;
static struct k_work refresh_work;
K_MUTEX_DEFINE(state_mutex);
static bool state_initialized;
static uint8_t active_board_id = 1;
static int battery_levels[3][3] = {
    {UD_OLED_UNKNOWN, UD_OLED_UNKNOWN, UD_OLED_UNKNOWN},
    {UD_OLED_UNKNOWN, UD_OLED_UNKNOWN, UD_OLED_UNKNOWN},
    {UD_OLED_UNKNOWN, UD_OLED_UNKNOWN, UD_OLED_UNKNOWN},
};
static bool show_name;

static void render_status(struct k_work *work) {
    ARG_UNUSED(work);
    if (!state_initialized || !keyboard_icon || !keyboard_name || !battery_left || !battery_right) {
        return;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    uint8_t board = active_board_id;
    int left = battery_levels[board][1];
    int right = battery_levels[board][2];
    bool name_visible = show_name;
    k_mutex_unlock(&state_mutex);

    char icon_text[12];
    snprintk(icon_text, sizeof(icon_text), LV_SYMBOL_KEYBOARD " %u", board);
    lv_label_set_text(keyboard_icon, icon_text);
    lv_label_set_text(keyboard_name, name_visible ? (board == 1 ? "Hana" : "Chocofi") : "");
    char left_text[12], right_text[12];
    if (left >= 0 && left <= 100) {
        snprintk(left_text, sizeof(left_text), LV_SYMBOL_BATTERY_3 " %u%%", left);
    } else {
        snprintk(left_text, sizeof(left_text), "--");
    }
    if (right >= 0 && right <= 100) {
        snprintk(right_text, sizeof(right_text), LV_SYMBOL_BATTERY_3 " %u%%", right);
    } else {
        snprintk(right_text, sizeof(right_text), "--");
    }
    lv_label_set_text(battery_left, left_text);
    lv_label_set_text(battery_right, right_text);
}

static void hide_name_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    k_mutex_lock(&state_mutex, K_FOREVER);
    show_name = false;
    k_mutex_unlock(&state_mutex);
    k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
}

void zmk_universal_oled_status_init(lv_obj_t *parent) {
    if (!state_initialized) {
        k_work_init(&refresh_work, render_status);
        k_work_init_delayable(&hide_name_work, hide_name_work_cb);
        state_initialized = true;
    }
    keyboard_icon = lv_label_create(parent);
    lv_obj_align(keyboard_icon, LV_ALIGN_TOP_LEFT, 48, 0);
    keyboard_name = lv_label_create(parent);
    lv_obj_set_style_text_font(keyboard_name, lv_theme_get_font_large(parent), LV_PART_MAIN);
    lv_obj_set_style_text_align(keyboard_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(keyboard_name, LV_ALIGN_CENTER, 0, 0);
    battery_left = lv_label_create(parent);
    battery_right = lv_label_create(parent);
    lv_obj_align(battery_right, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_align(battery_left, LV_ALIGN_TOP_RIGHT, -42, 0);
    k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
}

void zmk_universal_oled_status_set_active(uint8_t board_id) {
    if (board_id < 1 || board_id > 2) {
        return;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    active_board_id = board_id;
    show_name = true;
    k_mutex_unlock(&state_mutex);
    if (state_initialized) {
        k_work_reschedule_for_queue(zmk_display_work_q(), &hide_name_work, K_MSEC(UD_OLED_NAME_MS));
        k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
    }
}

void zmk_universal_oled_status_set_battery(uint8_t board_id, uint8_t side_id, int level) {
    if (board_id < 1 || board_id > 2 || side_id < 1 || side_id > 2) {
        return;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    battery_levels[board_id][side_id] = (level >= 0 && level <= 100) ? level : UD_OLED_UNKNOWN;
    bool active = active_board_id == board_id;
    k_mutex_unlock(&state_mutex);
    if (active) {
        k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
    }
}
