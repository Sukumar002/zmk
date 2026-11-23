#include <zephyr/settings/settings.h>
#include <errno.h>
#include <string.h>

#include <zmk/pointing_settings.h>

struct pointing_settings_record {
    uint16_t move_speed;
    uint16_t scroll_speed;
    uint8_t acceleration;
};

static struct pointing_settings_record current = {
    .move_speed = ZMK_POINTING_MOVE_SPEED_DEFAULT,
    .scroll_speed = ZMK_POINTING_SCROLL_SPEED_DEFAULT,
    .acceleration = ZMK_POINTING_ACCEL_DEFAULT,
};

uint16_t zmk_pointing_settings_move_speed(void) { return current.move_speed; }
uint16_t zmk_pointing_settings_scroll_speed(void) { return current.scroll_speed; }
uint8_t zmk_pointing_settings_acceleration(void) { return current.acceleration; }

static bool valid(uint16_t move_speed, uint16_t scroll_speed, uint8_t acceleration) {
    return move_speed >= 100 && move_speed <= 2000 &&
           scroll_speed >= 1 && scroll_speed <= 50 &&
           acceleration <= 3;
}

int zmk_pointing_settings_set(uint16_t move_speed, uint16_t scroll_speed, uint8_t acceleration) {
    if (!valid(move_speed, scroll_speed, acceleration)) {
        return -EINVAL;
    }

    current = (struct pointing_settings_record){
        .move_speed = move_speed,
        .scroll_speed = scroll_speed,
        .acceleration = acceleration,
    };

    return settings_save_one("pointing/config", &current, sizeof(current));
}

int zmk_pointing_settings_reset(void) {
    current = (struct pointing_settings_record){
        .move_speed = ZMK_POINTING_MOVE_SPEED_DEFAULT,
        .scroll_speed = ZMK_POINTING_SCROLL_SPEED_DEFAULT,
        .acceleration = ZMK_POINTING_ACCEL_DEFAULT,
    };
    return settings_delete("pointing/config");
}

static int pointing_settings_set_handler(const char *name, size_t len,
                                         settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "config") != 0 || len > sizeof(current)) {
        return -ENOENT;
    }

    struct pointing_settings_record loaded = current;
    int rc = read_cb(cb_arg, &loaded, len);
    if (rc < 0) {
        return rc;
    }

    if (valid(loaded.move_speed, loaded.scroll_speed, loaded.acceleration)) {
        current = loaded;
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(pointing, "pointing", NULL, pointing_settings_set_handler, NULL, NULL);
