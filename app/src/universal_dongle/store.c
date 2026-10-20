/* In-memory proof of per-board isolation; not connected to Zephyr NVS.
 * SPDX-License-Identifier: MIT
 */
#include <zmk/universal_store.h>
#include <string.h>
static const char *const paths[2][4] = {
    {"universal/v1/hana/keymap", "universal/v1/hana/combos", "universal/v1/hana/behaviors",
     "universal/v1/hana/layout_shift"},
    {"universal/v1/chocofi/keymap", "universal/v1/chocofi/combos", "universal/v1/chocofi/behaviors",
     "universal/v1/chocofi/layout_shift"}};
static int valid(int board, int kind) { return board >= 0 && board < 2 && kind >= 0 && kind < 4; }
void ud_store_init(struct ud_store *store) {
    if (store)
        memset(store, 0, sizeof(*store));
}
const char *ud_store_key(enum ud_store_board b, enum ud_store_kind k) {
    return valid(b, k) ? paths[b][k] : NULL;
}
const struct ud_store_record *ud_store_read(const struct ud_store *s, enum ud_store_board b,
                                            enum ud_store_kind k) {
    return s && valid(b, k) ? &s->values[b][k] : NULL;
}
int ud_store_write(struct ud_store *s, enum ud_store_board b, enum ud_store_kind k,
                   uint32_t expected_revision, const void *data, size_t size) {
    if (!s || !valid(b, k) || (!data && size))
        return UD_STORE_INVALID;
    if (size > UD_STORE_MAX_BYTES)
        return UD_STORE_NO_SPACE;
    struct ud_store_record *r = &s->values[b][k];
    if (r->revision != expected_revision)
        return UD_STORE_CONFLICT;
    if (size)
        memcpy(r->data, data, size);
    if (size < UD_STORE_MAX_BYTES)
        memset(r->data + size, 0, UD_STORE_MAX_BYTES - size);
    r->length = (uint16_t)size;
    r->revision++;
    return UD_STORE_OK;
}
