/* Universal dongle routing policy: BLE-agnostic and flash-agnostic.
 * SPDX-License-Identifier: MIT
 */
#include <zmk/universal_router.h>
#include <stddef.h>
#include <string.h>

void ud_init(struct ud_router *r, enum ud_board initial) {
    if (!r) return;
    memset(r, 0, sizeof(*r));
    r->active = initial < UD_BOARD_COUNT ? initial : UD_HANA;
    r->pending = r->active;
}
uint8_t ud_find_slot(const struct ud_router *r, enum ud_board board, enum ud_side side) {
    if (!r) return UD_NO_SLOT;
    for (uint8_t i = 0; i < UD_SLOTS; i++) {
        if (r->slots[i].assigned && r->slots[i].board == board && r->slots[i].side == side)
            return i;
    }
    return UD_NO_SLOT;
}
int ud_assign(struct ud_router *r, uint8_t slot, enum ud_board board, enum ud_side side) {
    if (!r || slot >= UD_SLOTS || board >= UD_BOARD_COUNT || side > UD_RIGHT) return UD_INVALID;
    if (r->phase != UD_READY) return UD_BUSY;
    uint8_t old = ud_find_slot(r, board, side);
    if (old != UD_NO_SLOT && old != slot) return UD_INVALID;
    if (r->slots[slot].connected) return UD_BUSY;
    r->slots[slot] = (struct ud_slot){.board=board,.side=side,.assigned=true,.connected=false};
    return UD_OK;
}
int ud_connected(struct ud_router *r, uint8_t slot, bool connected) {
    if (!r || slot >= UD_SLOTS || !r->slots[slot].assigned) return UD_INVALID;
    r->slots[slot].connected = connected;
    return UD_OK;
}
int ud_accept_position(const struct ud_router *r, uint8_t slot) {
    if (!r || slot >= UD_SLOTS || r->phase != UD_READY) return UD_IGNORED;
    const struct ud_slot *s = &r->slots[slot];
    return s->assigned && s->connected && s->board == r->active ? UD_OK : UD_IGNORED;
}
int ud_begin_switch(struct ud_router *r, enum ud_board target) {
    if (!r || target >= UD_BOARD_COUNT) return UD_INVALID;
    if (r->phase != UD_READY) return UD_BUSY;
    if (target == r->active) return UD_OK;
    r->pending = target;
    r->phase = UD_DRAIN_OLD;
    return UD_OK;
}
int ud_ack_released(struct ud_router *r) {
    if (!r || r->phase != UD_DRAIN_OLD) return UD_INVALID;
    r->phase = UD_CONNECT_NEW;
    return UD_OK;
}
int ud_finish_switch(struct ud_router *r) {
    if (!r || r->phase != UD_CONNECT_NEW) return UD_INVALID;
    r->active = r->pending;
    r->phase = UD_READY;
    return UD_OK;
}
