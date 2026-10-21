/* Universal dongle policy, BLE-agnostic. SPDX-License-Identifier: MIT */
#include <zmk/universal_router.h>
#include <stddef.h>
#include <string.h>

static bool ud_valid_board(enum ud_board board) {
    return board >= UD_HANA && board < UD_BOARD_COUNT;
}

static bool ud_valid_side(enum ud_side side) { return side >= UD_LEFT && side <= UD_RIGHT; }

/*
 * Chocofi native order is 10 top-row keys, 10 home-row keys, 10 bottom-row
 * keys, then 6 thumbs. Map those into Hana's 46-position row-major superset:
 * rows 0/1 use the inner 10 positions, row 2 skips Hana's two center media
 * positions, and thumbs occupy the six inner thumb positions.
 */
static const uint8_t chocofi_to_universal[UD_CHOCOFI_POSITION_COUNT] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    25, 26, 27, 28, 29, 32, 33, 34, 35, 36,
    39, 40, 41, 43, 44, 45,
};

int ud_translate_position(enum ud_board board, uint32_t source_position, uint32_t *unified_position) {
    if (!unified_position) {
        return UD_INVALID;
    }
    if (board == UD_HANA) {
        if (source_position >= UD_HANA_POSITION_COUNT) {
            return UD_INVALID;
        }
        *unified_position = source_position;
        return UD_OK;
    }
    if (board == UD_CHOCOFI) {
        if (source_position >= UD_CHOCOFI_POSITION_COUNT) {
            return UD_INVALID;
        }
        *unified_position = chocofi_to_universal[source_position];
        return UD_OK;
    }
    return UD_INVALID;
}

static bool ud_board_ready(const struct ud_router *r, enum ud_board board) {
    if (!r || !ud_valid_board(board)) {
        return false;
    }

    bool left = false;
    bool right = false;
    for (size_t i = 0; i < UD_SLOTS; i++) {
        const struct ud_slot *slot = &r->slots[i];
        if (!slot->assigned || !slot->connected || slot->board != board) {
            continue;
        }
        if (slot->side == UD_LEFT) {
            left = true;
        } else if (slot->side == UD_RIGHT) {
            right = true;
        }
    }

    return left && right;
}

void ud_init(struct ud_router *r, enum ud_board initial) {
    if (!r) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->active = ud_valid_board(initial) ? initial : UD_HANA;
    r->preferred = r->pending = r->active;
    r->idle = true;
}

uint8_t ud_find_slot(const struct ud_router *r, enum ud_board board, enum ud_side side) {
    if (!r || !ud_valid_board(board) || !ud_valid_side(side)) {
        return UD_NO_SLOT;
    }
    for (uint8_t i = 0; i < UD_SLOTS; i++) {
        if (r->slots[i].assigned && r->slots[i].board == board && r->slots[i].side == side) {
            return i;
        }
    }
    return UD_NO_SLOT;
}

unsigned ud_connected_count(const struct ud_router *r, enum ud_board board) {
    if (!r || !ud_valid_board(board)) {
        return 0;
    }
    unsigned n = 0;
    for (unsigned i = 0; i < UD_SLOTS; i++) {
        if (r->slots[i].assigned && r->slots[i].board == board && r->slots[i].connected) {
            n++;
        }
    }
    return n;
}

int ud_assign(struct ud_router *r, uint8_t slot, enum ud_board board, enum ud_side side) {
    if (!r || slot >= UD_SLOTS || !ud_valid_board(board) || !ud_valid_side(side)) {
        return UD_INVALID;
    }
    /* A slot is a persisted identity mapping, not a connection-order index.
     * Rebinding an assigned slot could silently route a bonded peer as another board. */
    if (r->slots[slot].assigned && (r->slots[slot].board != board || r->slots[slot].side != side)) {
        return UD_INVALID;
    }

    uint8_t old = ud_find_slot(r, board, side);
    if (old != UD_NO_SLOT && old != slot) {
        return UD_INVALID;
    }
    if (r->slots[slot].connected) {
        return UD_BUSY;
    }

    /* Reconnecting a previously assigned identity is safe during a switch:
     * it restores the same persisted mapping and cannot introduce a new peer. */
    if (r->slots[slot].assigned) {
        return UD_OK;
    }
    if (r->phase != UD_READY) {
        return UD_BUSY;
    }

    r->slots[slot] =
        (struct ud_slot){.board = board, .side = side, .assigned = true, .connected = false};
    return UD_OK;
}

int ud_connected(struct ud_router *r, uint8_t slot, bool connected) {
    if (!r || slot >= UD_SLOTS || !r->slots[slot].assigned) {
        return UD_INVALID;
    }
    r->slots[slot].connected = connected;
    return UD_OK;
}

int ud_accept_position(const struct ud_router *r, uint8_t slot) {
    if (!r || slot >= UD_SLOTS || r->idle || r->phase != UD_READY ||
        !ud_board_ready(r, r->active)) {
        return UD_IGNORED;
    }
    const struct ud_slot *s = &r->slots[slot];
    return s->assigned && s->connected && s->board == r->active ? UD_OK : UD_IGNORED;
}

int ud_begin_switch(struct ud_router *r, enum ud_board target) {
    if (!r || !ud_valid_board(target)) {
        return UD_INVALID;
    }
    if (r->phase != UD_READY) {
        return UD_BUSY;
    }
    if (target == r->active && !r->idle) {
        return UD_OK;
    }
    r->pending = target;
    r->phase = UD_DRAIN_OLD;
    return UD_OK;
}

int ud_ack_released(struct ud_router *r) {
    if (!r || r->phase != UD_DRAIN_OLD) {
        return UD_INVALID;
    }
    r->phase = UD_CONNECT_NEW;
    return UD_OK;
}

int ud_finish_switch(struct ud_router *r) {
    if (!r || r->phase != UD_CONNECT_NEW) {
        return UD_INVALID;
    }
    /* Never activate a keyboard until both identified halves are connected. */
    if (!ud_board_ready(r, r->pending)) {
        return UD_BUSY;
    }
    r->active = r->pending;
    r->idle = false;
    r->phase = UD_READY;
    return UD_OK;
}

int ud_reconcile_availability(struct ud_router *r) {
    if (!r) {
        return UD_INVALID;
    }
    if (r->phase != UD_READY) {
        return UD_BUSY;
    }

    enum ud_board other = (r->active == UD_HANA) ? UD_CHOCOFI : UD_HANA;
    if (ud_board_ready(r, r->active)) {
        r->idle = false;
        return UD_IGNORED; /* Never steal focus when the active pair is ready. */
    }

    /* Fail over whenever the active pair is unavailable and the other pair is
     * complete. A single surviving half cannot safely run a split keymap; waiting
     * for both active halves to disappear would leave the host unusable after a
     * one-half disconnect or battery failure. The adapter must drain HID state
     * before completing this transition. */
    if (ud_board_ready(r, other)) {
        return ud_begin_switch(r, other); /* Caller performs safe release + transition. */
    }

    r->idle = true; /* Neither board is complete; adapter continues scanning. */
    return UD_IGNORED;
}
