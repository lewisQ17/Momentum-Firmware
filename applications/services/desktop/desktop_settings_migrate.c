#include "desktop_settings.h"

#include <stdlib.h>
#include <toolbox/saved_struct.h>
#include <string.h>

#define DESKTOP_SETTINGS_MAGIC_MNTM (0x13)
typedef struct {
    uint32_t auto_lock_delay_ms;
    uint8_t auto_lock_with_pin;
    uint8_t display_clock;
} DesktopSettingsMntmV11;

#define DESKTOP_SETTINGS_MAGIC_OFW_UL (0x17)
typedef struct {
    char name_or_path[128];
} FavoriteApp;
typedef struct {
    uint32_t auto_lock_delay_ms;
    uint8_t dummy_mode;
    uint8_t display_clock;
    FavoriteApp favorite_apps[4];
    FavoriteApp dummy_apps[4];
} DesktopSettingsOfwV11;
typedef struct {
    uint8_t reserved[11];
    DesktopSettingsOfwV11 settings;
} DesktopSettingsOfwV10;
typedef struct {
    uint32_t auto_lock_delay_ms;
    uint8_t usb_inhibit_auto_lock;
    uint8_t displayBatteryPercentage;
    uint8_t dummy_mode;
    uint8_t display_clock;
    FavoriteApp favorite_apps[4];
    FavoriteApp dummy_apps[9];
} DesktopSettingsUlV16;
typedef struct {
    uint32_t auto_lock_delay_ms;
    uint32_t auto_poweroff_delay_ms;
    uint8_t displayBatteryPercentage;
    uint8_t dummy_mode;
    uint8_t display_clock;
    FavoriteApp favorite_apps[4];
    FavoriteApp dummy_apps[9];
} DesktopSettingsUlV15;
typedef struct {
    uint32_t auto_lock_delay_ms;
    uint8_t displayBatteryPercentage;
    uint8_t dummy_mode;
    uint8_t display_clock;
    FavoriteApp favorite_apps[4];
    FavoriteApp dummy_apps[9];
} DesktopSettingsUlV14;
typedef struct {
    uint8_t reserved[11];
    uint32_t auto_lock_delay_ms;
    uint8_t displayBatteryPercentage;
    uint8_t dummy_mode;
    uint8_t display_clock;
    FavoriteApp favorite_apps[4];
    FavoriteApp dummy_apps[9];
} DesktopSettingsUlV13;

bool desktop_settings_migrate(DesktopSettings* settings, const char* path) {
    uint8_t magic, version;
    size_t size;
    if(!saved_struct_get_metadata(path, &magic, &version, &size)) {
        return false;
    }

    // Set defaults
    memset(settings, 0, sizeof(DesktopSettings));

    // Try to migrate from Momentum
    if(magic == DESKTOP_SETTINGS_MAGIC_MNTM) {
        if(version == 11 && size == sizeof(DesktopSettingsMntmV11)) {
            DesktopSettingsMntmV11 mntm_v11;
            bool success = saved_struct_load(path, &mntm_v11, sizeof(mntm_v11), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = mntm_v11.auto_lock_delay_ms;
                settings->auto_lock_with_pin = mntm_v11.auto_lock_with_pin;
                settings->display_clock = mntm_v11.display_clock;
            }
            return success;
        }
    }

    if(magic == DESKTOP_SETTINGS_MAGIC_OFW_UL) {
        // Try to migrate from OFW
        if(version == 11 && size == sizeof(DesktopSettingsOfwV11)) {
            DesktopSettingsOfwV11* ofw_v11 = malloc(sizeof(DesktopSettingsOfwV11));
            bool success = saved_struct_load(path, ofw_v11, sizeof(*ofw_v11), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ofw_v11->auto_lock_delay_ms;
                settings->display_clock = ofw_v11->display_clock;
            }
            free(ofw_v11);
            return success;
        }
        if(version == 10 && size == sizeof(DesktopSettingsOfwV10)) {
            DesktopSettingsOfwV10* ofw_v10 = malloc(sizeof(DesktopSettingsOfwV10));
            bool success = saved_struct_load(path, ofw_v10, sizeof(*ofw_v10), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ofw_v10->settings.auto_lock_delay_ms;
                settings->display_clock = ofw_v10->settings.display_clock;
            }
            free(ofw_v10);
            return success;
        }

        // Try to migrate from Unleashed
        if(version == 16 && size == sizeof(DesktopSettingsUlV16)) {
            DesktopSettingsUlV16* ul_v16 = malloc(sizeof(DesktopSettingsUlV16));
            bool success = saved_struct_load(path, ul_v16, sizeof(*ul_v16), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ul_v16->auto_lock_delay_ms;
                settings->usb_inhibit_auto_lock = ul_v16->usb_inhibit_auto_lock;
                settings->display_clock = ul_v16->display_clock;
            }
            free(ul_v16);
            return success;
        }
        if(version == 15 && size == sizeof(DesktopSettingsUlV15)) {
            DesktopSettingsUlV15* ul_v15 = malloc(sizeof(DesktopSettingsUlV15));
            bool success = saved_struct_load(path, ul_v15, sizeof(*ul_v15), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ul_v15->auto_lock_delay_ms;
                settings->display_clock = ul_v15->display_clock;
            }
            free(ul_v15);
            return success;
        }
        if(version == 14 && size == sizeof(DesktopSettingsUlV14)) {
            DesktopSettingsUlV14* ul_v14 = malloc(sizeof(DesktopSettingsUlV14));
            bool success = saved_struct_load(path, ul_v14, sizeof(*ul_v14), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ul_v14->auto_lock_delay_ms;
                settings->display_clock = ul_v14->display_clock;
            }
            free(ul_v14);
            return success;
        }
        if(version == 13 && size == sizeof(DesktopSettingsUlV13)) {
            DesktopSettingsUlV13* ul_v13 = malloc(sizeof(DesktopSettingsUlV13));
            bool success = saved_struct_load(path, ul_v13, sizeof(*ul_v13), magic, version);
            if(success) {
                settings->auto_lock_delay_ms = ul_v13->auto_lock_delay_ms;
                settings->display_clock = ul_v13->display_clock;
            }
            free(ul_v13);
            return success;
        }
    }
    return false;
}

#include <flipper_application/flipper_application.h>

static const FlipperAppPluginDescriptor plugin_descriptor = {
    .appid = "desktop",
    .ep_api_version = 1,
    .entry_point = &desktop_settings_migrate,
};

const FlipperAppPluginDescriptor* desktop_settings_migrate_ep(void) {
    return &plugin_descriptor;
}
