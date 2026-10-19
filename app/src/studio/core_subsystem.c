/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <pb_encode.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/studio/core.h>
#include <zmk/studio/rpc.h>

ZMK_RPC_SUBSYSTEM(core)

#define CORE_RESPONSE(type, ...) ZMK_RPC_RESPONSE(core, type, __VA_ARGS__)

static bool encode_device_info_name(pb_ostream_t *stream, const pb_field_t *field,
                                    void *const *arg) {
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, CONFIG_ZMK_KEYBOARD_NAME, strlen(CONFIG_ZMK_KEYBOARD_NAME));
}

#if IS_ENABLED(CONFIG_HWINFO)
static bool encode_device_info_serial_number(pb_ostream_t *stream, const pb_field_t *field,
                                             void *const *arg) {
    uint8_t id_buffer[32];
    const ssize_t id_size = hwinfo_get_device_id(id_buffer, ARRAY_SIZE(id_buffer));

    if (id_size <= 0) {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, id_buffer, id_size);
}
#endif // IS_ENABLED(CONFIG_HWINFO)

static bool encode_status_layer_name(pb_ostream_t *stream, const pb_field_t *field,
                                     void *const *arg) {
    const uint8_t layer = zmk_keymap_highest_layer_active();
    const char *name = zmk_keymap_layer_name(layer);

    if (!name || name[0] == '\0') {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, name, strlen(name));
}

static uint8_t cached_hid_indicators;

#if DT_NODE_EXISTS(DT_NODELABEL(layout_shift_map_gui_alt))
static const struct device *const studio_gui_alt_layout_shift =
    DEVICE_DT_GET(DT_NODELABEL(layout_shift_map_gui_alt));

struct studio_layout_shift_map_data_view {
    zmk_mod_flags_t modifier_map[8];
    bool active;
};

static zmk_core_StatusOsMode studio_status_os_mode(void) {
    if (!device_is_ready(studio_gui_alt_layout_shift)) {
        return zmk_core_StatusOsMode_ZMK_STUDIO_STATUS_OS_MODE_UNKNOWN;
    }

    const struct studio_layout_shift_map_data_view *data = studio_gui_alt_layout_shift->data;

    return data->active ? zmk_core_StatusOsMode_ZMK_STUDIO_STATUS_OS_MODE_WINDOWS
                        : zmk_core_StatusOsMode_ZMK_STUDIO_STATUS_OS_MODE_MACOS;
}
#else
static zmk_core_StatusOsMode studio_status_os_mode(void) {
    return zmk_core_StatusOsMode_ZMK_STUDIO_STATUS_OS_MODE_UNKNOWN;
}
#endif

static zmk_core_StatusSnapshot get_status_snapshot(void) {
    zmk_core_StatusSnapshot status = zmk_core_StatusSnapshot_init_zero;
    const uint8_t active_layer = zmk_keymap_highest_layer_active();
    const struct zmk_endpoint_instance endpoint = zmk_endpoint_get_selected();

    status.active_layer = active_layer;
    status.layer_name.funcs.encode = encode_status_layer_name;
    status.modifiers = zmk_hid_get_explicit_mods();
    status.connected = endpoint.transport != ZMK_TRANSPORT_NONE;
    status.hid_indicators = cached_hid_indicators;
    status.left_battery = -1;
    status.right_battery = -1;
    status.dongle_battery = -1;
    status.os_mode = studio_status_os_mode();

    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        status.transport = zmk_core_StatusTransport_ZMK_STUDIO_STATUS_TRANSPORT_USB;
        break;
    case ZMK_TRANSPORT_BLE:
        status.transport = zmk_core_StatusTransport_ZMK_STUDIO_STATUS_TRANSPORT_BLE;
        break;
    default:
        status.transport = zmk_core_StatusTransport_ZMK_STUDIO_STATUS_TRANSPORT_NONE;
        break;
    }

#if IS_ENABLED(CONFIG_ZMK_USB)
    status.usb_hid_ready = zmk_usb_is_hid_ready();
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
    status.ble_profile = zmk_ble_active_profile_index();
    status.ble_profile_connected = zmk_ble_active_profile_is_connected();
    status.ble_profile_bonded = !zmk_ble_active_profile_is_open();
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) &&                             \
    IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    uint8_t battery_level;
    if (zmk_split_central_get_peripheral_battery_level(0, &battery_level) == 0) {
        status.left_battery = battery_level;
    }
#if ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT > 1
    if (zmk_split_central_get_peripheral_battery_level(1, &battery_level) == 0) {
        status.right_battery = battery_level;
    }
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_STUDIO_STATUS_DONGLE_BATTERY)
    status.dongle_battery = zmk_battery_state_of_charge();
#endif

    return status;
}

zmk_studio_Response get_device_info(const zmk_studio_Request *req) {
    LOG_DBG("");
    zmk_core_GetDeviceInfoResponse resp = zmk_core_GetDeviceInfoResponse_init_zero;

    resp.name.funcs.encode = encode_device_info_name;
#if IS_ENABLED(CONFIG_HWINFO)
    resp.serial_number.funcs.encode = encode_device_info_serial_number;
#endif // IS_ENABLED(CONFIG_HWINFO)

    return CORE_RESPONSE(get_device_info, resp);
}

zmk_studio_Response get_lock_state(const zmk_studio_Request *req) {
    LOG_DBG("");
    zmk_core_LockState resp = zmk_studio_core_get_lock_state();

    return CORE_RESPONSE(get_lock_state, resp);
}

zmk_studio_Response reset_settings(const zmk_studio_Request *req) {
    LOG_DBG("");
    ZMK_RPC_SUBSYSTEM_SETTINGS_RESET_FOREACH(sub) {
        int ret = sub->callback();
        if (ret < 0) {
            LOG_ERR("Failed to reset settings: %d", ret);
            return CORE_RESPONSE(reset_settings, false);
        }
    }

    return CORE_RESPONSE(reset_settings, true);
}

zmk_studio_Response get_status(const zmk_studio_Request *req) {
    ARG_UNUSED(req);
    return CORE_RESPONSE(get_status, get_status_snapshot());
}

ZMK_RPC_SUBSYSTEM_HANDLER(core, get_device_info, ZMK_STUDIO_RPC_HANDLER_UNSECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(core, get_lock_state, ZMK_STUDIO_RPC_HANDLER_UNSECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(core, reset_settings, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(core, get_status, ZMK_STUDIO_RPC_HANDLER_UNSECURED);

static int core_event_mapper(const zmk_event_t *eh, zmk_studio_Notification *n) {
    struct zmk_studio_core_lock_state_changed *lock_ev = as_zmk_studio_core_lock_state_changed(eh);

    if (lock_ev) {
        *n = ZMK_RPC_NOTIFICATION(core, lock_state_changed, lock_ev->state);
        return 0;
    }

    struct zmk_hid_indicators_changed *indicator_ev = as_zmk_hid_indicators_changed(eh);
    if (indicator_ev) {
        cached_hid_indicators = indicator_ev->indicators;
    }

    const bool status_event =
        as_zmk_layer_state_changed(eh) || as_zmk_modifiers_state_changed(eh) ||
        as_zmk_endpoint_changed(eh) || as_zmk_hid_indicators_changed(eh) ||
        as_zmk_usb_conn_state_changed(eh) || as_zmk_split_peripheral_status_changed(eh)
#if IS_ENABLED(CONFIG_ZMK_BLE)
        || as_zmk_ble_active_profile_changed(eh)
#endif
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
        || as_zmk_peripheral_battery_state_changed(eh)
#endif
#if IS_ENABLED(CONFIG_ZMK_STUDIO_STATUS_DONGLE_BATTERY)
        || as_zmk_battery_state_changed(eh)
#endif
        ;

    if (!status_event) {
        return -ENOTSUP;
    }

    *n = ZMK_RPC_NOTIFICATION(core, status_changed, get_status_snapshot());
    return 0;
}

ZMK_RPC_EVENT_MAPPER(core, core_event_mapper, zmk_studio_core_lock_state_changed,
                     zmk_layer_state_changed, zmk_modifiers_state_changed, zmk_endpoint_changed,
                     zmk_hid_indicators_changed, zmk_usb_conn_state_changed,
                     zmk_split_peripheral_status_changed);

#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_RPC_EVENT_MAPPER_ADD_LISTENER(zmk_ble_active_profile_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
ZMK_RPC_EVENT_MAPPER_ADD_LISTENER(zmk_peripheral_battery_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_STUDIO_STATUS_DONGLE_BATTERY)
ZMK_RPC_EVENT_MAPPER_ADD_LISTENER(zmk_battery_state_changed);
#endif
