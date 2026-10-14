/*
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

enum zmk_runtime_timeout_kind {
    ZMK_RUNTIME_TIMEOUT_DISPLAY = 0,
    ZMK_RUNTIME_TIMEOUT_HALVES_SLEEP = 1,
};

enum zmk_runtime_timeout_feedback {
    ZMK_RUNTIME_TIMEOUT_FEEDBACK_TIMED = 0,
    ZMK_RUNTIME_TIMEOUT_FEEDBACK_HOLD = 1,
    ZMK_RUNTIME_TIMEOUT_FEEDBACK_HIDE = 2,
};

struct zmk_runtime_timeout_changed {
    enum zmk_runtime_timeout_kind kind;
    uint32_t timeout_ms;
    enum zmk_runtime_timeout_feedback feedback;
};

ZMK_EVENT_DECLARE(zmk_runtime_timeout_changed);
