#pragma once

#include <gui/gui.h>
#include <storage/storage.h>
#include <desktop/desktop.h>
#include <dialogs/dialogs.h>
#include <expansion/expansion.h>
#include <notification/notification_app.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <power/power_service/power.h>

#include <gui/modules/variable_item_list.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/number_input.h>
#include <gui/modules/popup.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/widget.h>

#include <momentum/asset_packs.h>
#include <loader/loader_menu.h>
#include <lib/subghz/subghz_setting.h>
#include <rgb_backlight.h>
#include <momentum/namespoof.h>
#include <dolphin/dolphin.h>
#include <dolphin/dolphin_i.h>
#include <dolphin/helpers/dolphin_state.h>
#include <momentum/settings.h>
#include <desktop/views/desktop_view_slideshow.h>

#include <applications.h>
#include <assets_icons.h>
#include <flipper_application/flipper_application.h>
#include <furi.h>
#include <gui/view.h>
#include <lib/flipper_format/flipper_format.h>
#include <lib/toolbox/value_index.h>
#include <m-array.h>
#include <toolbox/stream/file_stream.h>

#include "scenes/momentum_app_scene.h"

ARRAY_DEF(CharList, char*)

#define DOLPHIN_MAX_XP (DOLPHIN_LEVELS[DOLPHIN_LEVEL_COUNT - 1] + 1)

typedef struct {
    Gui* gui;
    Storage* storage;
    Desktop* desktop;
    Dolphin* dolphin;
    DialogsApp* dialogs;
    Expansion* expansion;
    NotificationApp* notification;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;

    VariableItemList* var_item_list;
    Submenu* submenu;
    TextInput* text_input;
    ByteInput* byte_input;
    NumberInput* number_input;
    Popup* popup;
    DialogEx* dialog_ex;
    Widget* widget;

    CharList_t asset_pack_names;
    uint8_t asset_pack_index;
    bool asset_pack_names_loaded;
    CharList_t mainmenu_app_labels;
    CharList_t mainmenu_app_exes;
    uint8_t mainmenu_app_index;
    bool mainmenu_apps_loaded;
    DesktopSettings desktop_settings;
    bool subghz_use_defaults;
    FrequencyList_t subghz_static_freqs;
    uint8_t subghz_static_index;
    FrequencyList_t subghz_hopper_freqs;
    uint8_t subghz_hopper_index;
    bool subghz_freqs_loaded;
    bool subghz_extend;
    bool subghz_bypass;
    RgbColor lcd_color;
    RgbColor vgm_color;
    char device_name[FURI_HAL_VERSION_ARRAY_NAME_LENGTH];
    uint8_t spoof_mac[6]; // BLE MAC spoof; all-zero = real
    uint8_t spoof_serial[8]; // serial/UID spoof; all-zero = real
    uint32_t dolphin_xp;
    uint32_t dolphin_angry;
    FuriString* version_tag;
    FuriTimer* meminfo_timer;

    bool save_mainmenu_apps;
    bool save_desktop;
    bool save_subghz_freqs;
    bool save_subghz;
    bool save_name;
    bool name_spoofed; // device_name is a user-set spoof, not the real OTP name
    bool save_xp;
    bool save_angry;
    bool save_dolphin;
    bool save_backlight;
    bool save_settings;
    bool apply_pack;
    bool show_slideshow;
    bool require_reboot;
    char pack_warn_text[64]; // persistent backing for the asset-pack warning popup
    FuriString* icon_file_path; // last path picked in the loose-icon preview browser
    const Icon* icon_preview; // heap-owned loose icon being previewed (NULL when none)
    char icon_info_text[32]; // persistent backing for the preview caption
} MomentumApp;

typedef enum {
    MomentumAppViewVarItemList,
    MomentumAppViewSubmenu,
    MomentumAppViewTextInput,
    MomentumAppViewByteInput,
    MomentumAppViewNumberInput,
    MomentumAppViewPopup,
    MomentumAppViewDialogEx,
    MomentumAppViewWidget,
} MomentumAppView;

bool momentum_app_apply(MomentumApp* app);

// True if any byte is non-zero (all-zero = the "real / not set" spoof sentinel).
bool momentum_app_bytes_any_set(const uint8_t* data, size_t size);

// Lazy lists: every load is a no-op once loaded, so each scene that reads a list
// calls its loader on enter. Everything is released in momentum_app_free().
void momentum_app_load_asset_pack_names(MomentumApp* app);
void momentum_app_unload_asset_pack_names(MomentumApp* app);

void momentum_app_push_mainmenu_app(MomentumApp* app, FuriString* exe);
void momentum_app_load_mainmenu_apps(MomentumApp* app);
void momentum_app_empty_mainmenu_apps(MomentumApp* app);
// Empties the list and clears the loaded flag, so the next load re-reads the file.
void momentum_app_unload_mainmenu_apps(MomentumApp* app);

void momentum_app_load_subghz_freqs(MomentumApp* app);
