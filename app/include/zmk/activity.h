/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum zmk_activity_state { ZMK_ACTIVITY_ACTIVE, ZMK_ACTIVITY_IDLE, ZMK_ACTIVITY_SLEEP };

enum zmk_activity_state zmk_activity_get_state(void);

/* Update the inactivity thresholds used by the activity monitor. */
void zmk_activity_set_idle_timeout_ms(uint32_t timeout_ms);
void zmk_activity_set_sleep_timeout_ms(uint32_t timeout_ms);
