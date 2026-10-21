#include <assert.h>
#include <stdio.h>
#include <zmk/universal_onboarding.h>

int main(void) {
    struct ud_onboarding state;
    ud_onboarding_init(&state, false);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_NEEDS_SETUP);
    assert(ud_onboarding_discovered_count(&state) == 0);
    assert(ud_onboarding_confirm(&state, true) == UD_BUSY);

    /* Discovery order must not matter; these are logical identities, not BLE slots. */
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_CHOCOFI_RIGHT, true) == UD_OK);
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_LEFT, true) == UD_OK);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_DISCOVERING);
    assert(ud_onboarding_pair_ready(&state, UD_CHOCOFI) == false);
    assert(ud_onboarding_pair_ready(&state, UD_HANA) == false);

    assert(ud_onboarding_set_discovered(&state, UD_ROLE_CHOCOFI_LEFT, true) == UD_OK);
    assert(ud_onboarding_pair_ready(&state, UD_CHOCOFI));
    assert(!ud_onboarding_pair_ready(&state, UD_HANA));
    assert(ud_onboarding_confirm(&state, true) == UD_BUSY);

    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_RIGHT, true) == UD_OK);
    assert(ud_onboarding_pair_ready(&state, UD_HANA));
    assert(ud_onboarding_pair_ready(&state, UD_CHOCOFI));
    assert(ud_onboarding_discovered_count(&state) == 4);
    assert(ud_onboarding_confirm(&state, true) == UD_OK);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_READY);

    /* A lost peripheral invalidates setup completion and requires confirmation again. */
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_RIGHT, false) == UD_OK);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_DISCOVERING);
    assert(ud_onboarding_confirm(&state, true) == UD_BUSY);
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_RIGHT, true) == UD_OK);
    assert(ud_onboarding_confirm(&state, true) == UD_OK);

    assert(ud_onboarding_set_discovered(&state, UD_ROLE_COUNT, true) == UD_INVALID);
    assert(ud_onboarding_role_discovered(NULL, UD_ROLE_HANA_LEFT) == false);
    assert(ud_onboarding_pair_ready(&state, (enum ud_board) -1) == false);

    puts("universal dongle onboarding state tests passed");
    return 0;
}
