/* Isolated dual-keyboard configuration records. SPDX-License-Identifier: MIT */
#pragma once
#include <stddef.h>
#include <stdint.h>
enum ud_store_board { UD_STORE_HANA = 0, UD_STORE_CHOCOFI = 1 };
enum ud_store_kind {
    UD_STORE_KEYMAP = 0,
    UD_STORE_COMBOS = 1,
    UD_STORE_BEHAVIORS = 2,
    UD_STORE_LAYOUT = 3
};
enum ud_store_result {
    UD_STORE_OK = 0,
    UD_STORE_INVALID = -1,
    UD_STORE_CONFLICT = -2,
    UD_STORE_NO_SPACE = -3
};
#define UD_STORE_MAX_BYTES 128
struct ud_store_record {
    uint32_t revision;
    uint16_t length;
    uint8_t data[UD_STORE_MAX_BYTES];
};
struct ud_store {
    struct ud_store_record values[2][4];
};
void ud_store_init(struct ud_store *store);
int ud_store_write(struct ud_store *store, enum ud_store_board board, enum ud_store_kind kind,
                   uint32_t expected_revision, const void *data, size_t size);
const struct ud_store_record *ud_store_read(const struct ud_store *store, enum ud_store_board board,
                                            enum ud_store_kind kind);
const char *ud_store_key(enum ud_store_board board, enum ud_store_kind kind);
