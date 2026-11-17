#include "../badkb_script_maker_i.h"

/* Item ids: hold extras get an offset so they never clash with bksm_keys indexes */
#define PICK_EXTRA_BASE 0x100

static BksmPickKind bksm_picker_kind(BksmApp* app) {
    if(app->pick_for_combo) return BksmPickKeys;
    switch(bksm_commands[app->cmd].arg) {
    case BksmArgHoldKey:
        return BksmPickHold;
    case BksmArgMedia:
        return BksmPickMedia;
    case BksmArgMouse:
        return BksmPickMouse;
    default:
        return BksmPickKeys;
    }
}

void bksm_scene_picker_on_enter(void* context) {
    BksmApp* app = context;
    Submenu* submenu = app->submenu;
    BksmPickKind kind = bksm_picker_kind(app);

    submenu_reset(submenu);
    switch(kind) {
    case BksmPickMedia:
        submenu_set_header(submenu, "Media key");
        for(size_t i = 0; i < bksm_media_keys_count; i++) {
            submenu_add_item(submenu, bksm_media_keys[i], i, bksm_submenu_callback, app);
        }
        break;
    case BksmPickMouse:
        submenu_set_header(submenu, "Mouse button");
        for(size_t i = 0; i < bksm_mouse_buttons_count; i++) {
            submenu_add_item(submenu, bksm_mouse_buttons[i], i, bksm_submenu_callback, app);
        }
        break;
    case BksmPickHold:
        submenu_set_header(submenu, "Key to hold/release");
        for(size_t i = 0; i < bksm_hold_extra_count; i++) {
            submenu_add_item(
                submenu, bksm_hold_extra[i], PICK_EXTRA_BASE + i, bksm_submenu_callback, app);
        }
        for(size_t i = 0; i < bksm_keys_count; i++) {
            submenu_add_item(submenu, bksm_keys[i], i, bksm_submenu_callback, app);
        }
        break;
    case BksmPickKeys:
        submenu_set_header(submenu, "Key");
        for(size_t i = 0; i < bksm_keys_count; i++) {
            submenu_add_item(submenu, bksm_keys[i], i, bksm_submenu_callback, app);
        }
        break;
    }
    if(app->pick_for_combo && app->combo_key > 0) {
        submenu_set_selected_item(submenu, app->combo_key - 1);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewSubmenu);
}

bool bksm_scene_picker_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    BksmPickKind kind = bksm_picker_kind(app);
    const char* name = NULL;
    uint32_t id = event.event;

    switch(kind) {
    case BksmPickMedia:
        if(id < bksm_media_keys_count) name = bksm_media_keys[id];
        break;
    case BksmPickMouse:
        if(id < bksm_mouse_buttons_count) name = bksm_mouse_buttons[id];
        break;
    case BksmPickHold:
        if(id >= PICK_EXTRA_BASE && id - PICK_EXTRA_BASE < bksm_hold_extra_count) {
            name = bksm_hold_extra[id - PICK_EXTRA_BASE];
        } else if(id < bksm_keys_count) {
            name = bksm_keys[id];
        }
        break;
    case BksmPickKeys:
        if(id < bksm_keys_count) name = bksm_keys[id];
        break;
    }
    if(!name) return false;

    if(app->pick_for_combo) {
        app->combo_key = id + 1;
        scene_manager_previous_scene(app->scene_manager);
    } else {
        bksm_compose_and_commit(app, name);
    }
    return true;
}

void bksm_scene_picker_on_exit(void* context) {
    BksmApp* app = context;
    submenu_reset(app->submenu);
}
