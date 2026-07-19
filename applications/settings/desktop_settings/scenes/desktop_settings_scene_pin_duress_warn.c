#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"

static void
    desktop_settings_scene_pin_duress_warn_dialog_callback(DialogExResult result, void* context) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, result);
}

void desktop_settings_scene_pin_duress_warn_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    DialogEx* dialog_ex = app->dialog_ex;

    dialog_ex_set_header(dialog_ex, "Duress PIN wipes data!", 64, 8, AlignCenter, AlignCenter);
    dialog_ex_set_text(
        dialog_ex,
        "Entering it at the lock\nscreen erases the SD card\n& settings, then reboots.",
        64,
        34,
        AlignCenter,
        AlignCenter);
    dialog_ex_set_left_button_text(dialog_ex, "Cancel");
    dialog_ex_set_right_button_text(dialog_ex, "Set it");

    dialog_ex_set_context(dialog_ex, app);
    dialog_ex_set_result_callback(
        dialog_ex, desktop_settings_scene_pin_duress_warn_dialog_callback);

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewDialogEx);
}

bool desktop_settings_scene_pin_duress_warn_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DialogExResultRight:
            // Proceed into the shared PIN-setup flow; setup_done commits to the
            // duress register because app->pin_setup_duress is set.
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinSetupHowto);
            consumed = true;
            break;
        case DialogExResultLeft:
            app->pin_setup_duress = false;
            consumed = scene_manager_previous_scene(app->scene_manager);
            break;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        app->pin_setup_duress = false;
        consumed = scene_manager_previous_scene(app->scene_manager);
    }

    return consumed;
}

void desktop_settings_scene_pin_duress_warn_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
