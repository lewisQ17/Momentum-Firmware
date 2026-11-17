#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/number_input.h>
#include <gui/modules/variable_item_list.h>
#include <dialogs/dialogs.h>
#include <storage/storage.h>

#include "scenes/bksm_scene.h"

#define TAG "BadKbScriptMaker"

/* Same folder the Bad KB app browses */
#define BKSM_BASE_FOLDER EXT_PATH("badusb")
#define BKSM_EXTENSION   ".txt"

/* Size guards: keep RAM bounded and stay well inside what the Bad KB parser handles */
#define BKSM_MAX_LINES       200
#define BKSM_MAX_LINE_LEN    160 /* characters, without terminator */
#define BKSM_MAX_SCRIPT_SIZE (12 * 1024) /* sum of (line length + newline) */
#define BKSM_MAX_FILE_SIZE   (BKSM_MAX_SCRIPT_SIZE + BKSM_MAX_LINES) /* allows CRLF files */
#define BKSM_NAME_LEN        40
#define BKSM_LABEL_LEN       34

typedef enum {
    BksmViewSubmenu,
    BksmViewTextInput,
    BksmViewNumberInput,
    BksmViewVarList,
} BksmView;

typedef enum {
    BksmEventTextDone = 0x1000,
    BksmEventNumberDone,
    BksmEventComboPickKey,
    BksmEventComboAdd,
} BksmEvent;

typedef enum {
    BksmArgNone, /* NAME */
    BksmArgText, /* NAME <text>, text required */
    BksmArgTextOpt, /* NAME [text] */
    BksmArgNumber, /* NAME <int> */
    BksmArgMove, /* MOUSE_MOVE <x> <y> */
    BksmArgKey, /* NAME <key> */
    BksmArgHoldKey, /* NAME <key|modifier|mouse button> */
    BksmArgMedia, /* MEDIA <media key> */
    BksmArgMouse, /* <mouse button> (no command word) */
    BksmArgCombo, /* [mods...] [key] (no command word) */
    BksmArgRaw, /* free text line */
} BksmArg;

typedef struct {
    const char* label; /* shown in the command menu */
    const char* name; /* command word written to the script, NULL for bare lines */
    BksmArg arg;
    const char* hint; /* text/number input header */
    int32_t min;
    int32_t max;
    int32_t def;
} BksmCommand;

typedef enum {
    BksmPickKeys,
    BksmPickHold,
    BksmPickMedia,
    BksmPickMouse,
} BksmPickKind;

typedef enum {
    BksmScriptOk,
    BksmScriptTooManyLines,
    BksmScriptLineTooLong,
    BksmScriptTooLarge,
} BksmScriptResult;

typedef struct {
    char* lines[BKSM_MAX_LINES];
    uint16_t count;
    size_t bytes; /* sum of (strlen + 1) over all lines */
} BksmScript;

typedef enum {
    BksmModeInsert,
    BksmModeReplace,
} BksmMode;

typedef struct {
    Gui* gui;
    Storage* storage;
    DialogsApp* dialogs;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Submenu* submenu;
    TextInput* text_input;
    NumberInput* number_input;
    VariableItemList* var_list;

    BksmScript script;
    FuriString* file_path; /* empty for a new, unsaved script */
    bool dirty;

    /* editing context */
    BksmMode mode;
    uint16_t target; /* line to replace / position to insert at */
    uint16_t cursor; /* selected line in the editor */
    uint8_t cmd; /* index into bksm_commands */
    bool pick_for_combo;
    uint8_t combo_mods; /* bit0 CTRL, bit1 SHIFT, bit2 ALT, bit3 GUI */
    uint8_t combo_key; /* 0 = none, else bksm_keys index + 1 */
    uint8_t number_stage;
    int32_t number_init;
    int32_t number_value;
    int32_t number_init_y; /* MOUSE_MOVE: Y prefill */

    char text_buf[BKSM_MAX_LINE_LEN + 1];
    char line_buf[BKSM_MAX_LINE_LEN + 1];
    char name_buf[BKSM_NAME_LEN + 1];
} BksmApp;

/* Commands and key tables (bksm_commands.c) */
extern const BksmCommand bksm_commands[];
extern const size_t bksm_commands_count;
extern const char* const bksm_keys[];
extern const size_t bksm_keys_count;
extern const char* const bksm_hold_extra[];
extern const size_t bksm_hold_extra_count;
extern const char* const bksm_media_keys[];
extern const size_t bksm_media_keys_count;
extern const char* const bksm_mouse_buttons[];
extern const size_t bksm_mouse_buttons_count;

/* Script model (bksm_script.c) */
void bksm_script_clear(BksmScript* script);
BksmScriptResult bksm_script_insert(BksmScript* script, uint16_t index, const char* text);
BksmScriptResult bksm_script_replace(BksmScript* script, uint16_t index, const char* text);
void bksm_script_delete(BksmScript* script, uint16_t index);
void bksm_script_swap(BksmScript* script, uint16_t a, uint16_t b);
const char* bksm_script_result_text(BksmScriptResult result);

/* App helpers (badkb_script_maker.c) */
void bksm_show_message(BksmApp* app, const char* header, const char* text);
bool bksm_confirm(BksmApp* app, const char* header, const char* text, const char* yes);
bool bksm_load(BksmApp* app, const char* path);
bool bksm_save(BksmApp* app, const char* path);
void bksm_new_script(BksmApp* app);
bool bksm_commit_line(BksmApp* app, const char* line);
void bksm_compose_and_commit(BksmApp* app, const char* arg);
void bksm_begin_command(BksmApp* app, uint8_t cmd);
void bksm_begin_edit(BksmApp* app);
void bksm_return_to_editor(BksmApp* app);
void bksm_submenu_callback(void* context, uint32_t index);
