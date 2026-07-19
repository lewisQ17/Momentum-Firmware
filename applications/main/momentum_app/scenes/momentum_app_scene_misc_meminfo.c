#include "../momentum_app.h"

#define MEMINFO_REFRESH_EVENT   (0)
#define MEMINFO_REFRESH_PERIOD  (1000)

static void momentum_app_scene_misc_meminfo_update(MomentumApp* app) {
    Widget* widget = app->widget;
    widget_reset(widget);

    size_t free_heap = memmgr_get_free_heap();
    size_t total_heap = memmgr_get_total_heap();
    size_t min_free = memmgr_get_minimum_free_heap();
    size_t max_block = memmgr_heap_get_max_free_block();
    size_t used = (total_heap > free_heap) ? (total_heap - free_heap) : 0;
    uint32_t used_pct = (total_heap > 0) ? (uint32_t)((used * 100u) / total_heap) : 0;

    FuriString* str = furi_string_alloc();
    furi_string_printf(
        str,
        "Free:  %lu B\n"
        "Total: %lu B\n"
        "Min:   %lu B\n"
        "Block: %lu B\n"
        "Used:  %lu%%",
        (uint32_t)free_heap,
        (uint32_t)total_heap,
        (uint32_t)min_free,
        (uint32_t)max_block,
        used_pct);

    widget_add_string_element(
        widget, 64, 2, AlignCenter, AlignTop, FontPrimary, "Memory Info");
    widget_add_text_scroll_element(widget, 0, 15, 128, 49, furi_string_get_cstr(str));

    furi_string_free(str);
}

static void momentum_app_scene_misc_meminfo_timer_callback(void* context) {
    MomentumApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, MEMINFO_REFRESH_EVENT);
}

void momentum_app_scene_misc_meminfo_on_enter(void* context) {
    MomentumApp* app = context;

    momentum_app_scene_misc_meminfo_update(app);

    app->meminfo_timer = furi_timer_alloc(
        momentum_app_scene_misc_meminfo_timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->meminfo_timer, MEMINFO_REFRESH_PERIOD);

    view_dispatcher_switch_to_view(app->view_dispatcher, MomentumAppViewWidget);
}

bool momentum_app_scene_misc_meminfo_on_event(void* context, SceneManagerEvent event) {
    MomentumApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == MEMINFO_REFRESH_EVENT) {
            momentum_app_scene_misc_meminfo_update(app);
            consumed = true;
        }
    }

    return consumed;
}

void momentum_app_scene_misc_meminfo_on_exit(void* context) {
    MomentumApp* app = context;

    furi_timer_stop(app->meminfo_timer);
    furi_timer_free(app->meminfo_timer);
    app->meminfo_timer = NULL;

    widget_reset(app->widget);
}
