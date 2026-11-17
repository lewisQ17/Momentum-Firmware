#include "../badkb_script_maker_i.h"

static void bksm_scene_number_done(void* context, int32_t number) {
    BksmApp* app = context;
    app->number_value = number;
    view_dispatcher_send_custom_event(app->view_dispatcher, BksmEventNumberDone);
}

static void bksm_scene_number_setup(BksmApp* app) {
    const BksmCommand* cmd = &bksm_commands[app->cmd];
    const char* header = cmd->hint;
    int32_t current = app->number_init;

    if(cmd->arg == BksmArgMove) {
        header = app->number_stage == 0 ? "Mouse X (-127..127)" : "Mouse Y (-127..127)";
        if(app->number_stage == 1) current = app->number_init_y;
    }
    current = CLAMP(current, cmd->max, cmd->min);

    number_input_set_header_text(app->number_input, header);
    number_input_set_result_callback(
        app->number_input, bksm_scene_number_done, app, current, cmd->min, cmd->max);
}

void bksm_scene_number_on_enter(void* context) {
    BksmApp* app = context;
    bksm_scene_number_setup(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BksmViewNumberInput);
}

bool bksm_scene_number_on_event(void* context, SceneManagerEvent event) {
    BksmApp* app = context;
    const BksmCommand* cmd = &bksm_commands[app->cmd];

    if(event.type != SceneManagerEventTypeCustom || event.event != BksmEventNumberDone) {
        return false;
    }

    char arg[24];
    if(cmd->arg == BksmArgMove) {
        if(app->number_stage == 0) {
            /* keep X in number_init, ask Y next */
            app->number_init = app->number_value;
            app->number_stage = 1;
            bksm_scene_number_setup(app);
            return true;
        }
        snprintf(arg, sizeof(arg), "%ld %ld", (long)app->number_init, (long)app->number_value);
    } else {
        snprintf(arg, sizeof(arg), "%ld", (long)app->number_value);
    }
    bksm_compose_and_commit(app, arg);
    return true;
}

void bksm_scene_number_on_exit(void* context) {
    UNUSED(context);
}
