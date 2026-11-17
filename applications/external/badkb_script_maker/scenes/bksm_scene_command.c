#include "../badkb_script_maker_i.h"

void bksm_scene_command_on_enter(void* context) {
    BksmApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Pick command");
    for(size_t i = 0; i < bksm_commands_count; i++) {
        submenu_add_item(submenu, bksm_commands[i].label, i, bksm_submenu_callback, app);
    }
    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, BksmSceneCommand));

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewSubmenu);
}

bool bksm_scene_command_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;

    if(event.type == SceneManagerEventTypeCustom && event.event < bksm_commands_count) {
        scene_manager_set_scene_state(app->scene_manager, BksmSceneCommand, event.event);
        bksm_begin_command(app, event.event);
        return true;
    }
    return false;
}

void bksm_scene_command_on_exit(void* context) {
    BksmApp* app = context;
    submenu_reset(app->submenu);
}
