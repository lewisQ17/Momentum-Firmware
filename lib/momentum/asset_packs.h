#pragma once

#include <gui/canvas.h>
#include <stdint.h>

#define ASSET_PACKS_PATH EXT_PATH("asset_packs")

void asset_packs_init(void);
void asset_packs_free(void);

// Number of icons in the last-loaded pack whose files were present but failed to
// load (broken/corrupt). Valid after asset_packs_init(); 0 for the default pack.
uint32_t asset_packs_get_failed_count(void);

// Load a single icon from a loose path on the SD card, outside the asset-pack
// layout. A regular file is decoded as a ".bmx" (self-describing 2x int32 LE
// width/height header + one frame blob); a directory is decoded as an animated
// icon (a "meta" file + "frame_NN.bm" frames). A bare ".bm" is rejected because
// it carries no dimensions. Returns NULL on a missing/corrupt/oversized target.
// The caller OWNS the returned icon and must release it with asset_packs_icon_free().
const Icon* asset_packs_icon_load_file(const char* path);

// Release an icon returned by asset_packs_icon_load_file(). Safe on NULL.
void asset_packs_icon_free(const Icon* icon);
