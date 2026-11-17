#include "../badkb_script_maker_i.h"
#include <toolbox/path.h>

/* Item ids above any line index */
enum {
    EditorItemAdd = 0x10000,
    EditorItemSave,
    EditorItemSaveAs,
};

static void bksm_scene_editor_set_header(BksmApp* app) {
    char header[40];
    FuriString* name = furi_string_alloc();
    if(furi_string_empty(app->file_path)) {
        furi_string_set(name, "new");
    } else {
        path_extract_filename(app->file_path, name, true);
    }
    snprintf(
        header,
        sizeof(header),
        "%s%.20s %u/%u",
        app->dirty ? "*" : "",
        furi_string_get_cstr(name),
        app->script.count,
        BKSM_MAX_LINES);
    submenu_set_header(app->submenu, header);
    furi_string_free(name);
}

void bksm_scene_editor_on_enter(void* context) {
    BksmApp* app = context;
    Submenu* submenu = app->submenu;
    char label[BKSM_LABEL_LEN + 1];

    submenu_reset(submenu);
    bksm_scene_editor_set_header(app);

    for(uint16_t i = 0; i < app->script.count; i++) {
        const char* line = app->script.lines[i];
        snprintf(label, sizeof(label), "%u %s", i + 1, line[0] ? line : "(empty)");
        submenu_add_item(submenu, label, i, bksm_submenu_callback, app);
    }
    submenu_add_item(submenu, "+ Add line", EditorItemAdd, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Save", EditorItemSave, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Save as...", EditorItemSaveAs, bksm_submenu_callback, app);

    submenu_set_selected_item(
        submenu, app->cursor < app->script.count ? app->cursor : (uint32_t)EditorItemAdd);

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewSubmenu);
}

bool bksm_scene_editor_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        if(event.event < app->script.count) {
            app->cursor = event.event;
            scene_manager_next_scene(app->scene_manager, BksmSceneLineMenu);
        } else if(event.event == EditorItemAdd) {
            app->cursor = app->script.count;
            app->mode = BksmModeInsert;
            app->target = app->script.count;
            scene_manager_set_scene_state(app->scene_manager, BksmSceneCommand, 0);
            scene_manager_next_scene(app->scene_manager, BksmSceneCommand);
        } else if(event.event == EditorItemSave && !furi_string_empty(app->file_path)) {
            if(bksm_save(app, furi_string_get_cstr(app->file_path))) {
                bksm_scene_editor_set_header(app);
            }
        } else if(event.event == EditorItemSave || event.event == EditorItemSaveAs) {
            scene_manager_next_scene(app->scene_manager, BksmSceneSaveName);
        }
    }

    return consumed;
}

void bksm_scene_editor_on_exit(void* context) {
    BksmApp* app = context;
    submenu_reset(app->submenu);
}
