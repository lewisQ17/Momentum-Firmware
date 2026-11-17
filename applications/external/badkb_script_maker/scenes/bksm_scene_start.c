#include "../badkb_script_maker_i.h"
#include <assets_icons.h>

enum {
    StartItemNew,
    StartItemOpen,
    StartItemResume,
};

void bksm_scene_start_on_enter(void* context) {
    BksmApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "BadKB Script Maker");
    if(app->script.count > 0 || !furi_string_empty(app->file_path)) {
        submenu_add_item(submenu, "Resume editing", StartItemResume, bksm_submenu_callback, app);
    }
    submenu_add_item(submenu, "New script", StartItemNew, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Open script", StartItemOpen, bksm_submenu_callback, app);
    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, BksmSceneStart));

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewSubmenu);
}

static void bksm_scene_start_open(BksmApp* app) {
    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, BKSM_EXTENSION, &I_badusb_10px);
    options.base_path = BKSM_BASE_FOLDER;
    options.skip_assets = true;
    options.hide_dot_files = true;

    FuriString* path = furi_string_alloc_set(
        furi_string_empty(app->file_path) ? BKSM_BASE_FOLDER : furi_string_get_cstr(app->file_path));
    if(dialog_file_browser_show(app->dialogs, path, path, &options)) {
        if(bksm_load(app, furi_string_get_cstr(path))) {
            scene_manager_next_scene(app->scene_manager, BksmSceneEditor);
        }
    }
    furi_string_free(path);
}

bool bksm_scene_start_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, BksmSceneStart, event.event);
        consumed = true;
        switch(event.event) {
        case StartItemResume:
            scene_manager_next_scene(app->scene_manager, BksmSceneEditor);
            break;
        case StartItemNew:
            if(!app->dirty ||
               bksm_confirm(app, "Unsaved changes", "Discard current\nscript?", "Discard")) {
                bksm_new_script(app);
                scene_manager_next_scene(app->scene_manager, BksmSceneEditor);
            }
            break;
        case StartItemOpen:
            if(!app->dirty ||
               bksm_confirm(app, "Unsaved changes", "Discard current\nscript?", "Discard")) {
                bksm_scene_start_open(app);
            }
            break;
        default:
            consumed = false;
            break;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        if(app->dirty &&
           !bksm_confirm(app, "Unsaved changes", "Exit without\nsaving?", "Exit")) {
            consumed = true; /* stay */
        }
    }

    return consumed;
}

void bksm_scene_start_on_exit(void* context) {
    BksmApp* app = context;
    submenu_reset(app->submenu);
}
