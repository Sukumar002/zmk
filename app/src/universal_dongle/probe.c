/* Universal storage boot probe: read-only, experimental only.
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zmk/universal_settings.h>
#include <zmk/universal_store.h>
#include <zmk/universal_router.h>

LOG_MODULE_REGISTER(ud_probe, LOG_LEVEL_INF);
static struct ud_store scoped_config;
static struct ud_router routing_probe;

static int universal_storage_probe(void) {
    ud_init(&routing_probe, UD_CHOCOFI);
    (void)ud_assign(&routing_probe, 0, UD_HANA, UD_LEFT);
    (void)ud_assign(&routing_probe, 1, UD_HANA, UD_RIGHT);
    (void)ud_assign(&routing_probe, 2, UD_CHOCOFI, UD_LEFT);
    (void)ud_assign(&routing_probe, 3, UD_CHOCOFI, UD_RIGHT);
    int ret = ud_settings_load(&scoped_config);
    if (ret) {
        LOG_WRN("Universal settings read-only probe returned %d", ret);
        return 0; /* Experimental diagnostic must not prevent keyboard boot. */
    }
    LOG_INF("Universal config records: Hana keymap rev %u, Chocofi keymap rev %u",
            scoped_config.values[UD_STORE_HANA][UD_STORE_KEYMAP].revision,
            scoped_config.values[UD_STORE_CHOCOFI][UD_STORE_KEYMAP].revision);
    return 0;
}
SYS_INIT(universal_storage_probe, APPLICATION, 96);
