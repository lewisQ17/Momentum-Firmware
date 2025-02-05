#include "desktop_settings.h"
#include "desktop_settings_filename.h"

#include <saved_struct.h>
#include <storage/storage.h>
#include <lib/toolbox/load_plugin.h>

#define TAG "DesktopSettings"

#define DESKTOP_SETTINGS_VER   (12)
#define DESKTOP_SETTINGS_MAGIC (0x13) // Different from OFW 0x17

void desktop_settings_load(DesktopSettings* settings) {
    furi_assert(settings);

    bool success = saved_struct_load(
        DESKTOP_SETTINGS_PATH,
        settings,
        sizeof(DesktopSettings),
        DESKTOP_SETTINGS_MAGIC,
        DESKTOP_SETTINGS_VER);

    if(!success) {
        if(storage_file_exists(furi_record_open(RECORD_STORAGE), DESKTOP_SETTINGS_PATH)) {
            bool (*desktop_settings_migrate)(DesktopSettings* settings, const char* path) = NULL;
            PluginManager* manager = NULL;
            if(load_plugin(
                   "desktop", 1, "desktop_settings_migrate", &desktop_settings_migrate, &manager)) {
                success = desktop_settings_migrate(settings, DESKTOP_SETTINGS_PATH);
                plugin_manager_free(manager);
            }
        }
        furi_record_close(RECORD_STORAGE);

        if(!success) {
            FURI_LOG_W(TAG, "Failed to load file, using defaults");
            memset(settings, 0, sizeof(DesktopSettings));
            // desktop_settings_save(settings);
        }
    }
}

void desktop_settings_save(const DesktopSettings* settings) {
    furi_assert(settings);

    const bool success = saved_struct_save(
        DESKTOP_SETTINGS_PATH,
        settings,
        sizeof(DesktopSettings),
        DESKTOP_SETTINGS_MAGIC,
        DESKTOP_SETTINGS_VER);

    if(!success) {
        FURI_LOG_E(TAG, "Failed to save file");
    }
}
