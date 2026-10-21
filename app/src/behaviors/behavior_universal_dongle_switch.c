/* SPDX-License-Identifier: MIT */

#define DT_DRV_COMPAT zmk_behavior_universal_dongle_switch

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/universal_router.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int universal_switch_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    if (binding->param1 < 1 || binding->param1 > UD_BOARD_COUNT) {
        LOG_WRN("Invalid universal dongle target board %u", binding->param1);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    int result = zmk_universal_dongle_request_switch((enum ud_board)(binding->param1 - 1));
    if (result != UD_OK && result != UD_IGNORED) {
        LOG_WRN("Universal dongle switch request failed (%d)", result);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int universal_switch_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api universal_switch_driver_api = {
    .binding_pressed = universal_switch_binding_pressed,
    .binding_released = universal_switch_binding_released,
};

#define UNIVERSAL_SWITCH_INST(n)                                                                   \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &universal_switch_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UNIVERSAL_SWITCH_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
