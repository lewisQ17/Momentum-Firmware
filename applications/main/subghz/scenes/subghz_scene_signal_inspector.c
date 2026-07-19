#include "../subghz_i.h"
#include "../helpers/subghz_custom_event.h"

#include <lib/subghz/blocks/generic.h>

#define TAG "SubGhzSceneSignalInspector"

// Read-only detail view for a captured Sub-GHz signal (issue #449).
// Shows the full metadata of the selected history record. Nothing is
// transmitted - all data comes from the history getters and from the
// decoder that has already parsed the captured signal.
void subghz_scene_signal_inspector_on_enter(void* context) {
    SubGhz* subghz = context;

    // The Widget is shared between scenes and still holds the Info screen
    // elements (Info is only suspended, not exited, when we open this scene).
    widget_reset(subghz->widget);

    FuriString* text = furi_string_alloc();

    // Reload the decoder for the currently selected record. This is the same
    // read-only helper the Info screen uses to render its widget.
    if(subghz_scene_receiver_info_update_parser(subghz)) {
        const char* protocol_name =
            subghz_history_get_protocol_name(subghz->history, subghz->idx_menu_chosen);
        uint32_t frequency =
            subghz_history_get_frequency(subghz->history, subghz->idx_menu_chosen);
        SubGhzRadioPreset* preset =
            subghz_history_get_radio_preset(subghz->history, subghz->idx_menu_chosen);
        uint16_t repeats = subghz_history_get_repeats(subghz->history, subghz->idx_menu_chosen);
        uint32_t hash = subghz_history_get_hash_data(subghz->history, subghz->idx_menu_chosen);
        DateTime datetime = subghz_history_get_datetime(subghz->history, subghz->idx_menu_chosen);

        // Header block: capture metadata that is not part of the decoded string.
        furi_string_printf(
            text,
            "\e#Signal Details\n"
            "Protocol: %s\n"
            "Frequency: %lu Hz\n"
            "Preset: %s\n"
            "Repeats: %d\n"
            "Hash: 0x%08lX\n"
            "Time: %04d-%02d-%02d\n"
            "      %02d:%02d:%02d\n"
            "\e#Decoded\n",
            protocol_name,
            frequency,
            furi_string_get_cstr(preset->name),
            repeats,
            hash,
            datetime.year,
            datetime.month,
            datetime.day,
            datetime.hour,
            datetime.minute,
            datetime.second);

        // Protocol-formatted payload (bit length, Key/data, serial, button,
        // counter, seed, ... exactly as each protocol exposes them).
        subghz_protocol_decoder_base_get_string(subghz_txrx_get_decoder(subghz->txrx), text);
    } else {
        furi_string_set(text, "Error history parse.");
    }

    widget_add_text_scroll_element(subghz->widget, 0, 0, 128, 64, furi_string_get_cstr(text));

    furi_string_free(text);

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdWidget);
}

bool subghz_scene_signal_inspector_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    // Read-only view: the Back key is handled by the global navigation
    // callback, which pops this scene and returns to the Info screen.
    return false;
}

void subghz_scene_signal_inspector_on_exit(void* context) {
    SubGhz* subghz = context;
    widget_reset(subghz->widget);
}
