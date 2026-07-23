#include <string.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"

static void
    desktop_settings_scene_keybinds_seq_list_submenu_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void desktop_settings_scene_keybinds_seq_list_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    Submenu* submenu = app->submenu;
    submenu_reset(submenu);

    app->editing_sequence = false;

    FuriString* label = furi_string_alloc();
    FuriString* keys_str = furi_string_alloc();
    for(size_t i = 0; i < DESKTOP_KEYBIND_SEQ_COUNT; i++) {
        const DesktopKeybindSequence* s = &app->sequences[i];
        if(s->length > 0 && !furi_string_empty(s->action) && !furi_string_equal(s->action, "_")) {
            desktop_keybind_sequence_keys_str(s, keys_str);
            furi_string_printf(
                label,
                "%u: %s > %s",
                (unsigned)(i + 1),
                furi_string_get_cstr(keys_str),
                furi_string_get_cstr(s->action));
        } else {
            furi_string_printf(label, "%u: (empty)", (unsigned)(i + 1));
        }
        submenu_add_item(
            submenu,
            furi_string_get_cstr(label),
            i,
            desktop_settings_scene_keybinds_seq_list_submenu_callback,
            app);
    }
    furi_string_free(label);
    furi_string_free(keys_str);

    submenu_set_header(submenu, "Key Sequences:");
    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(
            app->scene_manager, DesktopSettingsAppSceneKeybindsSeqList));

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewMenu);
}

bool desktop_settings_scene_keybinds_seq_list_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        size_t index = event.event;
        if(index >= DESKTOP_KEYBIND_SEQ_COUNT) index = 0;
        scene_manager_set_scene_state(
            app->scene_manager, DesktopSettingsAppSceneKeybindsSeqList, index);

        // Preload the selected slot's existing keys into the edit buffer.
        app->seq_edit_index = index;
        const DesktopKeybindSequence* s = &app->sequences[index];
        app->seq_edit_len = s->length;
        memcpy(app->seq_edit_keys, s->keys, sizeof(app->seq_edit_keys));

        scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneKeybindsSeqEdit);
    }
    return consumed;
}

void desktop_settings_scene_keybinds_seq_list_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    submenu_reset(app->submenu);
}
