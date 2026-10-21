#include <assert.h>
#include <stdio.h>
#include <zmk/universal_onboarding.h>
#include <zmk/universal_router.h>

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

    /* Transient BLE loss preserves confirmation but accurately reports missing hardware. */
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_RIGHT, false) == UD_OK);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_DISCOVERING);
    assert(!ud_onboarding_pair_ready(&state, UD_HANA));
    assert(ud_onboarding_set_discovered(&state, UD_ROLE_HANA_RIGHT, true) == UD_OK);
    assert(ud_onboarding_get_status(&state) == UD_ONBOARDING_READY);

    assert(ud_onboarding_set_discovered(&state, UD_ROLE_COUNT, true) == UD_INVALID);
    assert(ud_onboarding_role_discovered(NULL, UD_ROLE_HANA_LEFT) == false);
    assert(ud_onboarding_pair_ready(&state, (enum ud_board) -1) == false);

    /* Router event filter: active-pair presses and their matching releases only. */
    struct ud_router router;
    ud_init(&router, UD_HANA);
    assert(ud_assign(&router, 0, UD_HANA, UD_LEFT) == UD_OK);
    assert(ud_assign(&router, 1, UD_HANA, UD_RIGHT) == UD_OK);
    assert(ud_assign(&router, 2, UD_CHOCOFI, UD_LEFT) == UD_OK);
    assert(ud_assign(&router, 3, UD_CHOCOFI, UD_RIGHT) == UD_OK);
    assert(ud_connected(&router, 0, true) == UD_OK);
    assert(ud_connected(&router, 1, true) == UD_OK);
    router.idle = false;
    bool hana_key_accepted = false;
    bool chocofi_key_accepted = false;
    assert(ud_filter_position_event(&router, 0, true, &hana_key_accepted) == UD_OK);
    assert(hana_key_accepted);
    assert(ud_filter_position_event(&router, 2, true, &chocofi_key_accepted) == UD_IGNORED);
    assert(!chocofi_key_accepted);
    /* A previously admitted release is allowed even after routing becomes idle. */
    router.idle = true;
    assert(ud_filter_position_event(&router, UD_NO_SLOT, false, &hana_key_accepted) == UD_OK);
    assert(!hana_key_accepted);
    assert(ud_filter_position_event(&router, UD_NO_SLOT, false, &chocofi_key_accepted) ==
           UD_IGNORED);

    puts("universal dongle onboarding and routing tests passed");
    return 0;
}
