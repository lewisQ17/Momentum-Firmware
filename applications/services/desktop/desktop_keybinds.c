#include "desktop_keybinds.h"
#include "desktop_keybinds_filename.h"
#include "desktop_i.h"

#include <applications/main/archive/helpers/archive_helpers_ext.h>
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>
#include <saved_struct.h>
#include <input/input.h>
#include <furi.h>
#include <string.h>

#define TAG "DesktopKeybinds"

// Multi-key sequence detection: rolling history of recent short direction
// presses. Entries older than the timeout are discarded so partial sequences
// don't linger. Runs on the GUI input thread only, so plain statics are safe.
#define KEYBIND_SEQ_HISTORY_LEN (8)
#define KEYBIND_SEQ_TIMEOUT_MS  (1500)

#define OLD_DESKTOP_KEYBINDS_VER   (1)
#define OLD_DESKTOP_KEYBINDS_MAGIC (0x14)
#define OLD_DESKTOP_KEYBINDS_PATH  DESKTOP_KEYBINDS_PATH_MIGRATE
#define OLD_MAX_KEYBIND_LENGTH     (64)

typedef struct {
    char data[OLD_MAX_KEYBIND_LENGTH];
} OldKeybind;

typedef OldKeybind OldKeybinds[DesktopKeybindTypeMAX][DesktopKeybindKeyMAX];

void desktop_keybinds_migrate(Desktop* desktop) {
    if(!storage_common_exists(desktop->storage, DESKTOP_KEYBINDS_PATH)) {
        OldKeybinds old;
        const bool success = saved_struct_load(
            OLD_DESKTOP_KEYBINDS_PATH,
            &old,
            sizeof(old),
            OLD_DESKTOP_KEYBINDS_MAGIC,
            OLD_DESKTOP_KEYBINDS_VER);

        if(success) {
            DesktopKeybinds new;
            for(DesktopKeybindType type = 0; type < DesktopKeybindTypeMAX; type++) {
                for(DesktopKeybindKey key = 0; key < DesktopKeybindKeyMAX; key++) {
                    FuriString* keybind = furi_string_alloc_set(old[type][key].data);
                    if(furi_string_empty(keybind)) {
                        furi_string_set_str(keybind, "_");
                    } else if(furi_string_equal(keybind, EXT_PATH("apps/Misc/nightstand.fap"))) {
                        furi_string_set(keybind, "Clock");
                    } else if(furi_string_equal(keybind, "RFID")) {
                        furi_string_set(keybind, "125 kHz RFID");
                    } else if(furi_string_equal(keybind, "SubGHz")) {
                        furi_string_set(keybind, "Sub-GHz");
                    } else if(furi_string_equal(keybind, "Xtreme")) {
                        furi_string_set(keybind, "Momentum");
                    }
                    new[type][key] = keybind;
                }
            }
            desktop_keybinds_save(desktop, &new);
            desktop_keybinds_free(&new);
        }
    }

    storage_common_remove(desktop->storage, OLD_DESKTOP_KEYBINDS_PATH);
}

const char* const desktop_keybinds_defaults[DesktopKeybindTypeMAX][DesktopKeybindKeyMAX] = {
    [DesktopKeybindTypePress] =
        {
            [DesktopKeybindKeyUp] = "Lock Menu",
            [DesktopKeybindKeyDown] = "Archive",
            [DesktopKeybindKeyRight] = "Passport",
            [DesktopKeybindKeyLeft] = "Clock",
        },
    [DesktopKeybindTypeHold] =
        {
            [DesktopKeybindKeyUp] = "_",
            [DesktopKeybindKeyDown] = "_",
            [DesktopKeybindKeyRight] = "Device Info",
            [DesktopKeybindKeyLeft] = "Lock with PIN",
        },
};

const char* const desktop_keybind_types[DesktopKeybindTypeMAX] = {
    [DesktopKeybindTypePress] = "Press",
    [DesktopKeybindTypeHold] = "Hold",
};

const char* const desktop_keybind_keys[DesktopKeybindKeyMAX] = {
    [DesktopKeybindKeyUp] = "Up",
    [DesktopKeybindKeyDown] = "Down",
    [DesktopKeybindKeyRight] = "Right",
    [DesktopKeybindKeyLeft] = "Left",
};

static FuriString*
    desktop_keybinds_load_one(Desktop* desktop, DesktopKeybindType type, DesktopKeybindKey key) {
    bool success = false;
    FuriString* keybind = furi_string_alloc();
    FlipperFormat* file = flipper_format_file_alloc(desktop->storage);

    if(flipper_format_file_open_existing(file, DESKTOP_KEYBINDS_PATH)) {
        FuriString* keybind_name = furi_string_alloc_printf(
            "%s%s", desktop_keybind_types[type], desktop_keybind_keys[key]);
        success = flipper_format_read_string(file, furi_string_get_cstr(keybind_name), keybind);
        furi_string_free(keybind_name);
    }

    flipper_format_free(file);
    if(!success) {
        FURI_LOG_W(TAG, "Failed to load file, using defaults");
        furi_string_set(keybind, desktop_keybinds_defaults[type][key]);
    }
    return keybind;
}

void desktop_keybinds_load(Desktop* desktop, DesktopKeybinds* keybinds) {
    for(DesktopKeybindType type = 0; type < DesktopKeybindTypeMAX; type++) {
        for(DesktopKeybindKey key = 0; key < DesktopKeybindKeyMAX; key++) {
            const char* default_keybind = desktop_keybinds_defaults[type][key];
            if((*keybinds)[type][key]) {
                furi_string_set((*keybinds)[type][key], default_keybind);
            } else {
                (*keybinds)[type][key] = furi_string_alloc_set(default_keybind);
            }
        }
    }

    FlipperFormat* file = flipper_format_file_alloc(desktop->storage);
    FuriString* keybind_name = furi_string_alloc();

    if(flipper_format_file_open_existing(file, DESKTOP_KEYBINDS_PATH)) {
        for(DesktopKeybindType type = 0; type < DesktopKeybindTypeMAX; type++) {
            for(DesktopKeybindKey key = 0; key < DesktopKeybindKeyMAX; key++) {
                furi_string_printf(
                    keybind_name, "%s%s", desktop_keybind_types[type], desktop_keybind_keys[key]);
                if(!flipper_format_read_string(
                       file, furi_string_get_cstr(keybind_name), (*keybinds)[type][key])) {
                    furi_string_set((*keybinds)[type][key], desktop_keybinds_defaults[type][key]);
                    goto fail;
                }
            }
        }
    } else {
    fail:
        FURI_LOG_W(TAG, "Failed to load file, using defaults");
    }

    furi_string_free(keybind_name);
    flipper_format_free(file);
}

static bool desktop_keybind_sequence_used(const DesktopKeybindSequence* sequence) {
    return sequence->length > 0 && !furi_string_empty(sequence->action) &&
           !furi_string_equal(sequence->action, "_");
}

void desktop_keybinds_save(Desktop* desktop, const DesktopKeybinds* keybinds) {
    desktop_keybinds_save_all(desktop, keybinds, NULL);
}

void desktop_keybinds_save_all(
    Desktop* desktop,
    const DesktopKeybinds* keybinds,
    const DesktopKeybindSequences* sequences) {
    FlipperFormat* file = flipper_format_file_alloc(desktop->storage);
    FuriString* name = furi_string_alloc();
    FuriString* keys_val = furi_string_alloc();

    if(flipper_format_file_open_always(file, DESKTOP_KEYBINDS_PATH)) {
        for(DesktopKeybindType type = 0; type < DesktopKeybindTypeMAX; type++) {
            for(DesktopKeybindKey key = 0; key < DesktopKeybindKeyMAX; key++) {
                furi_string_printf(
                    name, "%s%s", desktop_keybind_types[type], desktop_keybind_keys[key]);
                if(!flipper_format_write_string_cstr(
                       file,
                       furi_string_get_cstr(name),
                       furi_string_get_cstr((*keybinds)[type][key]))) {
                    goto fail;
                }
            }
        }

        // Sequences are written after the single-key keybinds. Old readers that
        // don't know about them simply ignore these extra keys.
        uint32_t count = 0;
        if(sequences) {
            for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT; i++) {
                if(desktop_keybind_sequence_used(&(*sequences)[i])) count++;
            }
        }
        if(!flipper_format_write_uint32(file, "SeqCount", &count, 1)) {
            goto fail;
        }
        if(sequences && count > 0) {
            uint32_t written = 0;
            for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT; i++) {
                const DesktopKeybindSequence* s = &(*sequences)[i];
                if(!desktop_keybind_sequence_used(s)) continue;
                furi_string_reset(keys_val);
                for(uint8_t j = 0; j < s->length; j++) {
                    furi_string_cat_printf(keys_val, "%u", s->keys[j]);
                }
                furi_string_printf(name, "Seq%luKeys", (unsigned long)written);
                if(!flipper_format_write_string_cstr(
                       file, furi_string_get_cstr(name), furi_string_get_cstr(keys_val))) {
                    goto fail;
                }
                furi_string_printf(name, "Seq%luAction", (unsigned long)written);
                if(!flipper_format_write_string_cstr(
                       file, furi_string_get_cstr(name), furi_string_get_cstr(s->action))) {
                    goto fail;
                }
                written++;
            }
        }
    } else {
    fail:
        FURI_LOG_E(TAG, "Failed to save file");
    }

    furi_string_free(keys_val);
    furi_string_free(name);
    flipper_format_free(file);
}

void desktop_keybind_sequences_load(Desktop* desktop, DesktopKeybindSequences* sequences) {
    for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT; i++) {
        DesktopKeybindSequence* s = &(*sequences)[i];
        s->length = 0;
        memset(s->keys, 0, sizeof(s->keys));
        if(!s->action) {
            s->action = furi_string_alloc();
        } else {
            furi_string_reset(s->action);
        }
    }

    FlipperFormat* file = flipper_format_file_alloc(desktop->storage);
    if(flipper_format_file_open_existing(file, DESKTOP_KEYBINDS_PATH)) {
        uint32_t count = 0;
        if(flipper_format_read_uint32(file, "SeqCount", &count, 1)) {
            if(count > DESKTOP_KEYBIND_SEQ_COUNT) count = DESKTOP_KEYBIND_SEQ_COUNT;
            FuriString* name = furi_string_alloc();
            FuriString* keys_val = furi_string_alloc();
            for(uint32_t i = 0; i < count; i++) {
                DesktopKeybindSequence* s = &(*sequences)[i];
                furi_string_printf(name, "Seq%luKeys", (unsigned long)i);
                if(!flipper_format_read_string(file, furi_string_get_cstr(name), keys_val)) {
                    continue;
                }
                uint8_t len = 0;
                const char* cstr = furi_string_get_cstr(keys_val);
                for(size_t k = 0; cstr[k] != '\0' && len < DESKTOP_KEYBIND_SEQ_MAX_LEN; k++) {
                    char c = cstr[k];
                    if(c < '0' || c > '3') continue;
                    s->keys[len++] = (uint8_t)(c - '0');
                }
                s->length = len;
                furi_string_printf(name, "Seq%luAction", (unsigned long)i);
                if(!flipper_format_read_string(file, furi_string_get_cstr(name), s->action)) {
                    furi_string_reset(s->action);
                }
            }
            furi_string_free(name);
            furi_string_free(keys_val);
        }
    }
    flipper_format_free(file);
}

void desktop_keybind_sequences_free(DesktopKeybindSequences* sequences) {
    for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT; i++) {
        if((*sequences)[i].action) {
            furi_string_free((*sequences)[i].action);
            (*sequences)[i].action = NULL;
        }
    }
}

void desktop_keybind_sequence_keys_str(const DesktopKeybindSequence* sequence, FuriString* out) {
    furi_string_reset(out);
    if(sequence->length == 0) {
        furi_string_set(out, "(empty)");
        return;
    }
    for(uint8_t j = 0; j < sequence->length; j++) {
        uint8_t k = sequence->keys[j];
        if(k >= DesktopKeybindKeyMAX) k = 0;
        if(j > 0) furi_string_cat(out, "-");
        furi_string_cat(out, desktop_keybind_keys[k]);
    }
}

void desktop_keybinds_free(DesktopKeybinds* keybinds) {
    for(DesktopKeybindType type = 0; type < DesktopKeybindTypeMAX; type++) {
        for(DesktopKeybindKey key = 0; key < DesktopKeybindKeyMAX; key++) {
            furi_string_free((*keybinds)[type][key]);
        }
    }
}

static const DesktopKeybindType keybind_types[] = {
    [InputTypeShort] = DesktopKeybindTypePress,
    [InputTypeLong] = DesktopKeybindTypeHold,
};

static const DesktopKeybindKey keybind_keys[] = {
    [InputKeyUp] = DesktopKeybindKeyUp,
    [InputKeyDown] = DesktopKeybindKeyDown,
    [InputKeyRight] = DesktopKeybindKeyRight,
    [InputKeyLeft] = DesktopKeybindKeyLeft,
};

static void desktop_keybind_dispatch(Desktop* desktop, const char* str) {
    if(strcmp(str, "_") == 0) {
    } else if(strcmp(str, "Apps Menu") == 0) {
        loader_start_detached_with_gui_error(desktop->loader, LOADER_APPLICATIONS_NAME, NULL);
    } else if(strcmp(str, "Archive") == 0) {
        desktop_launch_archive(desktop, NULL);
    } else if(strcmp(str, "Clock") == 0) {
        loader_start_detached_with_gui_error(
            desktop->loader, EXT_PATH("apps/Tools/nightstand.fap"), "");
    } else if(strcmp(str, "Device Info") == 0) {
        loader_start_detached_with_gui_error(desktop->loader, "Power", "about_battery");
    } else if(strcmp(str, "Lock Menu") == 0) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopMainEventOpenLockMenu);
    } else if(strcmp(str, "Lock Keypad") == 0) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopMainEventLockKeypad);
    } else if(strcmp(str, "Lock with PIN") == 0) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopMainEventLockWithPin);
    } else if(strcmp(str, "Wipe Device") == 0) {
        loader_start_detached_with_gui_error(desktop->loader, "Storage", "Wipe Device");
    } else {
        if(storage_common_exists(desktop->storage, str)) {
            run_with_default_app(str);
        } else {
            loader_start_detached_with_gui_error(desktop->loader, str, NULL);
        }
    }
}

static uint8_t keybind_seq_history[KEYBIND_SEQ_HISTORY_LEN];
static uint8_t keybind_seq_history_len = 0;
static uint32_t keybind_seq_last_tick = 0;

static void desktop_keybind_sequence_reset(void) {
    keybind_seq_history_len = 0;
}

// Append a direction key to the rolling history and fire the action of any
// configured sequence whose keys match the tail of the history. Returns true
// (and suppresses the single-key keybind) only when a sequence fires.
static bool desktop_keybind_sequence_feed(Desktop* desktop, DesktopKeybindKey key) {
    uint32_t now = furi_get_tick();
    if(keybind_seq_history_len > 0 && (now - keybind_seq_last_tick) > KEYBIND_SEQ_TIMEOUT_MS) {
        keybind_seq_history_len = 0;
    }
    keybind_seq_last_tick = now;

    if(keybind_seq_history_len >= KEYBIND_SEQ_HISTORY_LEN) {
        memmove(keybind_seq_history, keybind_seq_history + 1, KEYBIND_SEQ_HISTORY_LEN - 1);
        keybind_seq_history_len = KEYBIND_SEQ_HISTORY_LEN - 1;
    }
    keybind_seq_history[keybind_seq_history_len++] = (uint8_t)key;

    DesktopKeybindSequences sequences;
    memset(sequences, 0, sizeof(sequences));
    desktop_keybind_sequences_load(desktop, &sequences);

    bool matched = false;
    for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT && !matched; i++) {
        const DesktopKeybindSequence* s = &sequences[i];
        if(!desktop_keybind_sequence_used(s)) continue;
        if(s->length > keybind_seq_history_len) continue;
        bool eq = true;
        for(uint8_t j = 0; j < s->length; j++) {
            if(keybind_seq_history[keybind_seq_history_len - s->length + j] != s->keys[j]) {
                eq = false;
                break;
            }
        }
        if(eq) {
            desktop_keybind_dispatch(desktop, furi_string_get_cstr(s->action));
            matched = true;
        }
    }

    desktop_keybind_sequences_free(&sequences);
    if(matched) keybind_seq_history_len = 0;
    return matched;
}

void desktop_run_keybind(Desktop* desktop, InputType _type, InputKey _key) {
    if(_type != InputTypeShort && _type != InputTypeLong) return;
    if(_key != InputKeyUp && _key != InputKeyDown && _key != InputKeyRight && _key != InputKeyLeft)
        return;

    DesktopKeybindType type = keybind_types[_type];
    DesktopKeybindKey key = keybind_keys[_key];

    if(_type == InputTypeShort) {
        // A matched multi-key sequence takes priority over the single-key keybind
        // for the final key; otherwise the single-key keybind runs as before.
        if(desktop_keybind_sequence_feed(desktop, key)) {
            return;
        }
    } else {
        // A hold press breaks any in-progress sequence.
        desktop_keybind_sequence_reset();
    }

    FuriString* keybind = desktop_keybinds_load_one(desktop, type, key);
    desktop_keybind_dispatch(desktop, furi_string_get_cstr(keybind));
    furi_string_free(keybind);
}
