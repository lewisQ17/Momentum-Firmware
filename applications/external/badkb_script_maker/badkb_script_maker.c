#include "badkb_script_maker_i.h"

#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Dialog helpers                                                      */
/* ------------------------------------------------------------------ */

void bksm_show_message(BksmApp* app, const char* header, const char* text) {
    DialogMessage* msg = dialog_message_alloc();
    dialog_message_set_header(msg, header, 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(msg, text, 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(msg, NULL, "OK", NULL);
    dialog_message_show(app->dialogs, msg);
    dialog_message_free(msg);
}

bool bksm_confirm(BksmApp* app, const char* header, const char* text, const char* yes) {
    DialogMessage* msg = dialog_message_alloc();
    dialog_message_set_header(msg, header, 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(msg, text, 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(msg, "Cancel", NULL, yes);
    DialogMessageButton result = dialog_message_show(app->dialogs, msg);
    dialog_message_free(msg);
    return result == DialogMessageButtonRight;
}

/* ------------------------------------------------------------------ */
/* File I/O                                                            */
/* ------------------------------------------------------------------ */

bool bksm_load(BksmApp* app, const char* path) {
    /* Parse into a scratch script so a failed load never touches the open one */
    BksmScript* tmp = malloc(sizeof(BksmScript));
    memset(tmp, 0, sizeof(BksmScript));
    File* file = storage_file_alloc(app->storage);
    const char* error = NULL;
    char err_buf[48];
    uint8_t buf[64];
    size_t len = 0;
    uint16_t line_no = 1;

    do {
        if(!storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
            error = "Cannot open file";
            break;
        }
        if(storage_file_size(file) > BKSM_MAX_FILE_SIZE) {
            error = "File too large\n(max 12 KB)";
            break;
        }

        bool done = false;
        while(!done && !error) {
            size_t read = storage_file_read(file, buf, sizeof(buf));
            if(read < sizeof(buf) && storage_file_get_error(file) != FSE_OK) {
                error = "SD card read error";
                break;
            }
            if(read == 0) done = true;

            for(size_t i = 0; i < read && !error; i++) {
                char c = (char)buf[i];
                if(c == '\r') continue;
                if(c == '\0') {
                    error = "Not a text file";
                } else if(c == '\n') {
                    app->line_buf[len] = '\0';
                    BksmScriptResult res = bksm_script_insert(tmp, tmp->count, app->line_buf);
                    if(res != BksmScriptOk) {
                        snprintf(
                            err_buf,
                            sizeof(err_buf),
                            "Line %u:\n%s",
                            line_no,
                            bksm_script_result_text(res));
                        error = err_buf;
                    }
                    len = 0;
                    line_no++;
                } else if(len >= BKSM_MAX_LINE_LEN) {
                    snprintf(err_buf, sizeof(err_buf), "Line %u too long\n(max 160 chars)", line_no);
                    error = err_buf;
                } else {
                    app->line_buf[len++] = c;
                }
            }
        }
        if(!error && len > 0) {
            app->line_buf[len] = '\0';
            BksmScriptResult res = bksm_script_insert(tmp, tmp->count, app->line_buf);
            if(res != BksmScriptOk) error = bksm_script_result_text(res);
        }
    } while(false);

    if(storage_file_is_open(file)) storage_file_close(file);
    storage_file_free(file);

    if(error) {
        FURI_LOG_W(TAG, "Load %s failed: %s", path, error);
        bksm_script_clear(tmp);
        free(tmp);
        bksm_show_message(app, "Cannot open", error);
        return false;
    }

    bksm_script_clear(&app->script);
    memcpy(&app->script, tmp, sizeof(BksmScript));
    free(tmp);
    furi_string_set(app->file_path, path);
    app->dirty = false;
    app->cursor = 0;
    FURI_LOG_I(TAG, "Loaded %u lines from %s", app->script.count, path);
    return true;
}

bool bksm_save(BksmApp* app, const char* path) {
    FuriString* tmp_path = furi_string_alloc_printf("%s.tmp", path);
    File* file = storage_file_alloc(app->storage);
    bool ok = false;

    do {
        if(!storage_file_open(
               file, furi_string_get_cstr(tmp_path), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            break;
        }
        bool write_ok = true;
        for(uint16_t i = 0; i < app->script.count && write_ok; i++) {
            const char* line = app->script.lines[i];
            size_t len = strlen(line);
            if(len && storage_file_write(file, line, len) != len) write_ok = false;
            if(write_ok && storage_file_write(file, "\n", 1) != 1) write_ok = false;
        }
        if(!storage_file_close(file)) write_ok = false;
        if(!write_ok) break;
        /* Atomic replace: the old script survives if anything above failed */
        if(storage_common_rename(app->storage, furi_string_get_cstr(tmp_path), path) != FSE_OK) {
            break;
        }
        ok = true;
    } while(false);

    if(!ok) {
        if(storage_file_is_open(file)) storage_file_close(file);
        storage_simply_remove(app->storage, furi_string_get_cstr(tmp_path));
        FURI_LOG_E(TAG, "Save to %s failed", path);
        bksm_show_message(app, "Save failed", "Could not write to\nSD card. Script kept\nin memory.");
    } else {
        furi_string_set(app->file_path, path);
        app->dirty = false;
        FURI_LOG_I(TAG, "Saved %u lines to %s", app->script.count, path);
    }

    storage_file_free(file);
    furi_string_free(tmp_path);
    return ok;
}

void bksm_new_script(BksmApp* app) {
    bksm_script_clear(&app->script);
    furi_string_reset(app->file_path);
    app->dirty = false;
    app->cursor = 0;
}

/* ------------------------------------------------------------------ */
/* Editing flow                                                        */
/* ------------------------------------------------------------------ */

void bksm_submenu_callback(void* context, uint32_t index) {
    BksmApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void bksm_return_to_editor(BksmApp* app) {
    scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BksmSceneEditor);
}

bool bksm_commit_line(BksmApp* app, const char* line) {
    BksmScriptResult res;
    if(app->mode == BksmModeReplace && app->target < app->script.count) {
        res = bksm_script_replace(&app->script, app->target, line);
    } else {
        res = bksm_script_insert(&app->script, app->target, line);
    }
    if(res != BksmScriptOk) {
        bksm_show_message(app, "Not added", bksm_script_result_text(res));
        return false;
    }
    app->cursor = MIN(app->target, (uint16_t)(app->script.count - 1));
    app->dirty = true;
    bksm_return_to_editor(app);
    return true;
}

void bksm_compose_and_commit(BksmApp* app, const char* arg) {
    const BksmCommand* cmd = &bksm_commands[app->cmd];
    int written;
    if(!cmd->name) {
        written = snprintf(app->line_buf, sizeof(app->line_buf), "%s", arg ? arg : "");
    } else if(arg && arg[0]) {
        written = snprintf(app->line_buf, sizeof(app->line_buf), "%s %s", cmd->name, arg);
    } else {
        written = snprintf(app->line_buf, sizeof(app->line_buf), "%s", cmd->name);
    }
    if(written < 0 || (size_t)written >= sizeof(app->line_buf)) {
        bksm_show_message(app, "Not added", bksm_script_result_text(BksmScriptLineTooLong));
        return;
    }
    bksm_commit_line(app, app->line_buf);
}

/* Route a picked command to its argument input (or commit it directly) */
void bksm_begin_command(BksmApp* app, uint8_t cmd_index) {
    furi_check(cmd_index < bksm_commands_count);
    app->cmd = cmd_index;
    const BksmCommand* cmd = &bksm_commands[cmd_index];

    switch(cmd->arg) {
    case BksmArgNone:
        bksm_compose_and_commit(app, NULL);
        break;
    case BksmArgText:
    case BksmArgTextOpt:
    case BksmArgRaw:
        app->text_buf[0] = '\0';
        scene_manager_next_scene(app->scene_manager, BksmSceneText);
        break;
    case BksmArgNumber:
    case BksmArgMove:
        app->number_stage = 0;
        app->number_init = cmd->def;
        app->number_init_y = 0;
        scene_manager_next_scene(app->scene_manager, BksmSceneNumber);
        break;
    case BksmArgKey:
    case BksmArgHoldKey:
    case BksmArgMedia:
    case BksmArgMouse:
        app->pick_for_combo = false;
        scene_manager_next_scene(app->scene_manager, BksmScenePicker);
        break;
    case BksmArgCombo:
        app->combo_mods = 0;
        app->combo_key = 0;
        scene_manager_set_scene_state(app->scene_manager, BksmSceneCombo, 0);
        scene_manager_next_scene(app->scene_manager, BksmSceneCombo);
        break;
    }
}

static int32_t bksm_parse_int(const char* s, int32_t fallback) {
    char* end = NULL;
    long v = strtol(s, &end, 10);
    return (end && end != s) ? (int32_t)v : fallback;
}

/* Edit the selected line with the input that matches its command */
void bksm_begin_edit(BksmApp* app) {
    furi_check(app->cursor < app->script.count);
    const char* line = app->script.lines[app->cursor];
    app->mode = BksmModeReplace;
    app->target = app->cursor;

    size_t word_len = strcspn(line, " ");
    const char* arg = line[word_len] ? &line[word_len + 1] : "";

    for(size_t i = 0; i < bksm_commands_count; i++) {
        const BksmCommand* cmd = &bksm_commands[i];
        if(!cmd->name || strlen(cmd->name) != word_len || strncmp(line, cmd->name, word_len) != 0)
            continue;

        app->cmd = i;
        switch(cmd->arg) {
        case BksmArgText:
        case BksmArgTextOpt:
            strlcpy(app->text_buf, arg, sizeof(app->text_buf));
            scene_manager_next_scene(app->scene_manager, BksmSceneText);
            return;
        case BksmArgNumber:
            app->number_stage = 0;
            app->number_init = CLAMP(bksm_parse_int(arg, cmd->def), cmd->max, cmd->min);
            scene_manager_next_scene(app->scene_manager, BksmSceneNumber);
            return;
        case BksmArgMove: {
            app->number_stage = 0;
            app->number_init = CLAMP(bksm_parse_int(arg, 0), cmd->max, cmd->min);
            const char* y = strchr(arg, ' ');
            app->number_init_y = y ? CLAMP(bksm_parse_int(y + 1, 0), cmd->max, cmd->min) : 0;
            scene_manager_next_scene(app->scene_manager, BksmSceneNumber);
            return;
        }
        case BksmArgKey:
        case BksmArgHoldKey:
        case BksmArgMedia:
            app->pick_for_combo = false;
            scene_manager_next_scene(app->scene_manager, BksmScenePicker);
            return;
        default:
            break;
        }
        break;
    }

    /* Anything else (combos, mouse clicks, aliases, unknown): edit as raw text */
    for(size_t i = 0; i < bksm_commands_count; i++) {
        if(bksm_commands[i].arg == BksmArgRaw) app->cmd = i;
    }
    strlcpy(app->text_buf, line, sizeof(app->text_buf));
    scene_manager_next_scene(app->scene_manager, BksmSceneText);
}

/* ------------------------------------------------------------------ */
/* App lifecycle                                                       */
/* ------------------------------------------------------------------ */

static bool bksm_custom_event_callback(void* context, uint32_t event) {
    BksmApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool bksm_back_event_callback(void* context) {
    BksmApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static BksmApp* bksm_app_alloc(void) {
    BksmApp* app = malloc(sizeof(BksmApp));
    memset(app, 0, sizeof(BksmApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->file_path = furi_string_alloc();

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&bksm_scene_handlers, app);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, bksm_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, bksm_back_event_callback);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BksmViewSubmenu, submenu_get_view(app->submenu));
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BksmViewTextInput, text_input_get_view(app->text_input));
    app->number_input = number_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BksmViewNumberInput, number_input_get_view(app->number_input));
    app->var_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BksmViewVarList, variable_item_list_get_view(app->var_list));

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    return app;
}

static void bksm_app_free(BksmApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, BksmViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, BksmViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, BksmViewNumberInput);
    view_dispatcher_remove_view(app->view_dispatcher, BksmViewVarList);
    submenu_free(app->submenu);
    text_input_free(app->text_input);
    number_input_free(app->number_input);
    variable_item_list_free(app->var_list);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    bksm_script_clear(&app->script);
    furi_string_free(app->file_path);

    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t badkb_script_maker_app(void* p) {
    BksmApp* app = bksm_app_alloc();

    scene_manager_next_scene(app->scene_manager, BksmSceneStart);
    /* Launched with a script path (e.g. from the Archive): open it straight away */
    const char* arg = p;
    if(arg && arg[0] && bksm_load(app, arg)) {
        scene_manager_next_scene(app->scene_manager, BksmSceneEditor);
    }

    view_dispatcher_run(app->view_dispatcher);

    bksm_app_free(app);
    return 0;
}
