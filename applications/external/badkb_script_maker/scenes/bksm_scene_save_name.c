#include "../badkb_script_maker_i.h"
#include <toolbox/path.h>

static void bksm_scene_save_name_done(void* context) {
    BksmApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, BksmEventTextDone);
}

static bool bksm_save_name_validator(const char* text, FuriString* error, void* context) {
    UNUSED(context);
    if(text[0] == '.' || text[0] == ' ') {
        furi_string_set(error, "Cannot start\nwith . or space");
        return false;
    }
    for(const char* c = text; *c; c++) {
        if(strchr("\\/:*?\"<>|", *c) || (uint8_t)*c < 0x20 || (uint8_t)*c > 0x7E) {
            furi_string_set(error, "Invalid\ncharacter");
            return false;
        }
    }
    return true;
}

void bksm_scene_save_name_on_enter(void* context) {
    BksmApp* app = context;
    TextInput* text_input = app->text_input;

    if(furi_string_empty(app->file_path)) {
        strlcpy(app->name_buf, "script", sizeof(app->name_buf));
    } else {
        FuriString* name = furi_string_alloc();
        path_extract_filename(app->file_path, name, true);
        strlcpy(app->name_buf, furi_string_get_cstr(name), sizeof(app->name_buf));
        furi_string_free(name);
    }

    text_input_reset(text_input);
    text_input_set_header_text(text_input, "Save as (in badusb/)");
    text_input_set_result_callback(
        text_input, bksm_scene_save_name_done, app, app->name_buf, sizeof(app->name_buf), true);
    text_input_set_validator(text_input, bksm_save_name_validator, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewTextInput);
}

bool bksm_scene_save_name_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != BksmEventTextDone) {
        return false;
    }

    /* Strip a typed ".txt" so it is not doubled */
    size_t len = strlen(app->name_buf);
    size_t ext_len = strlen(BKSM_EXTENSION);
    if(len > ext_len && strcmp(&app->name_buf[len - ext_len], BKSM_EXTENSION) == 0) {
        app->name_buf[len - ext_len] = '\0';
    }

    FuriString* path =
        furi_string_alloc_printf("%s/%s%s", BKSM_BASE_FOLDER, app->name_buf, BKSM_EXTENSION);
    const char* path_c = furi_string_get_cstr(path);

    bool proceed = true;
    if(!furi_string_equal(path, app->file_path) &&
       storage_common_exists(app->storage, path_c)) {
        proceed = bksm_confirm(app, "File exists", "Overwrite it?", "Overwrite");
    }
    if(proceed) {
        storage_simply_mkdir(app->storage, BKSM_BASE_FOLDER);
        if(bksm_save(app, path_c)) {
            bksm_return_to_editor(app);
        }
    }

    furi_string_free(path);
    return true;
}

void bksm_scene_save_name_on_exit(void* context) {
    BksmApp* app = context;
    text_input_reset(app->text_input);
}
