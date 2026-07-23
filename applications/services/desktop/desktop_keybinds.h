#pragma once

#include <furi/core/string.h>
#include <input/input.h>

#include "desktop.h"

typedef enum {
    DesktopKeybindTypePress,
    DesktopKeybindTypeHold,
    DesktopKeybindTypeMAX,
} DesktopKeybindType;

typedef enum {
    DesktopKeybindKeyUp,
    DesktopKeybindKeyDown,
    DesktopKeybindKeyRight,
    DesktopKeybindKeyLeft,
    DesktopKeybindKeyMAX,
} DesktopKeybindKey;

typedef FuriString* DesktopKeybinds[DesktopKeybindTypeMAX][DesktopKeybindKeyMAX];

// Multi-key sequence keybinds: a short sequence of direction keys (short-press
// only, e.g. Up-Up-Down) that fires a single action. Layered additively on top
// of the single-key keybinds above; single-key keybinds are unaffected.
#define DESKTOP_KEYBIND_SEQ_MAX_LEN (4)
#define DESKTOP_KEYBIND_SEQ_COUNT   (5)

typedef struct {
    uint8_t length; // number of keys used (0 = unused slot)
    uint8_t keys[DESKTOP_KEYBIND_SEQ_MAX_LEN]; // DesktopKeybindKey values
    FuriString* action; // action string ("_" or empty = no action)
} DesktopKeybindSequence;

typedef DesktopKeybindSequence DesktopKeybindSequences[DESKTOP_KEYBIND_SEQ_COUNT];

void desktop_keybinds_migrate(Desktop* desktop);
void desktop_keybinds_load(Desktop* desktop, DesktopKeybinds* keybinds);
void desktop_keybinds_save(Desktop* desktop, const DesktopKeybinds* keybinds);
// Saves single-key keybinds and (optionally) multi-key sequences to one file in
// a single write. Pass NULL sequences to write zero sequences.
void desktop_keybinds_save_all(
    Desktop* desktop,
    const DesktopKeybinds* keybinds,
    const DesktopKeybindSequences* sequences);
void desktop_keybinds_free(DesktopKeybinds* keybinds);

// Sequences share the same storage file as the single-key keybinds.
// Slots must be zero-initialised before the first load (action == NULL).
void desktop_keybind_sequences_load(Desktop* desktop, DesktopKeybindSequences* sequences);
void desktop_keybind_sequences_free(DesktopKeybindSequences* sequences);
// Build a human-readable representation of a sequence's keys, e.g. "Up-Up-Down".
void desktop_keybind_sequence_keys_str(const DesktopKeybindSequence* sequence, FuriString* out);

void desktop_run_keybind(Desktop* desktop, InputType _type, InputKey _key);
