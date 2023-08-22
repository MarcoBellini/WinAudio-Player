#include "stdafx.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_PluginLoader.h"


typedef WA_HMODULE* (*WA_Plugin_GetHeader)(void);

static bool WA_Input_Create_Search_Path(wchar_t *pSearchPath, wchar_t* pPluginsPath)
{
	DWORD nIndex = 0U;
	DWORD i = 0U;
	wchar_t SearchStr[] = TEXT("\\Plugins\0");
	wchar_t ExtStr[] = TEXT("\\*.dll\0");
	errno_t err;

	if (!GetModuleFileName(NULL, pSearchPath, MAX_PATH))
		return false;	

	while (pSearchPath[i] != L'\0')
	{
		if (pSearchPath[i] == L'\\')
		{
			nIndex = i;
		}

		i++;
	}

	pSearchPath[nIndex] = '\0';

	err = wcscpy_s(pPluginsPath, MAX_PATH, pSearchPath);
	if (err) return false;

	err = wcscat_s(pPluginsPath, MAX_PATH, SearchStr);
	if (err) return false;

	err = wcscpy_s(pSearchPath, MAX_PATH, pPluginsPath);
	if (err) return false;

	err = wcscat_s(pSearchPath, MAX_PATH, ExtStr);
	if (err) return false;

	return true;
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
	wchar_t lpwPluginsPath[MAX_PATH];
	wchar_t lpwSearchPath[MAX_PATH];
	DWORD dwDLLCount, dwInputCount, dwOutputCount;
	HANDLE hFindHandle;
	WIN32_FIND_DATA FindData;
	errno_t err;

	/*
	Secure loading of libraries to prevent DLL preloading attacks
	see: https://support.microsoft.com/en-gb/topic/secure-loading-of-libraries-to-prevent-dll-preloading-attacks-d41303ec-0748-9211-f317-2edc819682e1
	*/
	SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);

	ZeroMemory(lpwPluginsPath, sizeof(lpwPluginsPath));
	ZeroMemory(lpwSearchPath, sizeof(lpwSearchPath));

	if (!WA_Input_Create_Search_Path(lpwSearchPath, lpwPluginsPath))
		return WA_ERROR_FAIL;


	// Count DLL into plugin folder
	hFindHandle = FindFirstFile(lpwSearchPath, &FindData);
	dwDLLCount = 0U;

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


	hFindHandle = FindFirstFile(lpwSearchPath, &FindData);
	Plugins.uPluginsCount = 0U;

	dwInputCount = 0U;
	dwOutputCount = 0U;	


	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			HMODULE hModule;
			WA_PluginHeader* pHeader;
			WA_Plugin_GetHeader pGetHeaderProc;
			WA_HMODULE hVTable;
			wchar_t pLoadPath[MAX_PATH];

			ZeroMemory(pLoadPath, sizeof(pLoadPath));

			err = wcscat_s(pLoadPath, MAX_PATH, lpwPluginsPath);
			if (err) return WA_ERROR_FAIL;

			err = wcscat_s(pLoadPath, MAX_PATH, L"\\");
			if (err) return WA_ERROR_FAIL;

			err = wcscat_s(pLoadPath, MAX_PATH, FindData.cFileName);
			if (err) return WA_ERROR_FAIL;

		
			hModule = LoadLibrary(pLoadPath);
			
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
	{		
		free(Plugins.pPluginList);
		return WA_ERROR_INPUTOUTPUTNOTFOUND;
	}
		


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