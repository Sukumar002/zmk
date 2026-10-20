/* Universal dongle: testable policy layer, no BLE/ZMK side effects.
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum ud_board { UD_HANA = 0, UD_CHOCOFI = 1, UD_BOARD_COUNT = 2 };
enum ud_side { UD_LEFT = 0, UD_RIGHT = 1 };
enum ud_switch_phase { UD_READY = 0, UD_DRAIN_OLD, UD_CONNECT_NEW };
enum ud_result { UD_OK = 0, UD_IGNORED = 1, UD_INVALID = -1, UD_BUSY = -2 };
#define UD_SLOTS 4
#define UD_NO_SLOT 255

struct ud_slot {
    uint8_t board;
    uint8_t side;
    bool assigned;
    bool connected;
};
struct ud_router {
    struct ud_slot slots[UD_SLOTS];
    enum ud_board active;
    enum ud_board pending;
    enum ud_switch_phase phase;
};
void ud_init(struct ud_router *r, enum ud_board initial);
int ud_assign(struct ud_router *r, uint8_t slot, enum ud_board board, enum ud_side side);
int ud_connected(struct ud_router *r, uint8_t slot, bool connected);
int ud_accept_position(const struct ud_router *r, uint8_t slot);
int ud_begin_switch(struct ud_router *r, enum ud_board target);
int ud_ack_released(struct ud_router *r);
int ud_finish_switch(struct ud_router *r);
uint8_t ud_find_slot(const struct ud_router *r, enum ud_board board, enum ud_side side);
