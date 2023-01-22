
#include "stdafx.h"
#include "WA_FFT.h"
#include "WA_UI_Visualizations.h"
#include "Utility.h"
#include "WA_UI_ColorPolicy.h"


#define RGB_TO_D2D_COLORF(this,rf,gf,bf, af) this.r = rf / 255.0f; this.g = gf/ 255.0f; this.b = bf / 255.0f; this.a = af / 255.0f

const static IID IID_ID2D1Factory = { 0x06152247,0x6F50,0x465A,0x92,0x45,0x11,0x8B,0xFD,0x3B,0x60,0x07};	// 06152247-6f50-465a-9245-118bfd3b6007

LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


struct TagWA_Visualizations
{
	ID2D1Factory* pFactory;
	ID2D1HwndRenderTarget* pTarget;
	ID2D1SolidColorBrush* pFillBrush;

	HWND hStatic;
	D2D1_SIZE_U StaticSize;

	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;

	uint32_t FrequencyMinIndex;
	uint32_t FrequencyMaxIndex;

	float InBuffer[WA_VISUALIZATIONS_INPUT_BUFFER];
	float OutBuffer[WA_VISUALIZATIONS_OUTPUT_FFT_HALF];
	float fBandDB[WA_VISUALIZATIONS_OUTPUT_FFT_HALF];
	uint32_t pIndexesTable[WA_VISUALIZATIONS_OUTPUT_FFT_HALF];

	WA_FFT* pFFT;
};


static inline uint32_t WA_Visualizations_Get_VisibleBands(WA_Visualizations* This)
{
	uint32_t uVisibleBands;

	if (This->StaticSize.width == 0)
		return 0U;
	
	uVisibleBands = (uint32_t)(This->StaticSize.width / (WA_VISUALIZATIONS_BAND_WIDTH + WA_VISUALIZATIONS_BAND_SPACE));
	uVisibleBands = min(WA_VISUALIZATIONS_OUTPUT_FFT_HALF, uVisibleBands);

	return uVisibleBands;
}


static void WA_Visualizations_Cache_IndexesTable(WA_Visualizations* This)
{
	uint32_t uVisibleBands, uIndex, uCount;
	float fMaxIndexLog2;
	bool bContinue;

	uVisibleBands = WA_Visualizations_Get_VisibleBands(This);
	fMaxIndexLog2 = log2f((float) This->FrequencyMaxIndex);

	uCount = 0U;

	for (uint32_t i = 0; i < uVisibleBands; i++)
	{

		uIndex = (uint32_t) (0.5f + powf(2.0f, ((float) i / (uVisibleBands - 1)) * fMaxIndexLog2));
		uIndex = max(uIndex, This->FrequencyMinIndex);

		bContinue = true;

		do
		{
			if (uCount == 0U)
			{
				This->pIndexesTable[uCount] = uIndex;
				uCount++;
				bContinue = false;
			}
			else
			{
				bool bFound = false;

				for (uint32_t j = 0; j < uCount; j++)
				{
					if (This->pIndexesTable[j] == uIndex)
					{
						bFound = true;
						uIndex++;
						break;
					}
				}

				if (!bFound)
				{					
					This->pIndexesTable[uCount] = uIndex;
					uCount++;
					bContinue = false;
				}
			}

		} while (bContinue);
	}

}


static void WA_Visualizations_Bytes_To_Float(WA_Visualizations* This, int8_t* pByte, float* pFloatSamples)
{

	uint32_t i;

	switch (This->uChannels)
	{
	case 1:
		switch (This->uBitsPerSample)
		{
		case 8:
		{
			uint32_t uOffset = 0;
			int8_t nMono;

			for (i = 0; i < WA_VISUALIZATIONS_INPUT_BUFFER; i++)
			{
				memcpy(&nMono, pByte + uOffset, 1);
				uOffset += 1;

				pFloatSamples[i] = nMono / 127.0f;
			}

			break;
		}
		case 16:
		{
			uint32_t uOffset = 0;
			int16_t nMono;

			for (i = 0; i < WA_VISUALIZATIONS_INPUT_BUFFER; i++)
			{
				memcpy(&nMono, pByte + uOffset, 2);
				uOffset += 2;

				pFloatSamples[i] = nMono / 32767.0f;
			}

			break;
		}
		}
	case 2:
		switch (This->uBitsPerSample)
		{
		case 8:
		{	

			uint32_t uOffset = 0;
			int8_t Channels[2];

			for (uint32_t i = 0U; i < WA_VISUALIZATIONS_INPUT_BUFFER; i++)
			{
				memcpy(&Channels, pByte + uOffset, 2);
				uOffset += 2;

				pFloatSamples[i] = (Channels[0] + Channels[1]) / 255.0f;
			}

			break;
		}
		case 16:
		{
			uint32_t uOffset = 0;
			int16_t Channels[2];

			for (uint32_t i = 0U; i < WA_VISUALIZATIONS_INPUT_BUFFER; i++)
			{
				memcpy(&Channels, pByte + uOffset, 4);
				uOffset += 4;		

				pFloatSamples[i] = (Channels[0] + Channels[1]) / 65534.0f;			
			}

			break;
		}
		}
	}

	// TODO: IMPLEMENT 24 bits and 32 bits Samples

}



static float WA_Visualizations_Avg_Bands(WA_Visualizations* This, float* fSamples, uint32_t uStartIndex, uint32_t uEndIndex)
{
	uint32_t i;
	float fValue = 0.00001f;

	if (uStartIndex == uEndIndex)
		return fSamples[uStartIndex];

	for (i = uStartIndex; i < uEndIndex; i++)
	{
		fValue += fSamples[i] > 0.00001f ? fSamples[i] : 0.00001f;
	}

	return (fValue / (uEndIndex - uStartIndex));

}


WA_Visualizations* WA_Visualizations_New(HWND hStatic)
{	
	HRESULT hr;
	D2D1_PIXEL_FORMAT PixelFormat;
	D2D1_RENDER_TARGET_PROPERTIES TargetProperies;
	D2D1_HWND_RENDER_TARGET_PROPERTIES HwndTargetProperies;
	RECT StaticRect;
	WA_Visualizations* This;
	D2D1_FACTORY_OPTIONS FactoryOptions;
	D2D1_COLOR_F FillColor;
	
	if (!hStatic)
		return false;

	This = calloc(1, sizeof(WA_Visualizations));

	if (!This)
		return false;

	This->pFFT = WA_FFT_New();


	if (!This->pFFT)
	{
		free(This);
		return NULL;
	}


#if _DEBUG	
	FactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	FactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif


	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&IID_ID2D1Factory,
		&FactoryOptions,
		&This->pFactory);

	if FAILED(hr)
	{
		free(This);
		return NULL;
	}


	// Set Pixel format
	PixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
	PixelFormat.format = DXGI_FORMAT_UNKNOWN;

	// Set Render Target Properties
	TargetProperies.dpiX = 0.0f;
	TargetProperies.dpiY = 0.0f;
	TargetProperies.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	TargetProperies.usage = D2D1_RENDER_TARGET_USAGE_NONE;
	TargetProperies.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	TargetProperies.pixelFormat = PixelFormat;


	// Set Hwnd Render target properties
	HwndTargetProperies.hwnd = hStatic;


	if (GetClientRect(hStatic, &StaticRect))
	{

		_ASSERT(StaticRect.right > 0);
		_ASSERT(StaticRect.bottom > 0);

		This->StaticSize.width = StaticRect.right;
		This->StaticSize.height = StaticRect.bottom;

		HwndTargetProperies.pixelSize = This->StaticSize;
	}


	HwndTargetProperies.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

	// Create Render Targer
	hr = ID2D1Factory_CreateHwndRenderTarget(This->pFactory,
		&TargetProperies,
		&HwndTargetProperies,
		&This->pTarget);


	if FAILED(hr)
	{
		ID2D1Factory_Release(This->pFactory);
		This->pFactory = NULL;

		free(This);
		return NULL;
	}

	FillColor.a = 1.0f;
	FillColor.r = GetRValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
	FillColor.g = GetGValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
	FillColor.b = GetBValue(ColorPolicy_Get_Primary_Color()) / 255.0f;


	hr = ID2D1HwndRenderTarget_CreateSolidColorBrush(This->pTarget, &FillColor, NULL, &This->pFillBrush);

	if FAILED(hr)
	{
		ID2D1HwndRenderTarget_Release(This->pTarget);
		ID2D1Factory_Release(This->pFactory);

		This->pTarget = NULL;
		This->pFactory = NULL;

		free(This);
		return NULL;
	}


	ZeroMemory(This->fBandDB, sizeof(float)* WA_VISUALIZATIONS_OUTPUT_FFT_HALF);


	// Subclass to intercept size events
	SetWindowSubclass(hStatic, StaticSubclassProc, WA_VISUALIZATIONS_SUBCLASS_ID, (DWORD_PTR) This);


	return This;
}

void WA_Visualizations_Delete(WA_Visualizations* This)
{

	if (!This)
		return;

	RemoveWindowSubclass(This->hStatic, StaticSubclassProc, WA_VISUALIZATIONS_SUBCLASS_ID);

	if (This->pFillBrush)
		ID2D1SolidColorBrush_Release(This->pFillBrush);

	if (This->pTarget)
		ID2D1HwndRenderTarget_Release(This->pTarget);

	if (This->pFactory)
		ID2D1Factory_Release(This->pFactory);

	if (This->pFFT)	
		WA_FFT_Delete(This->pFFT);

	free(This);
	This = NULL;
}

void WA_Visualizations_UpdateFormat(WA_Visualizations* This, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample)
{
	float fMaxFrequencyAllowed;

	This->uSamplerate = uSamplerate;
	This->uChannels = uChannels;
	This->uBitsPerSample = uBitsPerSample;
	This->FrequencyMinIndex = (uint32_t)((WA_VISUALIZATIONS_FREQUENCY_MIN / (uSamplerate / 2.0f)) * WA_VISUALIZATIONS_OUTPUT_FFT_HALF);
	
	// Cuts the maximum frequency if it is above the Nyquist frequency
	fMaxFrequencyAllowed = min(WA_VISUALIZATIONS_FREQUENCY_MAX, (uSamplerate / 2.0f));
	
	This->FrequencyMaxIndex = (uint32_t)((fMaxFrequencyAllowed / (uSamplerate / 2.0f)) * WA_VISUALIZATIONS_OUTPUT_FFT_HALF);

	WA_Visualizations_Cache_IndexesTable(This);
	ZeroMemory(This->fBandDB, sizeof(float) * WA_VISUALIZATIONS_OUTPUT_FFT_HALF);
}



void WA_Visualizations_Draw(WA_Visualizations* This, int8_t* pBuffer)
{
	D2D1_COLOR_F BackgroundColor;
	D2D1_RECT_F DrawRect;
	uint32_t uVisibleBand, i, uDrawIndex, uPrevIndex;
	float fDrawValue, fBarX, fMaxHeight;

	RGB_TO_D2D_COLORF(BackgroundColor, 0.0f, 0.0f, 0.0f, 255.0f);

	uVisibleBand = WA_Visualizations_Get_VisibleBands(This);

	WA_Visualizations_Bytes_To_Float(This, pBuffer, This->InBuffer);

	WA_FFT_TimeToFrequencyDomain(This->pFFT, This->InBuffer, This->OutBuffer, true);

	fMaxHeight = (float)This->StaticSize.height;


	ID2D1HwndRenderTarget_BeginDraw(This->pTarget);

	// Clear Background
	ID2D1HwndRenderTarget_Clear(This->pTarget, &BackgroundColor);

	// Draw Bands
	uPrevIndex = 0;
	uDrawIndex = 0;
	fBarX = 1.0f;

	for (i = 0; i < uVisibleBand; i++)
	{				
		uDrawIndex = This->pIndexesTable[i]; 

		fDrawValue = WA_Visualizations_Avg_Bands(This, This->OutBuffer, uPrevIndex, uDrawIndex);
		fDrawValue = (fDrawValue / 8.5f) * fMaxHeight;
		fDrawValue = min(fMaxHeight, fDrawValue);


		if (This->fBandDB[i] < fDrawValue)
		{
			This->fBandDB[i] = fDrawValue;
		}
		else
		{
			This->fBandDB[i] -= 0.8f;
			This->fBandDB[i] = max(This->fBandDB[i], 0.0f);
		}
		
		fDrawValue = This->fBandDB[i];				


		DrawRect.left = fBarX;
		DrawRect.right = fBarX + WA_VISUALIZATIONS_BAND_WIDTH;
		DrawRect.bottom = (float) This->StaticSize.height;
		DrawRect.top = (float) This->StaticSize.height - fDrawValue;

		ID2D1HwndRenderTarget_FillRectangle(This->pTarget, &DrawRect, (ID2D1Brush*) This->pFillBrush);
					
		fBarX += WA_VISUALIZATIONS_BAND_WIDTH + WA_VISUALIZATIONS_BAND_SPACE;
		uPrevIndex = uDrawIndex;
	}


	// End Drawing
	ID2D1HwndRenderTarget_EndDraw(This->pTarget, 0, 0);
	

}

void WA_Visualizations_Clear(WA_Visualizations* This)
{
	D2D1_COLOR_F BackgroundColor;

	RGB_TO_D2D_COLORF(BackgroundColor, 0.0f, 0.0f, 0.0f, 255.0f);

	ID2D1HwndRenderTarget_BeginDraw(This->pTarget);

	// Clear Background
	ID2D1HwndRenderTarget_Clear(This->pTarget, &BackgroundColor);	

	ID2D1HwndRenderTarget_EndDraw(This->pTarget, 0, 0);

	ZeroMemory(This->fBandDB, sizeof(float) * WA_VISUALIZATIONS_OUTPUT_FFT_HALF);
}


LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

	switch (uMsg)
	{
	case WM_SIZE:
	{
		WA_Visualizations* This = (WA_Visualizations *) dwRefData;

		// Check if we have a valid pointer
		if (This)
		{
			D2D1_SIZE_U dSize;

			dSize.width = LOWORD(lParam);
			dSize.height = HIWORD(lParam);

			This->StaticSize = dSize;

			if(This)
				ID2D1HwndRenderTarget_Resize(This->pTarget, &dSize);

		}		
	}

		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int8_t* WA_Visualizations_AllocBuffer(WA_Visualizations* This, uint32_t* puBufferSize)
{
	uint32_t uAdjustedSize;
	int8_t* pBuffer;

	uAdjustedSize = WA_VISUALIZATIONS_INPUT_BUFFER * (This->uBitsPerSample * This->uChannels) / 8;

	pBuffer = (int8_t*)calloc(uAdjustedSize, sizeof(int8_t));

	if (!pBuffer)
		return NULL;

	(*puBufferSize) = uAdjustedSize;

	return pBuffer;
}

void WA_Visualizations_FreeBuffer(WA_Visualizations* This, int8_t* pBuffer)
{
	if (!pBuffer)
		return;

	free(pBuffer);
	pBuffer = NULL;

}

