
#ifndef WA_GEN_PLUGINLOADER_H
#define WA_GEN_PLUGINLOADER_H

// This define how many Plugin the application can Load
#define WA_GEN_PLUGINLOADER_MAX_PLUGINS 20


struct TagWA_PluginLoader
{
	WA_Plugin* pPluginList;
	uint32_t uPluginsCount;
} Plugins;


uint32_t WA_GEN_PluginLoader_Load(HWND hMainWindow);
uint32_t WA_GEN_PluginLoader_Unload(void);
uint32_t WA_GEN_PluginLoader_Call_New(void);
uint32_t WA_GEN_PluginLoader_Call_Delete(void);

#endif
