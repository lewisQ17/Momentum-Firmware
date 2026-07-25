#include "../momentum_app.h"

void momentum_app_scene_interface_graphics_icon_on_enter(void* context) {
    MomentumApp* app = context;
    const Icon* icon = app->icon_preview;

    // The caller only pushes this scene on a successful load, but guard anyway.
    if(!icon) {
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    uint16_t w = icon_get_width(icon);
    uint16_t h = icon_get_height(icon);
    uint32_t frames = icon_get_frame_count(icon);

    // Centre the icon in the area above the caption line (validated w<=128, h<=64).
    uint8_t x = (w < 128) ? (uint8_t)((128 - w) / 2) : 0;
    uint8_t y = (h < 54) ? (uint8_t)((54 - h) / 2) : 0;
    widget_add_icon_element(app->widget, x, y, icon);

    snprintf(
        app->icon_info_text,
        sizeof(app->icon_info_text),
        "%ux%u  %lu frame%s",
        (unsigned)w,
        (unsigned)h,
        (unsigned long)frames,
        frames == 1 ? "" : "s");
    widget_add_string_element(
        app->widget, 64, 63, AlignCenter, AlignBottom, FontSecondary, app->icon_info_text);

    view_dispatcher_switch_to_view(app->view_dispatcher, MomentumAppViewWidget);
}

bool momentum_app_scene_interface_graphics_icon_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false; // Let Back pop the scene normally.
}

void momentum_app_scene_interface_graphics_icon_on_exit(void* context) {
    MomentumApp* app = context;
    // Reset the widget FIRST so it stops referencing the icon, THEN free it.
    widget_reset(app->widget);
    asset_packs_icon_free(app->icon_preview);
    app->icon_preview = NULL;
}
