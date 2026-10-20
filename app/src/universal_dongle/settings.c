/* Experimental persistence bridge: explicit board-scoped, versioned records.
 * Not linked into any existing keyboard build.
 * SPDX-License-Identifier: MIT
 */
#include <zmk/universal_settings.h>
#include <zephyr/settings/settings.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct stored_header {
    uint32_t magic;
    uint32_t revision;
    uint16_t length;
};
#define UD_STORE_MAGIC 0x55444B31u

static int load_cb(const char *name, size_t len, settings_read_cb read_cb, void *arg) {
    struct ud_store *store = arg;
    const char *parts[2] = {"hana", "chocofi"};
    const char *kinds[4] = {"keymap", "combos", "behaviors", "layout_shift"};
    for (int b = 0; b < 2; b++) {
        for (int k = 0; k < 4; k++) {
            char key[64];
            (void)snprintf(key, sizeof(key), "universal/v1/%s/%s", parts[b], kinds[k]);
            if (strcmp(key, name) != 0) continue;
            if (len < sizeof(struct stored_header) ||
                len > sizeof(struct stored_header) + UD_STORE_MAX_BYTES) return -EINVAL;
            uint8_t buf[sizeof(struct stored_header) + UD_STORE_MAX_BYTES];
            int ret = read_cb(arg, buf, len);
            if (ret < 0) return ret;
            if ((size_t)ret != len) return -EIO;
            struct stored_header h;
            memcpy(&h, buf, sizeof(h));
            if (h.magic != UD_STORE_MAGIC || h.length > UD_STORE_MAX_BYTES ||
                len != sizeof(h) + h.length) return -EINVAL;
            struct ud_store_record *record = &store->values[b][k];
            record->revision = h.revision;
            record->length = h.length;
            memcpy(record->data, buf + sizeof(h), h.length);
            return 0;
        }
    }
    return 0;
}

int ud_settings_load(struct ud_store *store) {
    if (!store) return -EINVAL;
    ud_store_init(store);
    /* Read keys independently; never enumerate or clear legacy keymap/* keys. */
    const char *parts[2] = {"hana", "chocofi"};
    const char *kinds[4] = {"keymap", "combos", "behaviors", "layout_shift"};
    for (int b=0;b<2;b++) for(int k=0;k<4;k++) {
        char path[64];
        (void)snprintf(path,sizeof(path),"universal/v1/%s/%s",parts[b],kinds[k]);
        /* settings_load_subtree_direct reports callbacks for matching records */
        int err=settings_load_subtree_direct(path,load_cb,store);
        if(err) return err;
    }
    return 0;
}

int ud_settings_save(enum ud_store_board board, enum ud_store_kind kind,
                     const struct ud_store_record *record) {
    if (!record || record->length > UD_STORE_MAX_BYTES) return -EINVAL;
    const char *key=ud_store_key(board,kind);
    if (!key) return -EINVAL;
    uint8_t data[sizeof(struct stored_header) + UD_STORE_MAX_BYTES];
    struct stored_header h = {UD_STORE_MAGIC,record->revision,record->length};
    memcpy(data,&h,sizeof(h));
    memcpy(data+sizeof(h),record->data,record->length);
    return settings_save_one(key,data,sizeof(h)+record->length);
}
