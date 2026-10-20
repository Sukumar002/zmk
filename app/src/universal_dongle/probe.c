/* Universal storage boot probe: read-only, experimental only.
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zmk/universal_settings.h>
#include <zmk/universal_store.h>

LOG_MODULE_REGISTER(ud_probe, LOG_LEVEL_INF);
static struct ud_store scoped_config;

static int universal_storage_probe(void) {
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
