/* Universal dongle OLED status; SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>

void zmk_universal_oled_status_init(lv_obj_t *parent, lv_obj_t *timeout_status_obj);
void zmk_widget_timeout_status_show_keyboard_name(const char *name);
void zmk_universal_oled_status_set_active(uint8_t board_id);
void zmk_universal_oled_status_set_battery(uint8_t board_id, uint8_t side_id, int level);
