
#include "pch.h"
#include "RevModel.h"
#include "WA_Reverb.h"
#include "Globals.h"
#include "resource.h"

// Export Function Name
#define EXPORTS extern "C" _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);

bool WA_Reverb_New(WA_Effect* This);
void WA_Reverb_Delete(WA_Effect* This);
uint32_t WA_Reverb_UpdateFormat(WA_Effect* This, const WA_AudioFormat* pAudioFormat);
uint32_t WA_Reverb_Process(WA_Effect* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes); 
void WA_Reverb_ConfigDialog(WA_Effect* This, HWND hParent);



static WA_Effect WA_ReverbVtbl = {
	{ // Start Common Header ---------------> 
	WA_PLUGINTYPE_DSP,				// Plugin Type
	1U,								// Version
	L"WinAudio Reverb\0",			// Description
	NULL,							// WinAudio HWND
	false							// Enabled or Disabled
	}, // End Common Header <-----------------
	WA_Reverb_New,
	WA_Reverb_Delete,
	WA_Reverb_UpdateFormat,
	WA_Reverb_Process,
	WA_Reverb_ConfigDialog,
	NULL
};


// thanks to: https://stackoverflow.com/questions/32228681/win32-dialog-inside-dll
static HMODULE GetCurrentModuleHandle() {
	HMODULE hModule = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)GetCurrentModuleHandle,
		&hModule);

	return hModule;
}



EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void)
{
	return (WA_HMODULE*)&WA_ReverbVtbl;
}


bool WA_Reverb_New(WA_Effect* This)
{
	WA_Reverb* pInstance = new WA_Reverb();

	This->hPluginData = static_cast<HCOOKIE>(pInstance);

	return true;
}

void WA_Reverb_Delete(WA_Effect* This)
{
	WA_Reverb* pInstance = static_cast<WA_Reverb*>(This->hPluginData);

	delete pInstance;
}

uint32_t WA_Reverb_UpdateFormat(WA_Effect* This, const WA_AudioFormat* pAudioFormat)
{
	WA_Reverb* pInstance = static_cast<WA_Reverb*>(This->hPluginData);

	pInstance->UpdateFormat(*pAudioFormat);

	return WA_OK;
}

uint32_t WA_Reverb_Process(WA_Effect* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes)
{
	
	WA_Reverb* pInstance = static_cast<WA_Reverb*>(This->hPluginData);

	pInstance->Process(pBuffer, uBufferLen);
	(*puProcessedBytes) = uBufferLen;

	return WA_OK;
	
}



void WA_Reverb_ConfigDialog(WA_Effect* This, HWND hParent)
{
	WA_Reverb* pInstance = static_cast<WA_Reverb*>(This->hPluginData);

	DialogBoxParam(GetCurrentModuleHandle(), MAKEINTRESOURCE(IDD_REVERB_DIALOG), hParent, ConfigDialogProc, (LPARAM) pInstance);
}