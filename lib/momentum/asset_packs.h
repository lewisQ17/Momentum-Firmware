#pragma once

#include <gui/canvas.h>
#include <stdint.h>

#define ASSET_PACKS_PATH EXT_PATH("asset_packs")

void asset_packs_init(void);
void asset_packs_free(void);

// Number of icons in the last-loaded pack whose files were present but failed to
// load (broken/corrupt). Valid after asset_packs_init(); 0 for the default pack.
uint32_t asset_packs_get_failed_count(void);
