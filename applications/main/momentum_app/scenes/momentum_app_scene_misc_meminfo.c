#include "../momentum_app.h"
#include <furi_hal_power.h>

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

    // Uptime since boot
    uint32_t tick_freq = furi_kernel_get_tick_frequency();
    uint32_t uptime_s = (tick_freq > 0) ? (furi_get_tick() / tick_freq) : 0;
    uint32_t up_h = uptime_s / 3600;
    uint32_t up_m = (uptime_s % 3600) / 60;
    uint32_t up_s = uptime_s % 60;

    // Battery / power — read-only fuel-gauge data (floats rendered as ints:
    // the firmware's printf has no %f). Same I2C reads the power service does.
    uint8_t batt_pct = furi_hal_power_get_pct();
    uint32_t batt_mv =
        (uint32_t)(furi_hal_power_get_battery_voltage(FuriHalPowerICFuelGauge) * 1000.0f);
    int32_t batt_temp = (int32_t)furi_hal_power_get_battery_temperature(FuriHalPowerICFuelGauge);
    bool charging = furi_hal_power_is_charging();

    FuriString* str = furi_string_alloc();
    furi_string_printf(
        str,
        "-- Heap --\n"
        "Free:  %lu B\n"
        "Total: %lu B\n"
        "Min:   %lu B\n"
        "Block: %lu B\n"
        "Used:  %lu%%\n"
        "-- System --\n"
        "Uptime: %luh %02lum %02lus\n"
        "Batt:  %u%% %s\n"
        "Volt:  %lu mV\n"
        "Temp:  %ld C",
        (uint32_t)free_heap,
        (uint32_t)total_heap,
        (uint32_t)min_free,
        (uint32_t)max_block,
        used_pct,
        up_h,
        up_m,
        up_s,
        batt_pct,
        charging ? "(chg)" : "",
        batt_mv,
        batt_temp);

    widget_add_string_element(widget, 64, 2, AlignCenter, AlignTop, FontPrimary, "System Info");
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
