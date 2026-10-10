#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <zmk/pointing_settings.h>
#include <zmk/studio/rpc.h>

ZMK_RPC_SUBSYSTEM(pointing)

#define POINTING_RESPONSE(type, ...) ZMK_RPC_RESPONSE(pointing, type, __VA_ARGS__)

static zmk_pointing_MouseSettings current_settings(void) {
    zmk_pointing_MouseSettings out = zmk_pointing_MouseSettings_init_zero;
    out.move_speed = zmk_pointing_settings_move_speed();
    out.scroll_speed = zmk_pointing_settings_scroll_speed();
    out.acceleration = zmk_pointing_settings_acceleration();
    return out;
}

zmk_studio_Response get_settings(const zmk_studio_Request *req) {
    return POINTING_RESPONSE(get_settings, current_settings());
}

ZMK_RPC_SUBSYSTEM_HANDLER(pointing, get_settings, ZMK_STUDIO_RPC_HANDLER_SECURED);

zmk_studio_Response set_settings(const zmk_studio_Request *req) {
    const zmk_pointing_MouseSettings *settings =
        &req->subsystem.pointing.request_type.set_settings;

    int rc = zmk_pointing_settings_set(settings->move_speed, settings->scroll_speed,
                                       settings->acceleration);
    if (rc < 0) {
        return ZMK_RPC_SIMPLE_ERR(GENERIC);
    }

    return POINTING_RESPONSE(set_settings, current_settings());
}

ZMK_RPC_SUBSYSTEM_HANDLER(pointing, set_settings, ZMK_STUDIO_RPC_HANDLER_SECURED);

static int pointing_settings_reset(void) { return zmk_pointing_settings_reset(); }

ZMK_RPC_SUBSYSTEM_SETTINGS_RESET(pointing, pointing_settings_reset);

static int event_mapper(const zmk_event_t *eh, zmk_studio_Notification *n) { return 0; }

ZMK_RPC_EVENT_MAPPER(pointing, event_mapper);
