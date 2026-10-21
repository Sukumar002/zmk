/* Universal dongle first-launch onboarding state. SPDX-License-Identifier: MIT */
#include <zmk/universal_onboarding.h>

void ud_onboarding_init(struct ud_onboarding *state, bool setup_confirmed) {
    if (!state) {
        return;
    }
    state->discovered_mask = 0;
    state->setup_confirmed = setup_confirmed;
}

int ud_onboarding_set_discovered(struct ud_onboarding *state, uint8_t role, bool discovered) {
    if (!state || role >= UD_ROLE_COUNT) {
        return UD_INVALID;
    }

    if (discovered) {
        state->discovered_mask |= (uint8_t)(1U << role);
    } else {
        state->discovered_mask &= (uint8_t)~(1U << role);
        /* Keep setup confirmation across transient BLE disconnects. */
    }
    return UD_OK;
}

bool ud_onboarding_role_discovered(const struct ud_onboarding *state, uint8_t role) {
    return state && role < UD_ROLE_COUNT &&
           (state->discovered_mask & (uint8_t)(1U << role)) != 0;
}

unsigned ud_onboarding_discovered_count(const struct ud_onboarding *state) {
    if (!state) {
        return 0;
    }

    unsigned count = 0;
    for (uint8_t role = 0; role < UD_ROLE_COUNT; role++) {
        count += ud_onboarding_role_discovered(state, role) ? 1U : 0U;
    }
    return count;
}

bool ud_onboarding_pair_ready(const struct ud_onboarding *state, enum ud_board board) {
    if (!state) {
        return false;
    }

    switch (board) {
    case UD_HANA:
        return ud_onboarding_role_discovered(state, UD_ROLE_HANA_LEFT) &&
               ud_onboarding_role_discovered(state, UD_ROLE_HANA_RIGHT);
    case UD_CHOCOFI:
        return ud_onboarding_role_discovered(state, UD_ROLE_CHOCOFI_LEFT) &&
               ud_onboarding_role_discovered(state, UD_ROLE_CHOCOFI_RIGHT);
    default:
        return false;
    }
}

enum ud_onboarding_status ud_onboarding_get_status(const struct ud_onboarding *state) {
    if (!state) {
        return UD_ONBOARDING_INVALID;
    }
    if (state->setup_confirmed &&
        state->discovered_mask == ((1U << UD_ROLE_COUNT) - 1U)) {
        return UD_ONBOARDING_READY;
    }
    return ud_onboarding_discovered_count(state) == 0 && !state->setup_confirmed
               ? UD_ONBOARDING_NEEDS_SETUP
               : UD_ONBOARDING_DISCOVERING;
}

int ud_onboarding_confirm(struct ud_onboarding *state, bool confirm) {
    if (!state) {
        return UD_INVALID;
    }
    if (!confirm) {
        state->setup_confirmed = false;
        return UD_OK;
    }
    if (state->discovered_mask != ((1U << UD_ROLE_COUNT) - 1U) ||
        !ud_onboarding_pair_ready(state, UD_HANA) ||
        !ud_onboarding_pair_ready(state, UD_CHOCOFI)) {
        return UD_BUSY;
    }
    state->setup_confirmed = true;
    return UD_OK;
}
