/* Universal dongle OLED status; SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/universal_dongle_oled_status.h>
#include "timeout_status.h"

#define UD_OLED_UNKNOWN (-1)
#define UD_OLED_NAME_MS 2500

static lv_obj_t *keyboard_icon;
static lv_obj_t *keyboard_number;
static lv_color_t keyboard_icon_buffer[LV_CANVAS_BUF_SIZE(12, 8, LV_COLOR_FORMAT_GET_BPP(LV_COLOR_FORMAT_L8), LV_DRAW_BUF_STRIDE_ALIGN)];
static uint8_t last_name_board;
static lv_obj_t *keyboard_name;
static lv_obj_t *battery_rows[4];
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
    if (!state_initialized || !keyboard_icon || !keyboard_name) {
        return;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    uint8_t board = active_board_id;
    int levels[4] = {battery_levels[1][1], battery_levels[1][2], battery_levels[2][1], battery_levels[2][2]};
    bool name_visible = show_name;
    k_mutex_unlock(&state_mutex);

    char icon_text[12];
    lv_canvas_fill_bg(keyboard_icon, lv_color_black(), LV_OPA_TRANSP);
    for (int x = 0; x < 12; x++) {
        lv_canvas_set_px(keyboard_icon, x, 0, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, x, 7, lv_color_white(), LV_OPA_COVER);
    }
    for (int y = 1; y < 7; y++) {
        lv_canvas_set_px(keyboard_icon, 0, y, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, 11, y, lv_color_white(), LV_OPA_COVER);
    }
    for (int x = 2; x <= 9; x += 2) {
        lv_canvas_set_px(keyboard_icon, x, 2, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, x, 4, lv_color_white(), LV_OPA_COVER);
    }
    for (int x = 3; x <= 8; x++) {
        lv_canvas_set_px(keyboard_icon, x, 6, lv_color_white(), LV_OPA_COVER);
    }
    snprintk(icon_text, sizeof(icon_text), "%u", board);
    lv_label_set_text(keyboard_number, icon_text);
    lv_label_set_text(keyboard_name, "");
    if (name_visible && last_name_board != board) {
        zmk_widget_timeout_status_show_keyboard_name(board == 1 ? "Hana" : "Chocofi");
        last_name_board = board;
    }
    for (int i = 0; i < 4; i++) {
        char text[16];
        uint8_t battery_board = (i < 2) ? 1 : 2;
        uint8_t battery_side = (i % 2) + 1;
        const char *name = battery_board == 1 ? "H" : "C";
        const char *side = battery_side == 1 ? "L" : "R";
        if (levels[i] >= 0 && levels[i] <= 100) {
            snprintk(text, sizeof(text), "%s%s " LV_SYMBOL_BATTERY_3 " %u%%", name, side, levels[i]);
        } else {
            snprintk(text, sizeof(text), "%s%s --", name, side);
        }
        if (battery_rows[i]) {
            lv_label_set_text(battery_rows[i], text);
        }
    }
}

static void hide_name_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    k_mutex_lock(&state_mutex, K_FOREVER);
    show_name = false;
    k_mutex_unlock(&state_mutex);
    k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
}

void zmk_universal_oled_status_init(lv_obj_t *parent, lv_obj_t *timeout_obj) {
    ARG_UNUSED(timeout_obj);
    if (!state_initialized) {
        k_work_init(&refresh_work, render_status);
        k_work_init_delayable(&hide_name_work, hide_name_work_cb);
        state_initialized = true;
    }
    keyboard_icon = lv_canvas_create(parent);
    lv_canvas_set_buffer(keyboard_icon, keyboard_icon_buffer, 12, 8, LV_COLOR_FORMAT_L8);
    lv_obj_align(keyboard_icon, LV_ALIGN_TOP_LEFT, 36, 0);
    keyboard_number = lv_label_create(parent);
    lv_obj_align_to(keyboard_number, keyboard_icon, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    keyboard_name = lv_label_create(parent);
    lv_obj_add_flag(keyboard_name, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < 4; i++) {
        battery_rows[i] = lv_label_create(parent);
        lv_obj_align(battery_rows[i], LV_ALIGN_TOP_RIGHT, 0, i * 10);
    }
    k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
    k_mutex_lock(&state_mutex, K_FOREVER);
    bool name_was_requested = show_name;
    k_mutex_unlock(&state_mutex);
    if (name_was_requested) {
        k_work_reschedule_for_queue(zmk_display_work_q(), &hide_name_work, K_MSEC(UD_OLED_NAME_MS));
    }
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
