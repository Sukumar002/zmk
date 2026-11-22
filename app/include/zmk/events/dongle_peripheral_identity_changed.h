/*
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/split/central.h>

struct zmk_dongle_peripheral_identity_changed {
    uint8_t source;
    enum zmk_split_peripheral_side side;
    bool identity_valid;
    bool connected;
};

ZMK_EVENT_DECLARE(zmk_dongle_peripheral_identity_changed);
