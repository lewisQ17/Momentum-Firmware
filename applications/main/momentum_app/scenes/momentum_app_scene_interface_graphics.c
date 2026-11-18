#include "../momentum_app.h"

enum VarItemListIndex {
    VarItemListIndexAssetPack,
    VarItemListIndexAnimSpeed,
    VarItemListIndexCycleAnims,
    VarItemListIndexUnlockAnims,
    VarItemListIndexIconPreview,
};

void momentum_app_scene_interface_graphics_var_item_list_callback(void* context, uint32_t index) {
    MomentumApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void momentum_app_scene_interface_graphics_asset_pack_changed(VariableItem* item) {
    MomentumApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(
        item, index == 0 ? "Default" : *CharList_get(app->asset_pack_names, index - 1));
    strlcpy(
        momentum_settings.asset_pack,
        index == 0 ? "" : *CharList_get(app->asset_pack_names, index - 1),
        ASSET_PACKS_NAME_LEN);
    app->asset_pack_index = index;
    app->save_settings = true;
    app->apply_pack = true;
}

const char* const anim_speed_names[] = {
    "25%",
    "50%",
    "75%",
    "100%",
    "125%",
    "150%",
    "175%",
    "200%",
    "225%",
    "250%",
    "275%",
    "300%",
};
const uint32_t anim_speed_values[COUNT_OF(anim_speed_names)] = {
    25,
    50,
    75,
    100,
    125,
    150,
    175,
    200,
    225,
    250,
    275,
    300,
};
static void momentum_app_scene_interface_graphics_anim_speed_changed(VariableItem* item) {
    MomentumApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, anim_speed_names[index]);
    momentum_settings.anim_speed = anim_speed_values[index];
    app->save_settings = true;
}

const char* const cycle_anims_names[] = {
    "OFF",
    "Meta.txt",
    "15 S",
    "30 S",
    "1 M",
    "2 M",
    "5 M",
    "10 M",
    "15 M",
    "30 M",
    "1 H",
    "2 H",
    "6 H",
    "12 H",
    "24 H",
};
const int32_t cycle_anims_values[COUNT_OF(cycle_anims_names)] = {
    -1,
    0,
    15,
    30,
    60,
    120,
    300,
    600,
    900,
    1800,
    3600,
    7200,
    21600,
    43200,
    86400,
};
static void momentum_app_scene_interface_graphics_cycle_anims_changed(VariableItem* item) {
    MomentumApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, cycle_anims_names[index]);
    momentum_settings.cycle_anims = cycle_anims_values[index];
    app->save_settings = true;
}

static void momentum_app_scene_interface_graphics_unlock_anims_changed(VariableItem* item) {
    MomentumApp* app = variable_item_get_context(item);
    bool value = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, value ? "ON" : "OFF");
    momentum_settings.unlock_anims = value;
    app->save_settings = true;
}

void momentum_app_scene_interface_graphics_on_enter(void* context) {
    MomentumApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;
    VariableItem* item;
    uint8_t value_index;

    momentum_app_load_asset_pack_names(app);

    item = variable_item_list_add(
        var_item_list,
        "Asset Pack",
        CharList_size(app->asset_pack_names) + 1,
        momentum_app_scene_interface_graphics_asset_pack_changed,
        app);
    variable_item_set_current_value_index(item, app->asset_pack_index);
    variable_item_set_current_value_text(
        item,
        app->asset_pack_index == 0 ?
            "Default" :
            *CharList_get(app->asset_pack_names, app->asset_pack_index - 1));

    item = variable_item_list_add(
        var_item_list,
        "Anim Speed",
        COUNT_OF(anim_speed_names),
        momentum_app_scene_interface_graphics_anim_speed_changed,
        app);
    value_index = value_index_uint32(
        momentum_settings.anim_speed, anim_speed_values, COUNT_OF(anim_speed_values));
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, anim_speed_names[value_index]);

    item = variable_item_list_add(
        var_item_list,
        "Cycle Anims",
        COUNT_OF(cycle_anims_names),
        momentum_app_scene_interface_graphics_cycle_anims_changed,
        app);
    value_index = value_index_int32(
        momentum_settings.cycle_anims, cycle_anims_values, COUNT_OF(cycle_anims_values));
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, cycle_anims_names[value_index]);

    item = variable_item_list_add(
        var_item_list,
        "Unlock Anims",
        2,
        momentum_app_scene_interface_graphics_unlock_anims_changed,
        app);
    variable_item_set_current_value_index(item, momentum_settings.unlock_anims);
    variable_item_set_current_value_text(item, momentum_settings.unlock_anims ? "ON" : "OFF");

    // Button-style row (0 values, no change callback): browse to a loose .bmx
    // file or icon folder and preview it, exercising asset_packs_icon_load_file().
    variable_item_list_add(var_item_list, "Preview Icon File", 0, NULL, app);

    variable_item_list_set_enter_callback(
        var_item_list, momentum_app_scene_interface_graphics_var_item_list_callback, app);

    variable_item_list_set_selected_item(
        var_item_list,
        scene_manager_get_scene_state(app->scene_manager, MomentumAppSceneInterfaceGraphics));

    view_dispatcher_switch_to_view(app->view_dispatcher, MomentumAppViewVarItemList);
}

bool momentum_app_scene_interface_graphics_on_event(void* context, SceneManagerEvent event) {
    MomentumApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(
            app->scene_manager, MomentumAppSceneInterfaceGraphics, event.event);
        consumed = true;
        switch(event.event) {
        case VarItemListIndexAssetPack:
            scene_manager_next_scene(app->scene_manager, MomentumAppSceneInterfaceGraphicsPack);
            break;
        case VarItemListIndexIconPreview: {
            const DialogsFileBrowserOptions browser_options = {
                .extension = ".bmx",
                .icon = &I_unknown_10px,
                .skip_assets = false,
                .hide_ext = false,
                .item_loader_callback = NULL,
                .item_loader_context = NULL,
                .base_path = STORAGE_EXT_PATH_PREFIX,
                .select_right = true, // Right selects a folder = animated icon
            };
            if(dialog_file_browser_show(
                   app->dialogs, app->icon_file_path, app->icon_file_path, &browser_options)) {
                asset_packs_icon_free(app->icon_preview);
                app->icon_preview =
                    asset_packs_icon_load_file(furi_string_get_cstr(app->icon_file_path));
                if(app->icon_preview) {
                    scene_manager_next_scene(
                        app->scene_manager, MomentumAppSceneInterfaceGraphicsIcon);
                } else {
                    DialogMessage* message = dialog_message_alloc();
                    dialog_message_set_header(
                        message, "Invalid Icon", 64, 3, AlignCenter, AlignTop);
                    dialog_message_set_text(
                        message,
                        "Not a .bmx file\nor icon folder.",
                        64,
                        32,
                        AlignCenter,
                        AlignCenter);
                    dialog_message_set_buttons(message, NULL, "OK", NULL);
                    dialog_message_show(app->dialogs, message);
                    dialog_message_free(message);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    return consumed;
}

void momentum_app_scene_interface_graphics_on_exit(void* context) {
    MomentumApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
