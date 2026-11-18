#include <string.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"

typedef enum {
    SeqEditItemUp,
    SeqEditItemDown,
    SeqEditItemRight,
    SeqEditItemLeft,
    SeqEditItemBackspace,
    SeqEditItemClear,
    SeqEditItemSetAction,
} SeqEditItem;

// SeqEditItemUp..SeqEditItemLeft map 1:1 to DesktopKeybindKey values.
static const DesktopKeybindKey seq_edit_item_to_key[] = {
    DesktopKeybindKeyUp,
    DesktopKeybindKeyDown,
    DesktopKeybindKeyRight,
    DesktopKeybindKeyLeft,
};

static void
    desktop_settings_scene_keybinds_seq_edit_submenu_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void desktop_settings_scene_keybinds_seq_edit_build(DesktopSettingsApp* app) {
    Submenu* submenu = app->submenu;
    submenu_reset(submenu);

    submenu_add_item(
        submenu,
        "Add Up",
        SeqEditItemUp,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Add Down",
        SeqEditItemDown,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Add Right",
        SeqEditItemRight,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Add Left",
        SeqEditItemLeft,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Backspace",
        SeqEditItemBackspace,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Clear",
        SeqEditItemClear,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Set Action (min 2 keys)",
        SeqEditItemSetAction,
        desktop_settings_scene_keybinds_seq_edit_submenu_callback,
        app);

    DesktopKeybindSequence tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.length = app->seq_edit_len;
    memcpy(tmp.keys, app->seq_edit_keys, sizeof(tmp.keys));
    FuriString* keys_str = furi_string_alloc();
    desktop_keybind_sequence_keys_str(&tmp, keys_str);
    FuriString* header = furi_string_alloc_printf("Seq: %s", furi_string_get_cstr(keys_str));
    submenu_set_header(submenu, furi_string_get_cstr(header));
    furi_string_free(header);
    furi_string_free(keys_str);
}

// Sequences are layered on top of the single-key keybinds: every press still
// runs its own Press keybind, which opens a menu or an app and takes the keys
// away from the desktop. Tell the user when that keeps this sequence from ever
// completing (the last key is fine, a completed sequence replaces its keybind).
static void desktop_settings_scene_keybinds_seq_edit_warn(DesktopSettingsApp* app) {
    for(uint8_t i = 0; i + 1 < app->seq_edit_len; i++) {
        const uint8_t key = app->seq_edit_keys[i];
        if(key >= DesktopKeybindKeyMAX) continue;
        if(furi_string_equal(app->keybinds[DesktopKeybindTypePress][key], "_")) continue;

        DesktopKeybindSequence tmp;
        memset(&tmp, 0, sizeof(tmp));
        tmp.length = 1;
        tmp.keys[0] = key;
        FuriString* key_str = furi_string_alloc();
        desktop_keybind_sequence_keys_str(&tmp, key_str);
        FuriString* text = furi_string_alloc_printf(
            "Press %s runs its own\nkeybind first. Remove it\nfor this sequence to work.",
            furi_string_get_cstr(key_str));
        furi_string_free(key_str);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Keybind conflict", 64, 0, AlignCenter, AlignTop);
        dialog_message_set_text(
            message, furi_string_get_cstr(text), 64, 14, AlignCenter, AlignTop);
        dialog_message_set_buttons(message, NULL, "OK", NULL);
        dialog_message_show(app->dialogs, message);
        dialog_message_free(message);
        furi_string_free(text);
        return;
    }
}

void desktop_settings_scene_keybinds_seq_edit_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    desktop_settings_scene_keybinds_seq_edit_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewMenu);
}

bool desktop_settings_scene_keybinds_seq_edit_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        switch(event.event) {
        case SeqEditItemUp:
        case SeqEditItemDown:
        case SeqEditItemRight:
        case SeqEditItemLeft:
            if(app->seq_edit_len < DESKTOP_KEYBIND_SEQ_MAX_LEN) {
                app->seq_edit_keys[app->seq_edit_len++] =
                    (uint8_t)seq_edit_item_to_key[event.event];
                desktop_settings_scene_keybinds_seq_edit_build(app);
                submenu_set_selected_item(app->submenu, event.event);
            }
            break;
        case SeqEditItemBackspace:
            if(app->seq_edit_len > 0) app->seq_edit_len--;
            desktop_settings_scene_keybinds_seq_edit_build(app);
            submenu_set_selected_item(app->submenu, SeqEditItemBackspace);
            break;
        case SeqEditItemClear:
            app->seq_edit_len = 0;
            desktop_settings_scene_keybinds_seq_edit_build(app);
            submenu_set_selected_item(app->submenu, SeqEditItemClear);
            break;
        case SeqEditItemSetAction:
            // A sequence needs at least two keys to differ from a single-key keybind.
            if(app->seq_edit_len >= 2) {
                desktop_settings_scene_keybinds_seq_edit_warn(app);
                app->editing_sequence = true;
                scene_manager_set_scene_state(
                    app->scene_manager,
                    DesktopSettingsAppSceneKeybindsActionType,
                    DesktopSettingsAppKeybindActionTypeRemoveKeybind);
                scene_manager_next_scene(
                    app->scene_manager, DesktopSettingsAppSceneKeybindsActionType);
            }
            break;
        default:
            break;
        }
    }
    return consumed;
}

void desktop_settings_scene_keybinds_seq_edit_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    submenu_reset(app->submenu);
}
