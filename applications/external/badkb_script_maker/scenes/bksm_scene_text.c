#include "../badkb_script_maker_i.h"

static void bksm_scene_text_done(void* context) {
    BksmApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, BksmEventTextDone);
}

void bksm_scene_text_on_enter(void* context) {
    BksmApp* app = context;
    const BksmCommand* cmd = &bksm_commands[app->cmd];
    TextInput* text_input = app->text_input;

    /* Bound the argument so "<NAME> <arg>" always fits in one script line */
    size_t buf_size = sizeof(app->text_buf);
    if(cmd->name) buf_size -= strlen(cmd->name) + 1;
    app->text_buf[buf_size - 1] = '\0';

    text_input_reset(text_input);
    text_input_set_header_text(text_input, cmd->hint ? cmd->hint : cmd->label);
    text_input_set_result_callback(
        text_input, bksm_scene_text_done, app, app->text_buf, buf_size, false);
    /* Script text needs the full ASCII set, not just filename-safe symbols */
    text_input_show_illegal_symbols(text_input, true);
    text_input_set_minimum_length(text_input, cmd->arg == BksmArgTextOpt ? 0 : 1);

    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewTextInput);
}

bool bksm_scene_text_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;

    if(event.type == SceneManagerEventTypeCustom && event.event == BksmEventTextDone) {
        bksm_compose_and_commit(app, app->text_buf);
        return true;
    }
    return false;
}

void bksm_scene_text_on_exit(void* context) {
    BksmApp* app = context;
    text_input_reset(app->text_input);
}
