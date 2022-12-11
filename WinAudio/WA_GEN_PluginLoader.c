#include "stdafx.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_PluginLoader.h"


typedef WA_HMODULE* (*WA_Plugin_GetHeader)(void);

static inline void WA_Input_Create_Search_Path(wchar_t *FullPath)
{
	size_t nIndex = 0;
	size_t i = 0;
	wchar_t SearchStr[] = TEXT("\\Plugins\0");
	wchar_t ExtStr[] = TEXT("\\*.dll\0");

	while (FullPath[i] != '\0')
	{
		if (FullPath[i] == '\\')
		{
			nIndex = i;
		}

		i++;
	}

	FullPath[nIndex] = '\0';

	wcscat_s(FullPath, MAX_PATH, SearchStr);

	// Add "Plugins" Directory to LoadLibrary Search Path
	// TODO: Refactor This Piece of Code
	SetDllDirectory(FullPath);

	wcscat_s(FullPath, MAX_PATH, ExtStr);
}






/*
1. Conta tutte le DLL nella directory PLUGIN
2. Verifica che siano meno di 20 in totale
3. Alloca tante istanze di WA_PLugin quante sono le DLL
4. Carica la DLL e recupera la funzione GetHeader
5. Se la funzione ha successo Riempi la struttura WA_Plugin e incrementa l'indice dei plugin validi
6. Per funzionare il player deve avere almeno 1 plugin di output e 1 di input altrimenti dai errore

*/
uint32_t WA_GEN_PluginLoader_Load(HWND hMainWindow)
{
	wchar_t lpwPluginPath[MAX_PATH];
	DWORD dwDLLCount, dwInputCount, dwOutputCount;
	HANDLE hFindHandle;
	WIN32_FIND_DATA FindData;


	// Move to Plugin Folder and Add a Search Pattern
	//dwPathLen = GetCurrentDirectory(MAX_PATH, lpwPluginPath);	
	//wcscat_s(lpwPluginPath, MAX_PATH, L"\\Plugins\\*.dll");

	GetModuleFileName(NULL, lpwPluginPath, MAX_PATH);
	WA_Input_Create_Search_Path(lpwPluginPath);


	// Count DLL into plugin folder
	hFindHandle = FindFirstFile(lpwPluginPath, &FindData);
	dwDLLCount = 0;

	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			dwDLLCount++;

		} while (FindNextFile(hFindHandle, &FindData) != 0);

		FindClose(hFindHandle);
	}


	// Check if Plugin count is less than WA_GEN_PLUGINLOADER_MAX_PLUGINS and > 0
	if (dwDLLCount > WA_GEN_PLUGINLOADER_MAX_PLUGINS)
		return WA_ERROR_TOOMUCHPLUGINS;

	if (dwDLLCount == 0)
		return WA_ERROR_INPUTOUTPUTNOTFOUND;

	Plugins.pPluginList = calloc(dwDLLCount, sizeof(WA_Plugin));

	if (!Plugins.pPluginList)
		return WA_ERROR_MALLOCERROR;


	hFindHandle = FindFirstFile(lpwPluginPath, &FindData);
	Plugins.uPluginsCount = 0;

	dwInputCount = 0;
	dwOutputCount = 0;


	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			HMODULE hModule;
			WA_PluginHeader* pHeader;
			WA_Plugin_GetHeader pGetHeaderProc;
			WA_HMODULE hVTable;
		
			hModule = LoadLibrary(FindData.cFileName);
			
			if (hModule)
			{
				pGetHeaderProc = (WA_Plugin_GetHeader) GetProcAddress(hModule, "WA_Plugin_GetHeader");
			
				if (pGetHeaderProc)
				{
					hVTable = pGetHeaderProc();

					if (hVTable)
					{
						pHeader = (WA_PluginHeader*) hVTable;
						pHeader->hMainWindow = hMainWindow;
						pHeader->bActive = false; // Disabled By Default. Enable when return true on New

						Plugins.pPluginList[Plugins.uPluginsCount].hModule = hModule;
						Plugins.pPluginList[Plugins.uPluginsCount].hVTable = hVTable;
						Plugins.pPluginList[Plugins.uPluginsCount].uPluginType = pHeader->uPluginType;
						

						Plugins.uPluginsCount++;

						switch (pHeader->uPluginType)
						{
						case WA_PLUGINTYPE_INPUT:
							dwInputCount++;
							break;
						case WA_PLUGINTYPE_OUTPUT:
							dwOutputCount++;
							break;
						}
					}
					else
					{
						FreeLibrary(hModule);
					}					
				}
				else
				{
					FreeLibrary(hModule);
				}			
			}

		} while (FindNextFile(hFindHandle, &FindData) != 0);

		FindClose(hFindHandle);
	}

	// Notify if no Input or Output Plugin are found
	if ((dwInputCount == 0) || (dwOutputCount == 0))
		return WA_ERROR_INPUTOUTPUTNOTFOUND;


	return WA_OK;	
}

uint32_t WA_GEN_PluginLoader_Unload(void)
{

	if (!Plugins.pPluginList)
		return WA_ERROR_MALLOCERROR;

	for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
	{
		if(Plugins.pPluginList[i].hModule)
			FreeLibrary(Plugins.pPluginList[i].hModule);

		Plugins.pPluginList[i].hModule = NULL;
		Plugins.pPluginList[i].hVTable = NULL;		
	}


	Plugins.uPluginsCount = 0;
	free(Plugins.pPluginList);
	Plugins.pPluginList = NULL;

	return WA_OK;
}

uint32_t WA_GEN_PluginLoader_Call_New(void)
{
	if (!Plugins.pPluginList)
		return WA_ERROR_MALLOCERROR;


	for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
	{
		switch (Plugins.pPluginList[i].uPluginType)
		{
		case WA_PLUGINTYPE_INPUT:
		{
			WA_Input *pIn = (WA_Input*) Plugins.pPluginList[i].hVTable;

			pIn->Header.bActive = pIn->WA_Input_New(pIn);
			break;
		}			
			
		case WA_PLUGINTYPE_OUTPUT:
		{
			WA_Output* pOut = (WA_Output*)Plugins.pPluginList[i].hVTable;

			pOut->Header.bActive = pOut->WA_Output_New(pOut);
			break;
		}
		case WA_PLUGINTYPE_DSP:
		{
			WA_Effect* pDsp = (WA_Effect*)Plugins.pPluginList[i].hVTable;

			pDsp->Header.bActive = pDsp->WA_Effect_New(pDsp);
			break;
		}
		}
	}

	return WA_OK;
}


uint32_t WA_GEN_PluginLoader_Call_Delete(void)
{
	if (!Plugins.pPluginList)
		return WA_ERROR_MALLOCERROR;


	for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
	{
		switch (Plugins.pPluginList[i].uPluginType)
		{
		case WA_PLUGINTYPE_INPUT:
		{
			WA_Input* pIn = (WA_Input*)Plugins.pPluginList[i].hVTable;

			
			if(pIn->Header.bActive)
				pIn->WA_Input_Delete(pIn);

	
			break;
		}

		case WA_PLUGINTYPE_OUTPUT:
		{
			WA_Output* pOut = (WA_Output*)Plugins.pPluginList[i].hVTable;

			if (pOut->Header.bActive)
				pOut->WA_Output_Delete(pOut);

			break;
		}
		case WA_PLUGINTYPE_DSP:
		{
			WA_Effect* pDsp = (WA_Effect*)Plugins.pPluginList[i].hVTable;

			if (pDsp->Header.bActive)
				pDsp->WA_Effect_Delete(pDsp);

			break;
		}
		}
	}

	return WA_OK;
}