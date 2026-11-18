#include "sprite.h"
#include <storage/storage.h>

#ifdef SPRITE_DEBUG
#define SPRITE_D(...) FURI_LOG_D("Sprite", __VA_ARGS__)
#define SPRITE_E(...) FURI_LOG_E("Sprite", __VA_ARGS__)
#else
#define SPRITE_D(...)
#define SPRITE_E(...)
#endif

#define SPRITE_MAX_FILE_SIZE 4096

struct Sprite {
    uint32_t width;
    uint32_t height;
    uint8_t data[];
};

Sprite* sprite_alloc(const char* path) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    Sprite* sprite = NULL;

    do {
        if(!storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
            SPRITE_E("Failed to open file: %s", path);
            break;
        }

        uint32_t size = 0;
        if(storage_file_read(file, &size, sizeof(size)) != sizeof(size)) {
            SPRITE_E("Failed to read file size: %s", path);
            break;
        }

        // the size comes from the file: it has to cover the header and stay sane
        if(size < sizeof(Sprite) || size > SPRITE_MAX_FILE_SIZE) {
            SPRITE_E("Invalid sprite size: %s", path);
            break;
        }

        sprite = malloc(size);
        if(storage_file_read(file, sprite, size) != size) {
            SPRITE_E("Failed to read file: %s", path);
            free(sprite);
            sprite = NULL;
            break;
        }

        // the bitmap rows are byte aligned and have to fit in what was read
        if(sprite->width == 0 || sprite->height == 0 || sprite->width > 128 ||
           sprite->height > 64 ||
           (size_t)((sprite->width + 7) / 8) * sprite->height > size - sizeof(Sprite)) {
            SPRITE_E("Invalid sprite dimensions: %s", path);
            free(sprite);
            sprite = NULL;
            break;
        }

        SPRITE_D(
            "Loaded sprite: %s, width: %lu, height: %lu", path, sprite->width, sprite->height);
    } while(false);

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return sprite;
}

void sprite_free(Sprite* sprite) {
    free(sprite);
}

size_t sprite_get_width(Sprite* sprite) {
    return sprite->width;
}

size_t sprite_get_height(Sprite* sprite) {
    return sprite->height;
}

void canvas_draw_sprite(Canvas* canvas, Sprite* sprite, int32_t x, int32_t y) {
    furi_check(sprite->width);
    furi_check(sprite->height);
    furi_check(sprite->data);
    canvas_draw_xbm(canvas, x, y, sprite->width, sprite->height, sprite->data);
}
