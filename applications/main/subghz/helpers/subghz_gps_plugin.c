#include "subghz_gps.h"

#include <expansion/expansion.h>
#include <lib/toolbox/load_plugin.h>

#define TAG "SubGhzGPS"

SubGhzGPS* subghz_gps_plugin_init(uint32_t baudrate) {
    Expansion* expansion = furi_record_open(RECORD_EXPANSION);
    bool connected = expansion_is_connected(expansion);
    if(connected) {
        furi_record_close(RECORD_EXPANSION);
        return NULL;
    }

    expansion_disable(expansion);

    void (*subghz_gps_init)(SubGhzGPS* subghz_gps, uint32_t baudrate) = NULL;
    PluginManager* manager = NULL;
    if(load_plugin("subghz_gps", 1, "subghz_gps", &subghz_gps_init, &manager)) {
        SubGhzGPS* subghz_gps = malloc(sizeof(SubGhzGPS));
        subghz_gps->plugin_manager = manager;
        subghz_gps_init(subghz_gps, baudrate);

        furi_record_close(RECORD_EXPANSION);
        return subghz_gps;
    }

    expansion_enable(expansion);
    furi_record_close(RECORD_EXPANSION);
    return NULL;
}

void subghz_gps_plugin_deinit(SubGhzGPS* subghz_gps) {
    subghz_gps->deinit(subghz_gps);
    plugin_manager_free(subghz_gps->plugin_manager);
    free(subghz_gps);
    furi_record_close(RECORD_STORAGE);

    expansion_enable(furi_record_open(RECORD_EXPANSION));
    furi_record_close(RECORD_EXPANSION);
}
