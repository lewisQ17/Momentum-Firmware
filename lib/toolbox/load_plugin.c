#include "load_plugin.h"

bool load_plugin(
    const char* appid,
    uint32_t version,
    const char* name,
    void* entry_point,
    PluginManager** plugin_manager) {
    *plugin_manager = plugin_manager_alloc(appid, version, NULL);
    char path[64];
    snprintf(path, sizeof(path), EXT_PATH("apps_data/%s/plugins/%s.fal"), appid, name);
    PluginManagerError error = plugin_manager_load_single(*plugin_manager, path);
    if(error == PluginManagerErrorNone) {
        *(const void**)entry_point = plugin_manager_get_ep(*plugin_manager, 0);
        return true;
    }
    plugin_manager_free(*plugin_manager);
    return false;
}
