/* Universal dongle OLED status; SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/universal_dongle_oled_status.h>

#define UD_OLED_UNKNOWN (-1)
#define UD_OLED_NAME_MS 2500
#define UD_KEYBOARD_ICON_W 12
#define UD_KEYBOARD_ICON_H 8
#define UD_BATTERY_ICON_W 5
#define UD_BATTERY_ICON_H 8

static lv_obj_t *keyboard_icon;
static lv_obj_t *keyboard_number;
static lv_color_t keyboard_icon_buffer[LV_CANVAS_BUF_SIZE(
    UD_KEYBOARD_ICON_W, UD_KEYBOARD_ICON_H, LV_COLOR_FORMAT_GET_BPP(LV_COLOR_FORMAT_L8),
    LV_DRAW_BUF_STRIDE_ALIGN)];
static uint8_t last_name_board;
static lv_obj_t *keyboard_name;
static lv_obj_t *battery_rows[2];
static lv_obj_t *battery_icons[2];
static lv_color_t battery_icon_buffers[2][LV_CANVAS_BUF_SIZE(
    UD_BATTERY_ICON_W, UD_BATTERY_ICON_H, LV_COLOR_FORMAT_GET_BPP(LV_COLOR_FORMAT_L8),
    LV_DRAW_BUF_STRIDE_ALIGN)];
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

static void draw_keyboard_icon(void) {
    lv_canvas_fill_bg(keyboard_icon, lv_color_black(), LV_OPA_TRANSP);
    for (int x = 0; x < UD_KEYBOARD_ICON_W; x++) {
        lv_canvas_set_px(keyboard_icon, x, 0, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, x, UD_KEYBOARD_ICON_H - 1, lv_color_white(), LV_OPA_COVER);
    }
    for (int y = 1; y < UD_KEYBOARD_ICON_H - 1; y++) {
        lv_canvas_set_px(keyboard_icon, 0, y, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, UD_KEYBOARD_ICON_W - 1, y, lv_color_white(), LV_OPA_COVER);
    }
    for (int x = 2; x <= 9; x += 2) {
        lv_canvas_set_px(keyboard_icon, x, 2, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(keyboard_icon, x, 4, lv_color_white(), LV_OPA_COVER);
    }
    for (int x = 3; x <= 8; x++) {
        lv_canvas_set_px(keyboard_icon, x, 6, lv_color_white(), LV_OPA_COVER);
    }
}

/* Hand-drawn battery glyph, matching the Hana OLED artwork instead of a font symbol. */
static void draw_battery_icon(int index, int level) {
    lv_obj_t *canvas = battery_icons[index];
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

    /* Top terminal and upright outline. */
    lv_canvas_set_px(canvas, 1, 0, lv_color_white(), LV_OPA_COVER);
    lv_canvas_set_px(canvas, 3, 0, lv_color_white(), LV_OPA_COVER);
    for (int x = 0; x < UD_BATTERY_ICON_W; x++) {
        lv_canvas_set_px(canvas, x, 1, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(canvas, x, UD_BATTERY_ICON_H - 1, lv_color_white(), LV_OPA_COVER);
    }
    for (int y = 2; y < UD_BATTERY_ICON_H - 1; y++) {
        lv_canvas_set_px(canvas, 0, y, lv_color_white(), LV_OPA_COVER);
        lv_canvas_set_px(canvas, UD_BATTERY_ICON_W - 1, y, lv_color_white(), LV_OPA_COVER);
    }

    if (level < 0 || level > 100) {
        return;
    }
    int fill_rows = (level * 5 + 99) / 100;
    for (int y = 6; y >= 2 && (6 - y) < fill_rows; y--) {
        for (int x = 1; x <= 3; x++) {
            lv_canvas_set_px(canvas, x, y, lv_color_white(), LV_OPA_COVER);
        }
    }
}

static void render_status(struct k_work *work) {
    ARG_UNUSED(work);
    if (!state_initialized || !keyboard_icon || !keyboard_name) {
        return;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    uint8_t board = active_board_id;
    int levels[2] = {battery_levels[board][1], battery_levels[board][2]};
    bool name_visible = show_name;
    k_mutex_unlock(&state_mutex);

    draw_keyboard_icon();
    char icon_text[12];
    snprintk(icon_text, sizeof(icon_text), "%u", board);
    lv_label_set_text(keyboard_number, icon_text);
    lv_label_set_text(keyboard_name, "");
    if (name_visible && last_name_board != board) {
        zmk_widget_timeout_status_show_keyboard_name(board == 1 ? "HANA" : "CHOCOFI");
        last_name_board = board;
    }
    for (int i = 0; i < 2; i++) {
        char text[8];
        if (levels[i] >= 0 && levels[i] <= 100) {
            snprintk(text, sizeof(text), "%d", levels[i]);
        } else {
            snprintk(text, sizeof(text), "--");
        }
        if (battery_rows[i]) {
            lv_label_set_text(battery_rows[i], text);
        }
        if (battery_icons[i]) {
            draw_battery_icon(i, levels[i]);
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
    lv_canvas_set_buffer(keyboard_icon, keyboard_icon_buffer, UD_KEYBOARD_ICON_W,
                         UD_KEYBOARD_ICON_H, LV_COLOR_FORMAT_L8);
    lv_obj_align(keyboard_icon, LV_ALIGN_TOP_LEFT, 36, 0);
    keyboard_number = lv_label_create(parent);
    lv_obj_align_to(keyboard_number, keyboard_icon, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    keyboard_name = lv_label_create(parent);
    lv_obj_add_flag(keyboard_name, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 2; i++) {
        battery_rows[i] = lv_label_create(parent);
        lv_obj_set_width(battery_rows[i], 20);
        lv_obj_set_style_text_align(battery_rows[i], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_align(battery_rows[i], LV_ALIGN_TOP_RIGHT, -7, i * 10);
        battery_icons[i] = lv_canvas_create(parent);
        lv_canvas_set_buffer(battery_icons[i], battery_icon_buffers[i], UD_BATTERY_ICON_W,
                             UD_BATTERY_ICON_H, LV_COLOR_FORMAT_L8);
        lv_obj_align(battery_icons[i], LV_ALIGN_TOP_RIGHT, 0, i * 10);
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
    k_mutex_unlock(&state_mutex);
    k_work_submit_to_queue(zmk_display_work_q(), &refresh_work);
}
