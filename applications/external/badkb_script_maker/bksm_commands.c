#include "badkb_script_maker_i.h"

/* Only commands, key names and media names that the Bad KB parser
 * (applications/main/bad_usb/helpers/ducky_script_*.c) really understands. */
const BksmCommand bksm_commands[] = {
    {"STRING (type text)", "STRING", BksmArgText, "Text to type", 0, 0, 0},
    {"STRINGLN (text+Enter)", "STRINGLN", BksmArgText, "Text to type + Enter", 0, 0, 0},
    {"ENTER", "ENTER", BksmArgNone, NULL, 0, 0, 0},
    {"Key / combo", NULL, BksmArgCombo, NULL, 0, 0, 0},
    {"DELAY (ms)", "DELAY", BksmArgNumber, "Delay in ms", 1, 600000, 500},
    {"DEFAULT_DELAY (ms)", "DEFAULT_DELAY", BksmArgNumber, "Delay after each line", 0, 60000, 100},
    {"STRING_DELAY (ms)", "STRING_DELAY", BksmArgNumber, "Per-char delay, next STRING", 0, 10000, 50},
    {"DEFAULT_STRING_DELAY", "DEFAULT_STRING_DELAY", BksmArgNumber, "Per-char delay", 0, 10000, 10},
    {"REPEAT (prev line)", "REPEAT", BksmArgNumber, "Repeat previous line", 1, 10000, 1},
    {"REM (comment)", "REM", BksmArgTextOpt, "Comment", 0, 0, 0},
    {"HOLD key", "HOLD", BksmArgHoldKey, NULL, 0, 0, 0},
    {"RELEASE key", "RELEASE", BksmArgHoldKey, NULL, 0, 0, 0},
    {"MEDIA key", "MEDIA", BksmArgMedia, NULL, 0, 0, 0},
    {"GLOBE + key", "GLOBE", BksmArgKey, NULL, 0, 0, 0},
    {"SYSRQ + key", "SYSRQ", BksmArgKey, NULL, 0, 0, 0},
    {"ALTCHAR (code)", "ALTCHAR", BksmArgNumber, "Alt+Numpad code", 0, 9999, 65},
    {"ALTSTRING (text)", "ALTSTRING", BksmArgText, "Text via Alt codes", 0, 0, 0},
    {"WAIT_FOR_BUTTON_PRESS", "WAIT_FOR_BUTTON_PRESS", BksmArgNone, NULL, 0, 0, 0},
    {"Mouse click", NULL, BksmArgMouse, NULL, 0, 0, 0},
    {"MOUSE_MOVE (x y)", "MOUSE_MOVE", BksmArgMove, "Mouse X", -127, 127, 0},
    {"MOUSE_SCROLL", "MOUSE_SCROLL", BksmArgNumber, "Scroll (+up/-down)", -127, 127, 1},
    {"MASS_STORAGE_MOUNT", "MASS_STORAGE_MOUNT", BksmArgText, "Image path on SD", 0, 0, 0},
    {"MASS_STORAGE_UMOUNT", "MASS_STORAGE_UMOUNT", BksmArgNone, NULL, 0, 0, 0},
    {"ID (USB, line 1)", "ID", BksmArgText, "VID:PID Mfr:Product", 0, 0, 0},
    {"BLE_ID (BT, line 1)", "BLE_ID", BksmArgText, "AA:BB:CC:DD:EE:FF Name", 0, 0, 0},
    {"Raw line", NULL, BksmArgRaw, "Raw script line", 0, 0, 0},
};
const size_t bksm_commands_count = COUNT_OF(bksm_commands);

/* ducky_keys[] names (canonical spelling) + single characters typed via layout */
const char* const bksm_keys[] = {
    "ENTER",     "TAB",        "ESC",         "SPACE", "BACKSPACE", "DELETE", "UP",
    "DOWN",      "LEFT",       "RIGHT",       "HOME",  "END",       "PAGEUP", "PAGEDOWN",
    "INSERT",    "CAPSLOCK",   "NUMLOCK",     "SCROLLLOCK",         "PRINTSCREEN",
    "PAUSE",     "MENU",       "F1",          "F2",    "F3",        "F4",     "F5",
    "F6",        "F7",         "F8",          "F9",    "F10",       "F11",    "F12",
    "F13",       "F14",        "F15",         "F16",   "F17",       "F18",    "F19",
    "F20",       "F21",        "F22",         "F23",   "F24",       "a",      "b",
    "c",         "d",          "e",           "f",     "g",         "h",      "i",
    "j",         "k",          "l",           "m",     "n",         "o",      "p",
    "q",         "r",          "s",           "t",     "u",         "v",      "w",
    "x",         "y",          "z",           "0",     "1",         "2",      "3",
    "4",         "5",          "6",           "7",     "8",         "9",
};
const size_t bksm_keys_count = COUNT_OF(bksm_keys);

/* HOLD / RELEASE also accept modifiers and mouse buttons */
const char* const bksm_hold_extra[] = {
    "CTRL",
    "SHIFT",
    "ALT",
    "GUI",
    "LEFTCLICK",
    "RIGHTCLICK",
    "MIDDLECLICK",
};
const size_t bksm_hold_extra_count = COUNT_OF(bksm_hold_extra);

/* ducky_media_keys[] */
const char* const bksm_media_keys[] = {
    "PLAY_PAUSE", "PLAY",      "PAUSE",       "STOP",        "NEXT_TRACK", "PREV_TRACK",
    "MUTE",       "VOLUME_UP", "VOLUME_DOWN", "BRIGHT_UP",   "BRIGHT_DOWN", "EJECT",
    "HOME",       "BACK",      "FORWARD",     "REFRESH",     "EXIT",       "SNAPSHOT",
    "FN",         "SLEEP",     "LOGOFF",      "REBOOT",      "POWER",
};
const size_t bksm_media_keys_count = COUNT_OF(bksm_media_keys);

/* ducky_mouse_keys[] */
const char* const bksm_mouse_buttons[] = {
    "LEFTCLICK",
    "RIGHTCLICK",
    "MIDDLECLICK",
};
const size_t bksm_mouse_buttons_count = COUNT_OF(bksm_mouse_buttons);
