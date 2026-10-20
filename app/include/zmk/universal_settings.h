/* Experimental persistent per-board storage bridge.
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <stdint.h>
#include <zmk/universal_store.h>
int ud_settings_save(enum ud_store_board board, enum ud_store_kind kind,
                     const struct ud_store_record *record);
int ud_settings_load(struct ud_store *store);
