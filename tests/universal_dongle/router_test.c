#include <assert.h>
#include <stdio.h>
#include <zmk/universal_router.h>
int main(void) {
    struct ud_router r;
    ud_init(&r, UD_HANA);
    assert(ud_assign(&r, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 2, UD_CHOCOFI, UD_LEFT) == UD_OK);
    assert(ud_assign(&r, 3, UD_CHOCOFI, UD_RIGHT) == UD_OK);
    assert(ud_assign(&r, 1, UD_HANA, UD_LEFT) == UD_INVALID);
    assert(ud_connected(&r, 0, true) == UD_OK);
    assert(ud_connected(&r, 1, true) == UD_OK);
    assert(ud_connected(&r, 2, true) == UD_OK);
    assert(ud_accept_position(&r, 0) == UD_OK);
    assert(ud_accept_position(&r, 2) == UD_IGNORED);
    assert(ud_begin_switch(&r, UD_CHOCOFI) == UD_OK);
    assert(ud_accept_position(&r, 0) == UD_IGNORED);
    assert(ud_accept_position(&r, 2) == UD_IGNORED);
    assert(ud_begin_switch(&r, UD_HANA) == UD_BUSY);
    assert(ud_ack_released(&r) == UD_OK);
    assert(ud_finish_switch(&r) == UD_OK);
    assert(ud_accept_position(&r, 0) == UD_IGNORED);
    assert(ud_accept_position(&r, 2) == UD_OK);
    assert(ud_find_slot(&r, UD_CHOCOFI, UD_RIGHT) == 3);
    puts("universal router policy tests passed");
    return 0;
}
