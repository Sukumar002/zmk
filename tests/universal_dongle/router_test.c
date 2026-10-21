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

    /* Both native layouts translate into a stable 46-position universal map. */
    uint32_t mapped = UINT32_MAX;
    assert(ud_translate_position(UD_HANA, 0, &mapped) == UD_OK && mapped == 0);
    assert(ud_translate_position(UD_HANA, 45, &mapped) == UD_OK && mapped == 45);
    assert(ud_translate_position(UD_CHOCOFI, 0, &mapped) == UD_OK && mapped == 1);
    assert(ud_translate_position(UD_CHOCOFI, 20, &mapped) == UD_OK && mapped == 25);
    assert(ud_translate_position(UD_CHOCOFI, 25, &mapped) == UD_OK && mapped == 32);
    assert(ud_translate_position(UD_CHOCOFI, 35, &mapped) == UD_OK && mapped == 45);
    assert(ud_translate_position(UD_HANA, 46, &mapped) == UD_INVALID);
    assert(ud_translate_position(UD_CHOCOFI, 36, &mapped) == UD_INVALID);
    assert(ud_translate_position(UD_CHOCOFI, 0, NULL) == UD_INVALID);
    assert(ud_translate_position((enum ud_board) - 1, 0, &mapped) == UD_INVALID);
    bool seen[UD_HANA_POSITION_COUNT] = {false};
    for (uint32_t i = 0; i < UD_CHOCOFI_POSITION_COUNT; i++) {
        assert(ud_translate_position(UD_CHOCOFI, i, &mapped) == UD_OK);
        assert(mapped < UD_HANA_POSITION_COUNT);
        assert(!seen[mapped]);
        seen[mapped] = true;
    }

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

    /* Return focus to Chocofi so the following checks prove that a healthy
     * inactive Hana pair never steals focus merely by being connected. */
    assert(ud_begin_switch(&r, UD_CHOCOFI) == UD_OK);
    finish(&r);
    assert(r.active == UD_CHOCOFI);

    /* Both Hana halves can connect without stealing focus. */
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);

    /* A non-active half appearing/disappearing has no effect on focus. */
    assert(ud_connected(&r, 0, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_IGNORED);
    assert(r.active == UD_CHOCOFI);
    assert(ud_connected(&r, 0, true) == UD_OK);

    /* If an active Chocofi half drops while Hana is complete, request a safe
     * switch immediately; the adapter must drain old HID state before finish. */
    assert(ud_connected(&r, 2, false) == UD_OK);
    assert(ud_reconcile_availability(&r) == UD_OK && r.phase == UD_DRAIN_OLD);
    assert(ud_accept_position(&r, 0) == UD_IGNORED);
    finish(&r);
    assert(r.active == UD_HANA && ud_accept_position(&r, 0) == UD_OK);
    assert(ud_connected(&r, 3, false) == UD_OK);

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
