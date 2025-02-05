#pragma once

#include <flipper_application/plugins/plugin_manager.h>

bool load_plugin(
    const char* appid,
    uint32_t version,
    const char* name,
    void* entry_point,
    PluginManager** plugin_manager);
