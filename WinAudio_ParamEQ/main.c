
#include "pch.h"
#include "WA_Biquad.h"
#include "resource.h"
#include "Globals.h"



// Export Function Name
#define EXPORTS _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);

bool WA_ParamEQ_New (WA_Effect* This);		
void WA_ParamEQ_Delete(WA_Effect* This);   
uint32_t WA_ParamEQ_UpdateFormat(WA_Effect* This, const WA_AudioFormat* pAudioFormat);
uint32_t WA_ParamEQ_Process(WA_Effect* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes); // Process Audio Samples. Return WA_OK or Error Code
void WA_ParamEQ_ConfigDialog (WA_Effect* This, HWND hParent); 

static WA_Effect WA_Param_EQ = {
	{ // Start Common Header ---------------> 
	WA_PLUGINTYPE_DSP,				// Plugin Type
	1U,								// Version
	L"WinAudio ParamEQ Effect\0",	// Description
	NULL,							// WinAudio HWND
	false							// Enabled or Disabled
	}, // End Common Header <-----------------
	WA_ParamEQ_New,
	WA_ParamEQ_Delete,
	WA_ParamEQ_UpdateFormat,
	WA_ParamEQ_Process,
	WA_ParamEQ_ConfigDialog,
	NULL
};

typedef struct TagWA_ParamEQ_Intance
{
	WA_AudioFormat Format;
	WA_Biquad* BiquadArray[WA_BIQUAD_ARRAY];

	float Gain[WA_BIQUAD_ARRAY];
	float Q[WA_BIQUAD_ARRAY];
	bool bEnableEq;

} WA_ParamEQ_Intance;

// thanks to: https://stackoverflow.com/questions/32228681/win32-dialog-inside-dll
static HMODULE GetCurrentModuleHandle() {
	HMODULE hModule = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)GetCurrentModuleHandle,
		&hModule);

	return hModule;
}

static void WA_ParamEQ_Bytes_To_Float(WA_Effect* This, const int8_t* pByte, uint32_t uByteLen, float* pFloat, uint32_t uFloatLen)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uSampleSize, uTotalSamples, uFloatSample;

	uSampleSize = pInstance->Format.uBitsPerSample / 8U;
	uTotalSamples = uByteLen / uSampleSize;
	uFloatSample = 0U;

	for (uint32_t uSample = 0U; uSample < uByteLen; uSample += uSampleSize)
	{

		switch (pInstance->Format.uBitsPerSample)
		{
		case 8:
			pFloat[uFloatSample] = pByte[uSample] / 127.0f;
			break;
		case 16:
			pFloat[uFloatSample] = (pByte[uSample] + (pByte[uSample + 1] << 8)) / 32767.0f;
			break;
		case 24:
			pFloat[uFloatSample] = (pByte[uSample] + (pByte[uSample + 1] << 8) +
				(pByte[uSample + 2] << 16)) / 8388607.0f;
			break;
		case 32:
			pFloat[uFloatSample] = (pByte[uSample] + (pByte[uSample + 1] << 8) +
				(pByte[uSample + 2] << 16) + (pByte[uSample + 3] << 24)) / 2147483647.0f;
		}

		uFloatSample++;
		_ASSERT(uFloatSample <= uFloatLen);
	}
}

static inline float WA_ParamEQ_NormFloat(float fValue)
{

	if (fValue > 1.0f)
		return 1.0f;
	else if (fValue < -1.0f)
		return -1.0f;
	else
		return fValue;
}

static void WA_ParamEQ_Float_To_Bytes(WA_Effect* This, const float* pFloat, uint32_t uFloatLen, int8_t* pByte, uint32_t uByteLen)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uSampleSize, uSample;
	float fNormSample;
	
	uSampleSize = pInstance->Format.uBitsPerSample / 8U;
	uSample = 0U;

	for (uint32_t uFloatSample = 0U; uFloatSample < uFloatLen; uFloatSample++)
	{

		fNormSample = WA_ParamEQ_NormFloat(pFloat[uFloatSample]);

		switch (pInstance->Format.uBitsPerSample)
		{
		case 8:			
			pByte[uSample] = (int8_t)(fNormSample * 127.0f);
			break;
		case 16:		
			pByte[uSample] = (int8_t)((int16_t)(fNormSample * 32767.0f) & 0xFF);
			pByte[uSample + 1] =  (int8_t)(((int16_t)(fNormSample * 32767.0) & 0xFF00) >> 8);
			break;
		case 24:
			pByte[uSample] = (int8_t)((int32_t)(fNormSample * 8388607.0f) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(fNormSample * 8388607.0f) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(fNormSample * 8388607.0f) & 0xFF0000) >> 16);
			break;
		case 32:
			pByte[uSample] = (int8_t)((int32_t)(fNormSample * 2147483647.0f) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(fNormSample * 2147483647.0f) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(fNormSample * 2147483647.0f) & 0xFF0000) >> 16);
			pByte[uSample + 3] = (int8_t)(((int32_t)(fNormSample * 2147483647.0f) & 0xFF0000) >> 24);
			break;
		}

		uSample += uSampleSize;
		_ASSERT(uSample <= uByteLen);
	}
}


static float* WA_ParamEQ_AllocFloat(WA_Effect* This, uint32_t uByteLen, uint32_t* puFloatLen)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uSampleSize, uTotalSamples;
	float* pBuffer;

	if (uByteLen == 0)
		return NULL;

	uSampleSize = pInstance->Format.uBitsPerSample / 8U;
	uTotalSamples = uByteLen / uSampleSize;

	pBuffer = (float*)calloc(uTotalSamples, sizeof(float));
	(*puFloatLen) = uTotalSamples;

	return pBuffer;

}

static void WA_ParamEQ_FreeFloat(WA_Effect* This, float* pFloat)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;

	if (pFloat)
		free(pFloat);

	pFloat = NULL;
}

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void)
{
	return (WA_HMODULE*)&WA_Param_EQ;
}

bool WA_ParamEQ_New(WA_Effect* This)
{
	WA_ParamEQ_Intance* pInstance;

	pInstance = (WA_ParamEQ_Intance*)malloc(sizeof(WA_ParamEQ_Intance));

	if (!pInstance)
		return false;	

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{
		pInstance->BiquadArray[i] = WA_Biquad_New();
		pInstance->Gain[i] = 0.0f;
		pInstance->Q[i] = 1.0f;

		UI.Gain[i] = 0.0f;
		UI.Q[i] = 1.0f;
		UI.bEnableEq = true;
	}

	This->hPluginData = (HCOOKIE)pInstance;

	return true;
}

void WA_ParamEQ_Delete(WA_Effect* This)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*) This->hPluginData;

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{
		WA_Biquad_Delete(pInstance->BiquadArray[i]);
		pInstance->BiquadArray[i] = NULL;
	}

	if (pInstance)
		free(pInstance);

	pInstance = NULL;

}

uint32_t WA_ParamEQ_UpdateFormat(WA_Effect* This, const WA_AudioFormat* pAudioFormat)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;

	memcpy(&pInstance->Format, pAudioFormat, sizeof(WA_AudioFormat));

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{
		WA_Biquad_Update(pInstance->BiquadArray[i], PEAK, WA_EQ_Freq_Array[i], pInstance->Q[i], pInstance->Gain[i], pAudioFormat->uSamplerate);
	}
		

	return WA_OK;
}

uint32_t WA_ParamEQ_Process(WA_Effect* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uFloatLen;
	float* pFloat;

	// Update Biquad on Differences
	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{

		if ((pInstance->Gain[i] != UI.Gain[i]) ||
			(pInstance->Q[i] != UI.Q[i]))
		{
			pInstance->Gain[i] = UI.Gain[i];
			pInstance->Q[i] = UI.Q[i];

			WA_Biquad_Update(pInstance->BiquadArray[i], PEAK, WA_EQ_Freq_Array[i], pInstance->Q[i], pInstance->Gain[i], pInstance->Format.uSamplerate);

		}

	}	

	pFloat = WA_ParamEQ_AllocFloat(This, uBufferLen, &uFloatLen);

	if (!pFloat)
		return WA_ERROR_MALLOCERROR;

	
	WA_ParamEQ_Bytes_To_Float(This, pBuffer, uBufferLen, pFloat, uFloatLen);

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{
		WA_Biquad_Process(pInstance->BiquadArray[i], pFloat, uFloatLen, pInstance->Format.uChannels);
	}
	

	WA_ParamEQ_Float_To_Bytes(This, pFloat, uFloatLen, pBuffer, uBufferLen);

	WA_ParamEQ_FreeFloat(This, pFloat);
		

	(*puProcessedBytes) = uBufferLen;

	return WA_OK;

}



void WA_ParamEQ_ConfigDialog(WA_Effect* This, HWND hParent)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	HWND hDialog;

	DialogBox(GetCurrentModuleHandle(), MAKEINTRESOURCE(IDD_EQ_DIALOG), hParent, DialogEQProc);
	//ShowWindow(hDialog, SW_SHOW);
}