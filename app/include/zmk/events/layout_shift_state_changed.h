/*
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <zmk/event_manager.h>

struct zmk_layout_shift_state_changed {
    bool active;
};

ZMK_EVENT_DECLARE(zmk_layout_shift_state_changed);
