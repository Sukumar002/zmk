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

struct zmk_runtime_timeout_changed {
    enum zmk_runtime_timeout_kind kind;
    uint32_t timeout_ms;
};

ZMK_EVENT_DECLARE(zmk_runtime_timeout_changed);
