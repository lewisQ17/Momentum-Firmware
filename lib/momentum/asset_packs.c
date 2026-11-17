#include "asset_packs_i.h"

#include "settings.h"

#include <assets_icons.h>
#include <core/dangerous_defines.h>
#include <furi_hal.h>
#include <gui/icon_i.h>
#include <storage/storage.h>

#define TAG "AssetPacks"

#define ICONS_FMT ASSET_PACKS_PATH "/%s/Icons/%s"
#define FONTS_FMT ASSET_PACKS_PATH "/%s/Fonts/%s.u8f"

// See lib/u8g2/u8g2_font.c
#define U8G2_FONT_DATA_STRUCT_SIZE 23

AssetPacks* asset_packs = NULL;

// Count of icons whose asset-pack files were present but could not be loaded
// (missing/corrupt/truncated frames, bad meta, wrong dimensions). A simple
// absent override is NOT counted. Reset at the start of each asset_packs_init.
static uint32_t asset_packs_failed_icons = 0;

uint32_t asset_packs_get_failed_count(void) {
    return asset_packs_failed_icons;
}

typedef struct {
    Icon icon;
    uint8_t* frames[];
} AnimatedIconSwap;

typedef struct {
    int32_t width;
    int32_t height;
    int32_t frame_rate;
    int32_t frame_count;
} FURI_PACKED AnimatedIconMetaFile;

static void
    load_icon_animated(const Icon* original, const char* name, FuriString* path, File* file) {
    const char* pack = momentum_settings.asset_pack;
    furi_string_printf(path, ICONS_FMT "/meta", pack, name);
    if(storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        AnimatedIconMetaFile meta;
        bool ok = storage_file_read(file, &meta, sizeof(meta)) == sizeof(meta);
        storage_file_close(file);

        // frame_count comes straight from an asset-pack meta file. Bound it (as
        // animation_storage.c does) so a crafted/huge value can't wrap the malloc
        // size and overflow the frames[] array.
        if(ok && (meta.frame_count <= 0 || meta.frame_count > 256)) {
            ok = false;
        }
        bool loaded = false;
        if(ok) {
            AnimatedIconSwap* swap =
                malloc(sizeof(AnimatedIconSwap) + (sizeof(uint8_t*) * meta.frame_count));
            int i = 0;
            for(; i < meta.frame_count; i++) {
                furi_string_printf(path, ICONS_FMT "/frame_%02d.bm", pack, name, i);
                if(storage_file_open(
                       file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
                    uint64_t frame_size = storage_file_size(file);
                    swap->frames[i] = malloc(frame_size);
                    ok = storage_file_read(file, swap->frames[i], frame_size) == frame_size;
                    storage_file_close(file);
                    if(ok) continue;
                } else {
                    storage_file_close(file);
                    i--;
                }
                break;
            }

            if(i == meta.frame_count) {
                FURI_CONST_ASSIGN(swap->icon.width, meta.width);
                FURI_CONST_ASSIGN(swap->icon.height, meta.height);
                FURI_CONST_ASSIGN(swap->icon.frame_count, meta.frame_count);
                FURI_CONST_ASSIGN(swap->icon.frame_rate, meta.frame_rate);
                FURI_CONST_ASSIGN_PTR(swap->icon.frames, swap->frames);

                IconSwapList_push_back(
                    asset_packs->icons,
                    (IconSwap){
                        .original = original,
                        .replaced = &swap->icon,
                    });
                loaded = true;
            } else {
                for(; i >= 0; i--) {
                    free(swap->frames[i]);
                }
                free(swap);
            }
        }

        // The meta file existed, so this icon was meant to be overridden. If we
        // didn't manage to load it, the pack's icon is broken: count it.
        if(!loaded) asset_packs_failed_icons++;
    }
    storage_file_close(file);
}

typedef struct {
    Icon icon;
    uint8_t* frames[1];
    uint8_t frame[];
} StaticIconSwap;

typedef struct {
    int32_t width;
    int32_t height;
} FURI_PACKED StaticIconBmxHeader;

static void
    load_icon_static(const Icon* original, const char* name, FuriString* path, File* file) {
    furi_string_printf(path, ICONS_FMT ".bmx", momentum_settings.asset_pack, name);
    if(storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        StaticIconBmxHeader header;
        // A valid .bmx holds at least the header; a shorter (corrupt) file would
        // underflow frame_size into a huge malloc, so treat it as no frame data
        // and let the read guard below reject it (keeping the built-in icon).
        uint64_t file_size = storage_file_size(file);
        uint64_t frame_size = (file_size >= sizeof(header)) ? (file_size - sizeof(header)) : 0;
        StaticIconSwap* swap = malloc(sizeof(StaticIconSwap) + frame_size);

        if(file_size >= sizeof(header) &&
           storage_file_read(file, &header, sizeof(header)) == sizeof(header) &&
           storage_file_read(file, swap->frame, frame_size) == frame_size) {
            FURI_CONST_ASSIGN(swap->icon.width, header.width);
            FURI_CONST_ASSIGN(swap->icon.height, header.height);
            FURI_CONST_ASSIGN(swap->icon.frame_count, 1);
            FURI_CONST_ASSIGN(swap->icon.frame_rate, 0);
            FURI_CONST_ASSIGN_PTR(swap->icon.frames, swap->frames);
            swap->frames[0] = swap->frame;

            IconSwapList_push_back(
                asset_packs->icons,
                (IconSwap){
                    .original = original,
                    .replaced = &swap->icon,
                });
        } else {
            // The .bmx file existed but was too short/unreadable: broken icon.
            free(swap);
            asset_packs_failed_icons++;
        }
    }
    storage_file_close(file);
}

static void free_icon(const Icon* icon) {
    StaticIconSwap* swap = (void*)icon;
    // StaticIconSwap and AnimatedIconSwap have similar structure, but
    // animated one has frames array of variable length, and frame data is
    // in another allocation, while static includes frame in same allocation
    // By checking if the first frame points to later in same allocation, we
    // can tell if it is static or animated
    if(swap->frames[0] != swap->frame) {
        for(size_t i = 0; i < swap->icon.frame_count; i++) {
            free(swap->frames[i]);
        }
    }
    free(swap);
}

static void load_font(Font font, const char* name, FuriString* path, File* file) {
    furi_string_printf(path, FONTS_FMT, momentum_settings.asset_pack, name);
    if(storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint64_t size = storage_file_size(file);
        uint8_t* swap = malloc(size);

        if(size > U8G2_FONT_DATA_STRUCT_SIZE && storage_file_read(file, swap, size) == size) {
            asset_packs->fonts[font] = swap;
            CanvasFontParameters* params = malloc(sizeof(CanvasFontParameters));
            // See lib/u8g2/u8g2_font.c
            params->leading_default = swap[10]; // max_char_height
            params->leading_min = params->leading_default - 2; // good enough
            params->height = MAX((int8_t)swap[15], 0); // ascent_para
            params->descender = MAX((int8_t)swap[16], 0); // descent_para
            asset_packs->font_params[font] = params;
        } else {
            free(swap);
        }
    }
    storage_file_close(file);
}

static void free_font(Font font) {
    free(asset_packs->fonts[font]);
    asset_packs->fonts[font] = NULL;
    free(asset_packs->font_params[font]);
    asset_packs->font_params[font] = NULL;
}

static const char* font_names[] = {
    [FontPrimary] = "Primary",
    [FontSecondary] = "Secondary",
    [FontKeyboard] = "Keyboard",
    [FontBigNumbers] = "BigNumbers",
    [FontBatteryPercent] = "BatteryPercent",
};

void asset_packs_init(void) {
    if(asset_packs) return;

    asset_packs_failed_icons = 0;

    const char* pack = momentum_settings.asset_pack;
    if(pack[0] == '\0') return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* p = furi_string_alloc_printf(ASSET_PACKS_PATH "/%s", pack);
    FileInfo info;
    if(storage_common_stat(storage, furi_string_get_cstr(p), &info) == FSE_OK &&
       info.flags & FSF_DIRECTORY) {
        asset_packs = malloc(sizeof(AssetPacks));
        memset(asset_packs, 0, sizeof(AssetPacks));
        IconSwapList_init(asset_packs->icons);

        File* f = storage_file_alloc(storage);

        furi_string_printf(p, ASSET_PACKS_PATH "/%s/Icons", pack);
        if(storage_common_stat(storage, furi_string_get_cstr(p), &info) == FSE_OK &&
           info.flags & FSF_DIRECTORY) {
            for(size_t i = 0; i < ICON_PATHS_COUNT; i++) {
                if(ICON_PATHS[i].icon->frame_count > 1) {
                    load_icon_animated(ICON_PATHS[i].icon, ICON_PATHS[i].path, p, f);
                } else {
                    load_icon_static(ICON_PATHS[i].icon, ICON_PATHS[i].path, p, f);
                }
            }
        }

        furi_string_printf(p, ASSET_PACKS_PATH "/%s/Fonts", pack);
        if(storage_common_stat(storage, furi_string_get_cstr(p), &info) == FSE_OK &&
           info.flags & FSF_DIRECTORY) {
            for(Font font = 0; font < FontTotalNumber; font++) {
                load_font(font, font_names[font], p, f);
            }
        }

        storage_file_free(f);
    }
    furi_string_free(p);
    furi_record_close(RECORD_STORAGE);
}

void asset_packs_free(void) {
    if(!asset_packs) return;

    for
        M_EACH(icon_swap, asset_packs->icons, IconSwapList_t) {
            free_icon(icon_swap->replaced);
        }
    IconSwapList_clear(asset_packs->icons);

    for(Font font = 0; font < FontTotalNumber; font++) {
        if(asset_packs->fonts[font] != NULL) {
            free_font(font);
        }
    }

    free(asset_packs);
    asset_packs = NULL;
}

const Icon* asset_packs_swap_icon(const Icon* requested) {
    if(!asset_packs) return requested;
    if((uint32_t)requested < FLASH_BASE || (uint32_t)requested > (FLASH_BASE + FLASH_SIZE)) {
        return requested;
    }
    for
        M_EACH(icon_swap, asset_packs->icons, IconSwapList_t) {
            if(icon_swap->original == requested) {
                return icon_swap->replaced;
            }
        }
    return requested;
}

// ---------------------------------------------------------------------------
// Loose icons: load a single Icon from an arbitrary file/dir on the SD card,
// outside the structured asset-pack layout. The on-disk formats match a pack:
//   - a regular file is a ".bmx": [int32 w][int32 h] header + one frame blob
//   - a directory is an animated icon: "meta" + "frame_NN.bm" files
// A bare ".bm" is rejected (it carries no width/height). The returned Icon is
// heap-owned by the CALLER and must be released with asset_packs_icon_free();
// it is never placed in the swap list, so asset_packs_free() never touches it.
//
// Unlike the pack loaders above, these validate dimensions and frame sizes: the
// path can point at any file the user browsed to, so the data is untrusted.
// ---------------------------------------------------------------------------

// No built-in icon exceeds 128x64, and the canvas icon decode buffer is exactly
// 128*64/8 bytes (gui/canvas_i.h ICON_DECOMPRESSOR_BUFFER_SIZE). Reject anything
// larger so a crafted header can't make the draw path read past that buffer.
#define ASSET_PACKS_ICON_MAX_W 128
#define ASSET_PACKS_ICON_MAX_H 64

// Upper bound for one frame blob, checked BEFORE allocating (furi malloc aborts on
// OOM). Largest valid frame is 1 + 16 * 64 = 1025 B uncompressed; a compressed
// frame is not larger than that plus a small header/overhead.
#define ASSET_PACKS_ICON_MAX_FRAME_SIZE 2048

static bool asset_packs_icon_dims_valid(int32_t width, int32_t height) {
    return width > 0 && height > 0 && width <= ASSET_PACKS_ICON_MAX_W &&
           height <= ASSET_PACKS_ICON_MAX_H;
}

// Verify a frame blob is at least as large as what the draw path reads from it.
// Uncompressed (byte 0 == 0): compress_icon_decode points the canvas straight at
// frame+1 and it reads ((w+7)/8)*h raw XBM bytes. Compressed (byte 0 != 0): a
// 4-byte CompressHeader whose bytes 2-3 (LE) hold the compressed payload size;
// the heatshrink decoder reads 4 + that many bytes. Either way the file must
// contain those bytes or the decode path reads past the malloc'd frame buffer.
static bool asset_packs_icon_frame_valid(
    const uint8_t* frame,
    uint64_t frame_size,
    int32_t width,
    int32_t height) {
    if(frame_size < 1) return false;
    if(frame[0] == 0) {
        uint32_t need = 1u + (uint32_t)((width + 7) / 8) * (uint32_t)height;
        return frame_size >= need;
    }
    if(frame_size < 4) return false;
    uint32_t compressed = (uint32_t)frame[2] | ((uint32_t)frame[3] << 8);
    return frame_size >= 4u + compressed;
}

static Icon* asset_packs_load_static_file(File* file, const char* path) {
    if(!storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        return NULL;
    }
    Icon* result = NULL;
    uint64_t file_size = storage_file_size(file);
    if(file_size >= sizeof(StaticIconBmxHeader) &&
       file_size - sizeof(StaticIconBmxHeader) <= ASSET_PACKS_ICON_MAX_FRAME_SIZE) {
        uint64_t frame_size = file_size - sizeof(StaticIconBmxHeader);
        StaticIconSwap* swap = malloc(sizeof(StaticIconSwap) + frame_size);
        StaticIconBmxHeader header;
        if(frame_size > 0 && storage_file_read(file, &header, sizeof(header)) == sizeof(header) &&
           storage_file_read(file, swap->frame, frame_size) == frame_size &&
           asset_packs_icon_dims_valid(header.width, header.height) &&
           asset_packs_icon_frame_valid(swap->frame, frame_size, header.width, header.height)) {
            FURI_CONST_ASSIGN(swap->icon.width, header.width);
            FURI_CONST_ASSIGN(swap->icon.height, header.height);
            FURI_CONST_ASSIGN(swap->icon.frame_count, 1);
            FURI_CONST_ASSIGN(swap->icon.frame_rate, 0);
            FURI_CONST_ASSIGN_PTR(swap->icon.frames, swap->frames);
            swap->frames[0] = swap->frame;
            result = &swap->icon;
        } else {
            free(swap);
        }
    }
    storage_file_close(file);
    return result;
}

static Icon* asset_packs_load_animated_dir(File* file, FuriString* scratch, const char* dir) {
    furi_string_printf(scratch, "%s/meta", dir);
    if(!storage_file_open(file, furi_string_get_cstr(scratch), FSAM_READ, FSOM_OPEN_EXISTING)) {
        return NULL;
    }
    AnimatedIconMetaFile meta;
    bool ok = storage_file_read(file, &meta, sizeof(meta)) == sizeof(meta);
    storage_file_close(file);

    // Reject values that would divide-by-zero or truncate downstream:
    //  - frame_count 0, or >255 (icon.frame_count is uint8_t; 256 wraps to 0 and
    //    icon_animation does `frame % frame_count`)
    //  - frame_rate 0 (icon_animation divides by it), or >255 (uint8_t)
    if(ok && (meta.frame_count <= 0 || meta.frame_count > 255 || meta.frame_rate < 1 ||
              meta.frame_rate > 255 || !asset_packs_icon_dims_valid(meta.width, meta.height))) {
        ok = false;
    }
    if(!ok) return NULL;

    AnimatedIconSwap* swap =
        malloc(sizeof(AnimatedIconSwap) + (sizeof(uint8_t*) * meta.frame_count));
    int i = 0;
    for(; i < meta.frame_count; i++) {
        furi_string_printf(scratch, "%s/frame_%02d.bm", dir, i);
        if(storage_file_open(file, furi_string_get_cstr(scratch), FSAM_READ, FSOM_OPEN_EXISTING)) {
            uint64_t frame_size = storage_file_size(file);
            if(frame_size > ASSET_PACKS_ICON_MAX_FRAME_SIZE) {
                swap->frames[i] = NULL; // freed in the unwind below
                ok = false;
            } else {
                swap->frames[i] = malloc(frame_size);
                ok = storage_file_read(file, swap->frames[i], frame_size) == frame_size &&
                     asset_packs_icon_frame_valid(
                         swap->frames[i], frame_size, meta.width, meta.height);
            }
            storage_file_close(file);
            if(ok) continue;
        } else {
            storage_file_close(file);
            i--;
        }
        break;
    }

    if(i == meta.frame_count) {
        FURI_CONST_ASSIGN(swap->icon.width, meta.width);
        FURI_CONST_ASSIGN(swap->icon.height, meta.height);
        FURI_CONST_ASSIGN(swap->icon.frame_count, meta.frame_count);
        FURI_CONST_ASSIGN(swap->icon.frame_rate, meta.frame_rate);
        FURI_CONST_ASSIGN_PTR(swap->icon.frames, swap->frames);
        return &swap->icon;
    }
    for(; i >= 0; i--) {
        free(swap->frames[i]);
    }
    free(swap);
    return NULL;
}

const Icon* asset_packs_icon_load_file(const char* path) {
    if(!path || path[0] == '\0') return NULL;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    FuriString* scratch = furi_string_alloc();
    Icon* result = NULL;

    FileInfo info;
    if(storage_common_stat(storage, path, &info) == FSE_OK) {
        if(info.flags & FSF_DIRECTORY) {
            result = asset_packs_load_animated_dir(file, scratch, path);
        } else {
            result = asset_packs_load_static_file(file, path);
        }
    }

    furi_string_free(scratch);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return result;
}

void asset_packs_icon_free(const Icon* icon) {
    if(icon) free_icon(icon);
}
