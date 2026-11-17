#include "../badkb_script_maker_i.h"

/* Row order in the variable item list */
enum {
    ComboRowCtrl,
    ComboRowShift,
    ComboRowAlt,
    ComboRowGui,
    ComboRowKey,
    ComboRowAdd,
};

static const char* const combo_mod_names[] = {"CTRL", "SHIFT", "ALT", "GUI"};
static const char* const off_on[] = {"Off", "On"};

static void bksm_combo_mod_changed(VariableItem* item) {
    BksmApp* app = variable_item_get_context(item);
    uint8_t row = variable_item_list_get_selected_item_index(app->var_list);
    if(row > ComboRowGui) return;
    uint8_t on = variable_item_get_current_value_index(item);
    if(on) {
        app->combo_mods |= (1 << row);
    } else {
        app->combo_mods &= ~(1 << row);
    }
    variable_item_set_current_value_text(item, off_on[on]);
}

static void bksm_combo_key_changed(VariableItem* item) {
    BksmApp* app = variable_item_get_context(item);
    app->combo_key = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(
        item, app->combo_key ? bksm_keys[app->combo_key - 1] : "(none)");
}

static void bksm_combo_enter(void* context, uint32_t index) {
    BksmApp* app = context;
    if(index == ComboRowKey) {
        view_dispatcher_send_custom_event(app->view_dispatcher, BksmEventComboPickKey);
    } else if(index == ComboRowAdd) {
        view_dispatcher_send_custom_event(app->view_dispatcher, BksmEventComboAdd);
    }
}

void bksm_scene_combo_on_enter(void* context) {
    BksmApp* app = context;
    VariableItemList* list = app->var_list;
    VariableItem* item;

    variable_item_list_reset(list);
    variable_item_list_set_header(list, "Key / combo");
    for(uint8_t i = 0; i < COUNT_OF(combo_mod_names); i++) {
        uint8_t on = (app->combo_mods >> i) & 1;
        item = variable_item_list_add(list, combo_mod_names[i], 2, bksm_combo_mod_changed, app);
        variable_item_set_current_value_index(item, on);
        variable_item_set_current_value_text(item, off_on[on]);
    }
    item = variable_item_list_add(list, "Key", bksm_keys_count + 1, bksm_combo_key_changed, app);
    variable_item_set_current_value_index(item, app->combo_key);
    variable_item_set_current_value_text(
        item, app->combo_key ? bksm_keys[app->combo_key - 1] : "(none)");
    variable_item_list_add(list, "Add line", 0, NULL, app);

    variable_item_list_set_enter_callback(list, bksm_combo_enter, app);
    variable_item_list_set_selected_item(
        list, scene_manager_get_scene_state(app->scene_manager, BksmSceneCombo));

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewVarList);
}

static void bksm_combo_commit(BksmApp* app) {
    char line[64] = {0};
    size_t pos = 0;
    for(uint8_t i = 0; i < COUNT_OF(combo_mod_names); i++) {
        if(!((app->combo_mods >> i) & 1)) continue;
        pos += snprintf(
            line + pos, sizeof(line) - pos, "%s%s", pos ? " " : "", combo_mod_names[i]);
    }
    if(app->combo_key) {
        snprintf(
            line + pos, sizeof(line) - pos, "%s%s", pos ? " " : "", bksm_keys[app->combo_key - 1]);
    }
    if(line[0] == '\0') {
        bksm_show_message(app, "Empty combo", "Turn on a modifier\nor pick a key");
        return;
    }
    bksm_compose_and_commit(app, line);
}

bool bksm_scene_combo_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == BksmEventComboPickKey) {
        scene_manager_set_scene_state(app->scene_manager, BksmSceneCombo, ComboRowKey);
        app->pick_for_combo = true;
        scene_manager_next_scene(app->scene_manager, BksmScenePicker);
        return true;
    } else if(event.event == BksmEventComboAdd) {
        scene_manager_set_scene_state(app->scene_manager, BksmSceneCombo, ComboRowAdd);
        bksm_combo_commit(app);
        return true;
    }
    return false;
}

void bksm_scene_combo_on_exit(void* context) {
    BksmApp* app = context;
    variable_item_list_set_selected_item(app->var_list, 0);
    variable_item_list_reset(app->var_list);
}
