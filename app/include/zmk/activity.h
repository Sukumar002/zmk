/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <zephyr/sys/util.h>

enum zmk_activity_state { ZMK_ACTIVITY_ACTIVE, ZMK_ACTIVITY_IDLE, ZMK_ACTIVITY_SLEEP };

enum zmk_activity_state zmk_activity_get_state(void);

/*
 * Runtime activity timeouts. A value of 0 disables the corresponding
 * inactivity transition.
 */
uint32_t zmk_activity_get_idle_timeout_ms(void);
void zmk_activity_set_idle_timeout_ms(uint32_t timeout_ms);

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
uint32_t zmk_activity_get_sleep_timeout_ms(void);
void zmk_activity_set_sleep_timeout_ms(uint32_t timeout_ms);
#endif
