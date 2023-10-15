
#include "pch.h"
#include "WA_Biquad.h"
#include "WA_VolumeBoost.h"
#include "resource.h"
#include "Globals.h"
#include "..\WInAudio_SharedFunc\WA_GEN_INI.h"




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
	WA_Boost* pBoost;

	double Gain[WA_BIQUAD_ARRAY];
	double Q[WA_BIQUAD_ARRAY];
	bool bEnableEq;
	bool bEnableBoost;
	double fVolumeMult;

} WA_ParamEQ_Intance;

// thanks to: https://stackoverflow.com/questions/32228681/win32-dialog-inside-dll
static HMODULE GetCurrentModuleHandle() {
	HMODULE hModule = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)GetCurrentModuleHandle,
		&hModule);

	return hModule;
}

static void WA_ParamEQ_Bytes_To_Float(WA_Effect* This, const int8_t* pByte, uint32_t uByteLen, double* pFloat, uint32_t uFloatLen)
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
			pFloat[uFloatSample] = pByte[uSample] / 127.0;
			break;
		case 16:
			pFloat[uFloatSample] = ((pByte[uSample] & 0xFF) | (pByte[uSample + 1] << 8)) / 32767.0;
			break;
		case 24:
			pFloat[uFloatSample] = ((pByte[uSample] & 0xFF) | ((pByte[uSample + 1] << 8) & 0xFF00) |
				(pByte[uSample + 2] << 16)) / 8388607.0;
			break;
		case 32:
			pFloat[uFloatSample] = ((pByte[uSample] & 0xFF) | ((pByte[uSample + 1] << 8) & 0xFF00) |
				((pByte[uSample + 2] << 16) & 0xFF0000) | (pByte[uSample + 3] << 24)) / 2147483647.0;
		}

		uFloatSample++;
		_ASSERT(uFloatSample <= uFloatLen);
	}
}

static inline double WA_ParamEQ_NormFloat(double fValue)
{

	if (fValue > 1.0)
		return 1.0;
	else if (fValue < -1.0)
		return -1.0;
	else
		return fValue;
}

static void WA_ParamEQ_Double_To_Bytes(WA_Effect* This, const double* pFloat, uint32_t uFloatLen, int8_t* pByte, uint32_t uByteLen)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uSampleSize, uSample;
	double fNormSample;
	
	uSampleSize = pInstance->Format.uBitsPerSample / 8U;
	uSample = 0U;

	for (uint32_t uFloatSample = 0U; uFloatSample < uFloatLen; uFloatSample++)
	{

		fNormSample = WA_ParamEQ_NormFloat(pFloat[uFloatSample]);

		switch (pInstance->Format.uBitsPerSample)
		{
		case 8:			
			pByte[uSample] = (int8_t)(fNormSample * 127.0);
			break;
		case 16:		
			pByte[uSample] = (int8_t)((int16_t)(fNormSample * 32767.0) & 0xFF);
			pByte[uSample + 1] =  (int8_t)(((int16_t)(fNormSample * 32767.0) & 0xFF00) >> 8);
			break;
		case 24:
			pByte[uSample] = (int8_t)((int32_t)(fNormSample * 8388607.0) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(fNormSample * 8388607.0) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(fNormSample * 8388607.0) & 0xFF0000) >> 16);
			break;
		case 32:
			pByte[uSample] = (int8_t)((int32_t)(fNormSample * 2147483647.0) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(fNormSample * 2147483647.0) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(fNormSample * 2147483647.0) & 0xFF0000) >> 16);
			pByte[uSample + 3] = (int8_t)(((int32_t)(fNormSample * 2147483647.0) & 0xFF000000) >> 24);
			break;
		}

		uSample += uSampleSize;
		_ASSERT(uSample <= uByteLen);
	}
}


static double* WA_ParamEQ_AllocDouble(WA_Effect* This, uint32_t uByteLen, uint32_t* puDoubleLen)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uSampleSize, uTotalSamples;
	double* pBuffer;

	if (uByteLen == 0)
		return NULL;

	uSampleSize = pInstance->Format.uBitsPerSample / 8U;
	uTotalSamples = uByteLen / uSampleSize;

	pBuffer = (double*)calloc(uTotalSamples, sizeof(double));
	(*puDoubleLen) = uTotalSamples;

	return pBuffer;

}

static void WA_ParamEQ_FreeDouble(WA_Effect* This, double* pDouble)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;

	if (pDouble)
		free(pDouble);

	pDouble = NULL;
}

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void)
{
	return (WA_HMODULE*)&WA_Param_EQ;
}

bool WA_ParamEQ_New(WA_Effect* This)
{
	WA_ParamEQ_Intance* pInstance;
	WA_Ini* pIni;
	wchar_t Buffer[32];

	pInstance = (WA_ParamEQ_Intance*)malloc(sizeof(WA_ParamEQ_Intance));

	if (!pInstance)
		return false;	

	pIni = WA_Ini_New();

	if (!pIni)
	{
		free(pInstance);
		return WA_ERROR_MALLOCERROR;
	}

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{
		ZeroMemory(Buffer, sizeof(Buffer));

		pInstance->BiquadArray[i] = WA_Biquad_New();

		swprintf_s(Buffer, 32, L"Gain_%d\0", i);
		pInstance->Gain[i] = WA_Ini_Read_Double(pIni, WA_EQ_STD_GAIN, WA_EQ_INI_SECTION, Buffer);

		swprintf_s(Buffer, 32, L"Q_%d\0", i);
		pInstance->Q[i] = WA_Ini_Read_Double(pIni, WA_EQ_STD_Q, WA_EQ_INI_SECTION, Buffer);


		UI.Gain[i] = pInstance->Gain[i];
		UI.Q[i] = pInstance->Q[i];
		
	}

	pInstance->bEnableEq = WA_Ini_Read_Bool(pIni, false, WA_EQ_INI_SECTION, L"EnableEq");
	UI.bEnableEq = pInstance->bEnableEq;

	pInstance->bEnableBoost = WA_Ini_Read_Bool(pIni, false, WA_EQ_INI_SECTION, L"EnableBoost");
	UI.bEnableBoost = pInstance->bEnableBoost;

	ZeroMemory(Buffer, sizeof(Buffer));
	pInstance->fVolumeMult = WA_Ini_Read_Double(pIni, 5.0, WA_EQ_INI_SECTION, L"VolumeMultiplier");
	UI.fVolumeMult = pInstance->fVolumeMult;

	pInstance->pBoost = WA_Volume_Boost_Init();

	This->hPluginData = (HCOOKIE)pInstance;

	WA_Ini_Delete(pIni);

	return true;
}

void WA_ParamEQ_Delete(WA_Effect* This)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*) This->hPluginData;
	wchar_t Buffer[32];
	WA_Ini* pIni;

	pIni = WA_Ini_New();

	for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
	{

		if (pIni)
		{
			ZeroMemory(Buffer, sizeof(Buffer));

			swprintf_s(Buffer, 32, L"Gain_%d\0", i);
			WA_Ini_Write_Double(pIni, pInstance->Gain[i], WA_EQ_INI_SECTION, Buffer);

			swprintf_s(Buffer, 32, L"Q_%d\0", i);
			WA_Ini_Write_Double(pIni, pInstance->Q[i], WA_EQ_INI_SECTION, Buffer);
		}	


		WA_Biquad_Delete(pInstance->BiquadArray[i]);
		pInstance->BiquadArray[i] = NULL;
	}

	WA_Volume_Boost_Delete(pInstance->pBoost);

	if (pIni)
	{
		WA_Ini_Write_Bool(pIni, pInstance->bEnableEq, WA_EQ_INI_SECTION, L"EnableEq");
		WA_Ini_Write_Bool(pIni, pInstance->bEnableBoost, WA_EQ_INI_SECTION, L"EnableBoost");
		WA_Ini_Write_Double(pIni, pInstance->fVolumeMult, WA_EQ_INI_SECTION, L"VolumeMultiplier");

		WA_Ini_Delete(pIni);
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

	WA_Volume_Boost_Update(pInstance->pBoost, pAudioFormat->uAvgBytesPerSec, pAudioFormat->uChannels, pInstance->fVolumeMult);
		
	
	return WA_OK;
}

uint32_t WA_ParamEQ_Process(WA_Effect* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;
	uint32_t uDoubleLen;
	double* pDouble;


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

	pInstance->bEnableEq = UI.bEnableEq;

	// Update Volume Factor
	if (pInstance->fVolumeMult != UI.fVolumeMult)
	{
		pInstance->fVolumeMult = UI.fVolumeMult;

		WA_Volume_Boost_Update(pInstance->pBoost, pInstance->Format.uAvgBytesPerSec, pInstance->Format.uChannels, pInstance->fVolumeMult);
	}

	pInstance->bEnableBoost = UI.bEnableBoost;


	// Skip 8 Bits Samples
	if (pInstance->Format.uBitsPerSample < 16)
	{
		(*puProcessedBytes) = uBufferLen;
		return WA_OK;
	}
		

	pDouble = WA_ParamEQ_AllocDouble(This, uBufferLen, &uDoubleLen);

	if (!pDouble)
		return WA_ERROR_MALLOCERROR;
	
	WA_ParamEQ_Bytes_To_Float(This, pBuffer, uBufferLen, pDouble, uDoubleLen);

	if (pInstance->bEnableBoost)
	{
		WA_Volume_Boost_Process(pInstance->pBoost, pDouble, uDoubleLen);
	}

	if (pInstance->bEnableEq)
	{
		for (uint16_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
		{
			WA_Biquad_Process(pInstance->BiquadArray[i], pDouble, uDoubleLen, pInstance->Format.uChannels);
		}
	}	

	WA_ParamEQ_Double_To_Bytes(This, pDouble, uDoubleLen, pBuffer, uBufferLen);
	WA_ParamEQ_FreeDouble(This, pDouble);		
	(*puProcessedBytes) = uBufferLen;

	return WA_OK;

}



void WA_ParamEQ_ConfigDialog(WA_Effect* This, HWND hParent)
{
	WA_ParamEQ_Intance* pInstance = (WA_ParamEQ_Intance*)This->hPluginData;

	DialogBox(GetCurrentModuleHandle(), MAKEINTRESOURCE(IDD_EQ_DIALOG), hParent, DialogEQProc);
}