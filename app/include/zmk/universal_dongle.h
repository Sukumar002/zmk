/*
 * Experimental universal dongle interface.
 * SPDX-License-Identifier: MIT
 *
 * This header intentionally has no linked implementation yet. The feature
 * must not affect standard Hana or Chocofi builds until explicitly enabled.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum zmk_universal_board {
    ZMK_UNIVERSAL_BOARD_HANA = 0,
    ZMK_UNIVERSAL_BOARD_CHOCOFI = 1,
};

enum zmk_universal_side {
    ZMK_UNIVERSAL_SIDE_LEFT = 0,
    ZMK_UNIVERSAL_SIDE_RIGHT = 1,
};

struct zmk_universal_identity {
    enum zmk_universal_board board;
    enum zmk_universal_side side;
    uint8_t paired_slot;
};

struct zmk_universal_switch_status {
    enum zmk_universal_board selected;
    bool switching;
    bool left_connected;
    bool right_connected;
};

/*
 * Proposed API contract:
 * - Board identities are validated before event routing.
 * - Switching must release all pressed HID keys and discard pending holds.
 * - Registry must not use connection-order as a left/right identity.
 * - Implementations will be added in stages behind a default-off Kconfig.
 */
int zmk_universal_request_switch(enum zmk_universal_board target);
int zmk_universal_route_position(uint8_t peripheral_slot, uint32_t position, bool pressed);
int zmk_universal_get_switch_status(struct zmk_universal_switch_status *status);
