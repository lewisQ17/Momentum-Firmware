#include "../momentum_app.h"

void momentum_app_scene_misc_credits_on_enter(void* context) {
    MomentumApp* app = context;
    Widget* widget = app->widget;
    widget_reset(widget);

    widget_add_string_element(widget, 64, 2, AlignCenter, AlignTop, FontPrimary, "Credits");
    widget_add_text_scroll_element(
        widget,
        0,
        15,
        128,
        49,
        "Momentum Firmware\n"
        "by the Momentum team\n"
        "(Next-Flip)\n"
        "\n"
        "Built on Unleashed &\n"
        "the official Flipper\n"
        "Zero firmware.\n"
        "\n"
        "This device runs a\n"
        "hardened personal fork:\n"
        "security, crash &\n"
        "memory-safety fixes plus\n"
        "extra tools.\n"
        "\n"
        "Thanks to every Flipper\n"
        "open-source contributor.");

    view_dispatcher_switch_to_view(app->view_dispatcher, MomentumAppViewWidget);
}

bool momentum_app_scene_misc_credits_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void momentum_app_scene_misc_credits_on_exit(void* context) {
    MomentumApp* app = context;
    widget_reset(app->widget);
}
