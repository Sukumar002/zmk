#include <assert.h>
#include <stdio.h>
#include <zmk/universal_router.h>

static void finish(struct ud_router *r) {
    assert(ud_ack_released(r) == UD_OK);
    assert(ud_finish_switch(r) == UD_OK);
}

int main(void) {
    struct ud_router r;
    ud_init(&r, (enum ud_board) - 1);
    assert(r.idle && r.active == UD_HANA);
    ud_init(&r, UD_CHOCOFI);
    assert(r.idle && r.active == UD_CHOCOFI);
    assert(ud_find_slot(&r, (enum ud_board) - 1, UD_LEFT) == UD_NO_SLOT);
    assert(ud_find_slot(&r, UD_HANA, (enum ud_side) - 1) == UD_NO_SLOT);
    assert(ud_begin_switch(&r, (enum ud_board) - 1) == UD_INVALID);

    /* During a switch, a known disconnected identity may reconnect into its
     * existing slot, but onboarding a new identity remains blocked. */
    struct ud_router reconnect_router;
    ud_init(&reconnect_router, UD_HANA);
    assert(ud_assign(&reconnect_router, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_connected(&reconnect_router, 0, true) == UD_OK);
    assert(ud_connected(&reconnect_router, 0, false) == UD_OK);
    assert(ud_begin_switch(&reconnect_router, UD_CHOCOFI) == UD_OK);
    assert(ud_assign(&reconnect_router, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_assign(&reconnect_router, 1, UD_HANA, UD_RIGHT) == UD_BUSY);

    /* Protocol identities map to logical slots, never BLE connection order. */
    assert(ud_identity_to_slot(1, 1, 1) == 0); /* Hana left */
    assert(ud_identity_to_slot(1, 1, 2) == 1); /* Hana right */
    assert(ud_identity_to_slot(1, 2, 1) == 2); /* Chocofi left */
    assert(ud_identity_to_slot(1, 2, 2) == 3); /* Chocofi right */
    assert(ud_identity_to_slot(2, 1, 1) == UD_NO_SLOT);
    assert(ud_identity_to_slot(1, 0, 1) == UD_NO_SLOT);
    assert(ud_identity_to_slot(1, 3, 1) == UD_NO_SLOT);
    assert(ud_identity_to_slot(1, 1, 0) == UD_NO_SLOT);
    assert(ud_identity_to_slot(1, 1, 3) == UD_NO_SLOT);
    assert(ud_assign(&r, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 2, UD_CHOCOFI, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 3, UD_CHOCOFI, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_LEFT) == UD_INVALID);
    /* An assigned slot cannot be silently rebound to a different peer identity. */
    assert(ud_assign(&r, 0, UD_CHOCOFI, UD_RIGHT) == UD_INVALID);
    assert(ud_find_slot(&r, UD_HANA, UD_LEFT) == 0);
    assert(ud_find_slot(&r, UD_CHOCOFI, UD_RIGHT) == 3);
    assert(ud_assign(&r, 0, (enum ud_board) - 1, UD_LEFT) == UD_INVALID);
    assert(ud_assign(&r, 0, UD_HANA, (enum ud_side) - 1) == UD_INVALID);

    /* One half must not make a keyboard active or trigger a switch. */
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_connected(&r, 2, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_accept_position(&r, 2) == UD_IGNORED);
    assert(ud_connected(&r, 3, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && !r.idle);
    assert(ud_accept_position(&r, 2) == UD_OK);

    /* Input filtering pairs only admitted presses with releases. */
    bool accepted_key = false;
    assert(ud_filter_position_event(&r, 2, false, &accepted_key) == UD_IGNORED);
    assert(!accepted_key);
    assert(ud_filter_position_event(&r, 2, true, &accepted_key) == UD_OK);
    assert(accepted_key);
    assert(ud_filter_position_event(&r, 2, true, &accepted_key) == UD_IGNORED);
    assert(accepted_key);
    /* Focus may change while held: the admitted release still gets through. */
    assert(ud_begin_switch(&r, UD_HANA) == UD_OK);
    assert(ud_filter_position_event(&r, 2, false, &accepted_key) == UD_OK);
    assert(!accepted_key);
    /* A release alone cannot complete a switch before the target pair is ready. */
    assert(ud_ack_released(&r) == UD_OK);
    assert(ud_finish_switch(&r) == UD_BUSY);
    assert(r.active == UD_CHOCOFI);
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_finish_switch(&r) == UD_OK);
    assert(r.active == UD_HANA);

    /* Both Hana halves can connect without stealing focus. */
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);

    /* A non-active half appearing/disappearing has no effect on focus. */
    assert(ud_connected(&r, 3, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);
    assert(ud_connected(&r, 3, true) == UD_OK);

    /* Losing one active half disables input but must not prematurely fail over. */
    assert(ud_connected(&r, 2, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_accept_position(&r, 3) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);

    /* Once the old pair is fully gone, a complete Hana pair can take over. */
    assert(ud_connected(&r, 3, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_OK && r.phase == UD_DRAIN_OLD);
    assert(ud_accept_position(&r, 0) == UD_IGNORED);
    assert(ud_begin_switch(&r, UD_CHOCOFI) == UD_BUSY);
    finish(&r);
    assert(r.active == UD_HANA && ud_accept_position(&r, 0) == UD_OK);

    /* All off: idle. A single Chocofi half cannot trigger failover. */
    assert(ud_connected(&r, 0, false) == UD_OK);
    assert(ud_connected(&r, 1, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_connected(&r, 2, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED && r.idle);
    assert(ud_connected(&r, 3, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_OK);
    finish(&r);
    assert(r.active == UD_CHOCOFI && !r.idle);
    assert(ud_find_slot(&r, UD_CHOCOFI, UD_RIGHT) == 3);

    /* Switch completion is rejected unless both target halves are connected. */
    assert(ud_begin_switch(&r, UD_HANA) == UD_OK);
    assert(ud_ack_released(&r) == UD_OK);
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_finish_switch(&r) == UD_BUSY);
    assert(r.active == UD_CHOCOFI);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_finish_switch(&r) == UD_OK);
    assert(r.active == UD_HANA);

    puts("universal complete-pair routing policy tests passed");
    return 0;
}
