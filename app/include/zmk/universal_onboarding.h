/* Universal dongle first-launch onboarding state. SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zmk/universal_router.h>

/* Controller roles are stable logical identities, never BLE connection indexes. */
enum ud_controller_role {
    UD_ROLE_HANA_LEFT = 0,
    UD_ROLE_HANA_RIGHT,
    UD_ROLE_CHOCOFI_LEFT,
    UD_ROLE_CHOCOFI_RIGHT,
    UD_ROLE_COUNT,
};

enum ud_onboarding_status {
    UD_ONBOARDING_NEEDS_SETUP = 0,
    UD_ONBOARDING_DISCOVERING,
    UD_ONBOARDING_READY,
    UD_ONBOARDING_INVALID,
};

struct ud_onboarding {
    uint8_t discovered_mask;
    bool setup_confirmed;
};

void ud_onboarding_init(struct ud_onboarding *state, bool setup_confirmed);
int ud_onboarding_set_discovered(struct ud_onboarding *state, uint8_t role, bool discovered);
bool ud_onboarding_role_discovered(const struct ud_onboarding *state, uint8_t role);
unsigned ud_onboarding_discovered_count(const struct ud_onboarding *state);
bool ud_onboarding_pair_ready(const struct ud_onboarding *state, enum ud_board board);
enum ud_onboarding_status ud_onboarding_get_status(const struct ud_onboarding *state);
int ud_onboarding_confirm(struct ud_onboarding *state, bool confirm);
