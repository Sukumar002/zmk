/* Universal dongle routing policy; no BLE or flash side effects. SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
enum ud_board { UD_HANA = 0, UD_CHOCOFI = 1, UD_BOARD_COUNT = 2 };
enum ud_side { UD_LEFT = 0, UD_RIGHT = 1 };
enum ud_switch_phase { UD_READY = 0, UD_DRAIN_OLD, UD_CONNECT_NEW };
enum ud_result { UD_OK = 0, UD_IGNORED = 1, UD_INVALID = -1, UD_BUSY = -2 };
#define UD_SLOTS 4
#define UD_HANA_POSITION_COUNT 46
#define UD_CHOCOFI_POSITION_COUNT 36
#define UD_NO_SLOT 255

/* Protocol v1 uses 1-based board/side IDs. Return a stable logical identity
 * slot, independent of BLE connection order, or UD_NO_SLOT for invalid data. */
static inline uint8_t ud_identity_to_slot(uint8_t version, uint8_t board_id, uint8_t side_id) {
    if (version != 1 || board_id < 1 || board_id > UD_BOARD_COUNT || side_id < 1 || side_id > 2) {
        return UD_NO_SLOT;
    }
    return (uint8_t)(((board_id - 1) * 2) + (side_id - 1));
}
struct ud_slot {
    uint8_t board;
    uint8_t side;
    bool assigned;
    bool connected;
};
struct ud_router {
    struct ud_slot slots[UD_SLOTS];
    enum ud_board active, pending, preferred;
    enum ud_switch_phase phase;
    bool idle;
};
void ud_init(struct ud_router *r, enum ud_board initial);
int ud_assign(struct ud_router *r, uint8_t slot, enum ud_board board, enum ud_side side);
int ud_connected(struct ud_router *r, uint8_t slot, bool connected);
int ud_accept_position(const struct ud_router *r, uint8_t slot);
/* Translate each board's native row-major position index into the 46-position
 * universal keymap. Hana is the reference layout; Chocofi's 36 positions map
 * into the matching keys, leaving Hana-only positions unused. */
int ud_translate_position(enum ud_board board, uint32_t source_position,
                          uint32_t *unified_position);
int ud_begin_switch(struct ud_router *r, enum ud_board target);
int ud_ack_released(struct ud_router *r);
int ud_finish_switch(struct ud_router *r);
/* Abort a pending switch and leave routing idle until a complete profile is usable. */
int ud_cancel_switch(struct ud_router *r);
uint8_t ud_find_slot(const struct ud_router *r, enum ud_board board, enum ud_side side);
unsigned ud_connected_count(const struct ud_router *r, enum ud_board board);
/* Track whether a physical key press was admitted. Releases must be paired
 * with an admitted press even if focus changes in between; an inactive board's
 * release must never release a key owned by the active board. Caller keeps one
 * accepted flag per source slot and physical position. */
static inline int ud_filter_position_event(const struct ud_router *r, uint8_t slot, bool pressed,
                                           bool *accepted) {
    if (!accepted) {
        return UD_INVALID;
    }
    if (pressed) {
        if (*accepted) {
            return UD_IGNORED;
        }
        if (ud_accept_position(r, slot) != UD_OK) {
            return UD_IGNORED;
        }
        *accepted = true;
        return UD_OK;
    }
    if (!*accepted) {
        return UD_IGNORED;
    }
    *accepted = false;
    return UD_OK;
}
/* Returns UD_OK if transition requested; UD_IGNORED if no change needed.
 * Caller performs HID release, BLE link policy and acknowledgements. */
int ud_reconcile_availability(struct ud_router *r);
