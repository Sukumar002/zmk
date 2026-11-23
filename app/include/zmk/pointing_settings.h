#pragma once

#include <stdint.h>

#define ZMK_POINTING_MOVE_SPEED_DEFAULT 600
#define ZMK_POINTING_SCROLL_SPEED_DEFAULT 10
#define ZMK_POINTING_ACCEL_DEFAULT 1

uint16_t zmk_pointing_settings_move_speed(void);
uint16_t zmk_pointing_settings_scroll_speed(void);
uint8_t zmk_pointing_settings_acceleration(void);

int zmk_pointing_settings_set(uint16_t move_speed, uint16_t scroll_speed, uint8_t acceleration);
int zmk_pointing_settings_reset(void);
