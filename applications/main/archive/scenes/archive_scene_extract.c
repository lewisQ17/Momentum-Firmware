#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include <toolbox/tar/tar_archive.h>
#include <toolbox/path.h>
#include <storage/storage.h>

// Scene state: 0 = confirm prompt, 1 = result shown.
#define EXTRACT_STATE_CONFIRM (0UL)
#define EXTRACT_STATE_RESULT  (1UL)

static void
    archive_scene_extract_button_callback(GuiButtonType result, InputType type, void* context) {
    furi_assert(context);
    ArchiveApp* app = (ArchiveApp*)context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(app->view_dispatcher, result);
    }
}

// Derive the output directory by stripping the archive extension from the path.
static void archive_extract_dest_from_path(const char* archive_path, FuriString* dest) {
    furi_string_set_str(dest, archive_path);
    if(furi_string_end_withi_str(dest, ".tar.gz")) {
        furi_string_left(dest, furi_string_size(dest) - strlen(".tar.gz"));
    } else if(furi_string_end_withi_str(dest, ".tgz")) {
        furi_string_left(dest, furi_string_size(dest) - strlen(".tgz"));
    } else if(furi_string_end_withi_str(dest, ".tar")) {
        furi_string_left(dest, furi_string_size(dest) - strlen(".tar"));
    }
}

// Blocking unpack (mirrors how the OTA updater / storage_cli extract). Runs while
// the loading popup is shown on the stack view, which is drawn by the GUI service
// thread independently of this (blocking) app thread.
static bool archive_extract_perform(const char* archive_path, const char* dest_dir) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, dest_dir); // no-op if it already exists
    TarArchive* tar = tar_archive_alloc(storage);
    bool ok = false;
    if(tar_archive_open(tar, archive_path, tar_archive_get_mode_for_path(archive_path))) {
        ok = tar_archive_unpack_to(tar, dest_dir, NULL);
    }
    tar_archive_free(tar);
    furi_record_close(RECORD_STORAGE);
    return ok;
}

void archive_scene_extract_on_enter(void* context) {
    furi_assert(context);
    ArchiveApp* app = (ArchiveApp*)context;

    ArchiveFile_t* current = archive_get_current_file(app->browser);
    FuriString* filename = furi_string_alloc();
    path_extract_filename(current->path, filename, false);

    char prompt[96];
    snprintf(prompt, sizeof(prompt), "\e#Extract %s?\e#", furi_string_get_cstr(filename));

    widget_add_text_box_element(
        app->widget, 0, 0, 128, 40, AlignCenter, AlignCenter, prompt, false);
    widget_add_button_element(
        app->widget, GuiButtonTypeLeft, "Cancel", archive_scene_extract_button_callback, app);
    widget_add_button_element(
        app->widget, GuiButtonTypeRight, "Extract", archive_scene_extract_button_callback, app);

    furi_string_free(filename);
    view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewWidget);
}

bool archive_scene_extract_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    ArchiveApp* app = (ArchiveApp*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint32_t state = scene_manager_get_scene_state(app->scene_manager, ArchiveAppSceneExtract);

        if(state == EXTRACT_STATE_CONFIRM && event.event == GuiButtonTypeRight) {
            ArchiveFile_t* current = archive_get_current_file(app->browser);
            FuriString* archive_path = furi_string_alloc_set(current->path);
            FuriString* dest = furi_string_alloc();
            archive_extract_dest_from_path(furi_string_get_cstr(archive_path), dest);

            view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewStack);
            archive_show_loading_popup(app, true);
            bool ok = archive_extract_perform(
                furi_string_get_cstr(archive_path), furi_string_get_cstr(dest));
            archive_show_loading_popup(app, false);

            widget_reset(app->widget);
            char result_str[96];
            if(ok) {
                snprintf(
                    result_str,
                    sizeof(result_str),
                    "\e#Extracted to\e#\n%s",
                    furi_string_get_cstr(dest));
            } else {
                snprintf(result_str, sizeof(result_str), "\e#Extract failed\e#");
            }
            widget_add_text_box_element(
                app->widget, 0, 0, 128, 40, AlignCenter, AlignCenter, result_str, false);
            widget_add_button_element(
                app->widget,
                GuiButtonTypeCenter,
                "OK",
                archive_scene_extract_button_callback,
                app);
            view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewWidget);

            scene_manager_set_scene_state(
                app->scene_manager, ArchiveAppSceneExtract, EXTRACT_STATE_RESULT);

            furi_string_free(archive_path);
            furi_string_free(dest);
            consumed = true;
        } else if(state == EXTRACT_STATE_CONFIRM && event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(app->scene_manager);
        } else if(state == EXTRACT_STATE_RESULT) {
            // Any button (OK) returns to the browser.
            consumed = scene_manager_previous_scene(app->scene_manager);
        }
    }
    return consumed;
}

void archive_scene_extract_on_exit(void* context) {
    furi_assert(context);
    ArchiveApp* app = (ArchiveApp*)context;
    scene_manager_set_scene_state(
        app->scene_manager, ArchiveAppSceneExtract, EXTRACT_STATE_CONFIRM);
    widget_reset(app->widget);
}
