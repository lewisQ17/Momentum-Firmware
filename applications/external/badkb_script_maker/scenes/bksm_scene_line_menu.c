#include "../badkb_script_maker_i.h"

enum {
    LineItemEdit,
    LineItemChange,
    LineItemInsertAbove,
    LineItemInsertBelow,
    LineItemDuplicate,
    LineItemMoveUp,
    LineItemMoveDown,
    LineItemDelete,
};

void bksm_scene_line_menu_on_enter(void* context) {
    BksmApp* app = context;
    Submenu* submenu = app->submenu;
    char header[24];

    submenu_reset(submenu);
    snprintf(header, sizeof(header), "Line %u", app->cursor + 1);
    submenu_set_header(submenu, header);
    submenu_add_item(submenu, "Edit", LineItemEdit, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Change command", LineItemChange, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Insert above", LineItemInsertAbove, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Insert below", LineItemInsertBelow, bksm_submenu_callback, app);
    submenu_add_item(submenu, "Duplicate", LineItemDuplicate, bksm_submenu_callback, app);
    if(app->cursor > 0) {
        submenu_add_item(submenu, "Move up", LineItemMoveUp, bksm_submenu_callback, app);
    }
    if(app->cursor + 1 < app->script.count) {
        submenu_add_item(submenu, "Move down", LineItemMoveDown, bksm_submenu_callback, app);
    }
    submenu_add_item(submenu, "Delete", LineItemDelete, bksm_submenu_callback, app);
    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, BksmSceneLineMenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewSubmenu);
}

static void bksm_line_menu_pick_command(BksmApp* app, BksmMode mode, uint16_t target) {
    app->mode = mode;
    app->target = target;
    scene_manager_set_scene_state(app->scene_manager, BksmSceneCommand, 0);
    scene_manager_next_scene(app->scene_manager, BksmSceneCommand);
}

bool bksm_scene_line_menu_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    bool consumed = false;

    if(event.type != SceneManagerEventTypeCustom) return false;
    if(app->cursor >= app->script.count) { /* stale selection, should not happen */
        bksm_return_to_editor(app);
        return true;
    }

    consumed = true;
    scene_manager_set_scene_state(app->scene_manager, BksmSceneLineMenu, event.event);
    BksmScript* script = &app->script;
    uint16_t cur = app->cursor;

    switch(event.event) {
    case LineItemEdit:
        bksm_begin_edit(app);
        break;
    case LineItemChange:
        bksm_line_menu_pick_command(app, BksmModeReplace, cur);
        break;
    case LineItemInsertAbove:
        bksm_line_menu_pick_command(app, BksmModeInsert, cur);
        break;
    case LineItemInsertBelow:
        bksm_line_menu_pick_command(app, BksmModeInsert, cur + 1);
        break;
    case LineItemDuplicate: {
        /* copy first: the source pointer may move during insert */
        strlcpy(app->line_buf, script->lines[cur], sizeof(app->line_buf));
        BksmScriptResult res = bksm_script_insert(script, cur + 1, app->line_buf);
        if(res == BksmScriptOk) {
            app->cursor = cur + 1;
            app->dirty = true;
            bksm_return_to_editor(app);
        } else {
            bksm_show_message(app, "Not added", bksm_script_result_text(res));
        }
        break;
    }
    case LineItemMoveUp:
        if(cur > 0) {
            bksm_script_swap(script, cur, cur - 1);
            app->cursor = cur - 1;
            app->dirty = true;
        }
        bksm_return_to_editor(app);
        break;
    case LineItemMoveDown:
        if(cur + 1 < script->count) {
            bksm_script_swap(script, cur, cur + 1);
            app->cursor = cur + 1;
            app->dirty = true;
        }
        bksm_return_to_editor(app);
        break;
    case LineItemDelete: {
        char text[48];
        snprintf(text, sizeof(text), "%.40s", script->lines[cur]);
        if(bksm_confirm(app, "Delete line?", text, "Delete")) {
            bksm_script_delete(script, cur);
            if(app->cursor >= script->count && app->cursor > 0) app->cursor--;
            app->dirty = true;
            bksm_return_to_editor(app);
        }
        break;
    }
    default:
        consumed = false;
        break;
    }

    return consumed;
}

void bksm_scene_line_menu_on_exit(void* context) {
    BksmApp* app = context;
    submenu_reset(app->submenu);
}
