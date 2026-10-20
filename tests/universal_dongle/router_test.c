#include <assert.h>
#include <stdio.h>
#include <zmk/universal_router.h>
static void finish(struct ud_router *r) {
    assert(ud_ack_released(r) == UD_OK);
    assert(ud_finish_switch(r) == UD_OK);
}
int main(void) {
    struct ud_router r;
    ud_init(&r, UD_CHOCOFI);
    assert(r.idle && r.active == UD_CHOCOFI);
    assert(ud_assign(&r, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 2, UD_CHOCOFI, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 3, UD_CHOCOFI, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_LEFT) == UD_INVALID);
    /* Both off: idle, no event accepted. */
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_accept_position(&r, 2) == UD_IGNORED);
    /* First peripheral wakes Chocofi: automatic readiness. */
    assert(ud_connected(&r, 2, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && !r.idle);
    assert(ud_accept_position(&r, 2) == UD_OK);
    /* Second board available, but no unsolicited focus theft. */
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);
    /* Losing just one half shouldn't force a keyboard change. */
    assert(ud_connected(&r, 3, true) == UD_OK);
    assert(ud_connected(&r, 3, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    /* Chocofi fully disappears: fail over to already-available Hana. */
    assert(ud_connected(&r, 2, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_OK && r.phase == UD_DRAIN_OLD);
    assert(ud_accept_position(&r, 0) == UD_IGNORED);
    assert(ud_begin_switch(&r, UD_CHOCOFI) == UD_BUSY);
    finish(&r);
    assert(r.active == UD_HANA && ud_accept_position(&r, 0) == UD_OK);
    /* All off: idle. */
    assert(ud_connected(&r, 0, false) == UD_OK);
    assert(ud_connected(&r, 1, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    /* Chocofi comes back first, request switch and recover. */
    assert(ud_connected(&r, 2, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_OK);
    finish(&r);
    assert(r.active == UD_CHOCOFI && !r.idle);
    assert(ud_find_slot(&r, UD_CHOCOFI, UD_RIGHT) == 3);
    puts("universal failover and idle policy tests passed");
    return 0;
}
