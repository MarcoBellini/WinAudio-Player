
#include "stdafx.h"
#include "WA_FFT.h"
#include "WA_UI_Visualizations.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_DPI.h"

// 06152247-6f50-465a-9245-118bfd3b6007
static const IID IID_ID2D1Factory = { 0x06152247,0x6F50,0x465A,0x92,0x45,0x11,0x8B,0xFD,0x3B,0x60,0x07};	

LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


struct TagWA_Visualizations
{
	ID2D1Factory* pFactory;
	ID2D1HwndRenderTarget* pTarget;	

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

	// Support DPI Scaling
	float fBandWidth;
	float fBandSpace;
	float fScaleFactor;
	float fFalloffVelocity;	
};


static inline uint32_t WA_Visualizations_Get_VisibleBands(WA_Visualizations* This)
{
	uint32_t uVisibleBands;

	if (This->StaticSize.width == 0)
		return 0U;

	float fStaticWidthDIP = (This->StaticSize.width * 96.0f) / WA_DPI_GetCurrentDPI();

	uVisibleBands = (uint32_t)(fStaticWidthDIP / (This->fBandWidth + This->fBandSpace)) + 1;
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

static void WA_Visualizations_Bytes_To_Float(WA_Visualizations* This, int8_t* pByte, float* pSamples)
{
	
	uint16_t uChuckSize = This->uBitsPerSample / 8U;
	uint32_t uIndex = 0;

	for (uint32_t i = 0; i < WA_VISUALIZATIONS_INPUT_BUFFER; i++)
	{
		pSamples[i] = 0.0f;

		for (uint32_t j = 1; j <= This->uChannels; j++)
		{

			switch (This->uBitsPerSample)
			{
			case 8:
				pSamples[i] += (pByte[uIndex] & 0xFF) / 127.0f;
				break;
			case 16:
				pSamples[i] += ((pByte[uIndex] & 0xFF) | (pByte[uIndex + 1] << 8)) / 32767.0f;
				break;
			case 24:
				pSamples[i] += ((pByte[uIndex] & 0xFF) | ((pByte[uIndex + 1] << 8) & 0xFF00) |
					(pByte[uIndex + 2] << 16)) / 8388607.0f;
				break;
			case 32:
				pSamples[i] += ((pByte[uIndex] & 0xFF) | ((pByte[uIndex + 1] << 8) & 0xFF00) |
					((pByte[uIndex + 2] << 16) & 0xFF0000) | (pByte[uIndex + 3] << 24)) / 2147483647.0f;
			}


			uIndex += uChuckSize;

		}

		pSamples[i] /= This->uChannels;
	}
}



static float WA_Visualizations_Avg_Bands(WA_Visualizations* This, float* fSamples, uint32_t uStartIndex, uint32_t uEndIndex)
{
	uint32_t i;
	float fValue = 0.0f;

	if (uStartIndex == uEndIndex)
		return fSamples[uStartIndex];

	for (i = uStartIndex; i < uEndIndex; i++)
	{
		fValue += fSamples[i] > WA_VISUALIZATIONS_MIN_SAMPLE ? fSamples[i] : 0.0f;
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

		// Convert to DIPs
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


	ZeroMemory(This->fBandDB, sizeof(float)* WA_VISUALIZATIONS_OUTPUT_FFT_HALF);

	// Update DPI scaled values
	This->fBandWidth = WA_VISUALIZATIONS_BAND_WIDTH;
	This->fBandSpace = WA_VISUALIZATIONS_BAND_SPACE;
	This->fScaleFactor = WA_VISUALIZATIONS_SCALE_FACTOR;
	This->fFalloffVelocity = WA_VISUALIZATIONS_FALLOFF_VELOCITY;


	// Subclass to intercept size events
	SetWindowSubclass(hStatic, StaticSubclassProc, WA_VISUALIZATIONS_SUBCLASS_ID, (DWORD_PTR) This);

	return This;
}

void WA_Visualizations_Delete(WA_Visualizations* This)
{

	if (!This)
		return;

	RemoveWindowSubclass(This->hStatic, StaticSubclassProc, WA_VISUALIZATIONS_SUBCLASS_ID);



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
	ID2D1SolidColorBrush* pFillBrush;
	D2D1_COLOR_F BackgroundColor, FillColor;
	D2D1_RECT_F DrawRect;
	HRESULT hr;
	uint32_t uVisibleBand, i, uDrawIndex, uPrevIndex;
	float fDrawValue, fBarX, fMaxHeight;

	BackgroundColor.a = 1.0f;
	BackgroundColor.r = GetRValue(ColorPolicy_Get_Background_Color()) / 255.0f;
	BackgroundColor.g = GetGValue(ColorPolicy_Get_Background_Color()) / 255.0f;
	BackgroundColor.b = GetBValue(ColorPolicy_Get_Background_Color()) / 255.0f;

	FillColor.a = 1.0f;
	FillColor.r = GetRValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
	FillColor.g = GetGValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
	FillColor.b = GetBValue(ColorPolicy_Get_Primary_Color()) / 255.0f;

	hr = ID2D1HwndRenderTarget_CreateSolidColorBrush(This->pTarget, &FillColor, NULL, &pFillBrush);

	if FAILED(hr)
		return;	

	WA_Visualizations_Bytes_To_Float(This, pBuffer, This->InBuffer);
	WA_FFT_TimeToFrequencyDomain(This->pFFT, This->InBuffer, This->OutBuffer, true);

	fMaxHeight = (This->StaticSize.height * 96.0f) / WA_DPI_GetCurrentDPI(); // Convert to DIPs
	uVisibleBand = WA_Visualizations_Get_VisibleBands(This);

	ID2D1HwndRenderTarget_BeginDraw(This->pTarget);
	ID2D1HwndRenderTarget_Clear(This->pTarget, &BackgroundColor);
	
	uPrevIndex = This->pIndexesTable[0];	
	fBarX = 1.0f;

	for (i = 1U; i < uVisibleBand; i++)
	{				
		uDrawIndex = This->pIndexesTable[i]; 		

		fDrawValue = WA_Visualizations_Avg_Bands(This, This->OutBuffer, uPrevIndex, uDrawIndex);
		fDrawValue *= This->fScaleFactor;
		fDrawValue = min(fMaxHeight, fDrawValue);


		if ((This->fBandDB[i] < fDrawValue) && (fDrawValue > 1.5f))
		{
			This->fBandDB[i] = fDrawValue;
		}
		else
		{
			This->fBandDB[i] -= This->fFalloffVelocity;
			This->fBandDB[i] = max(This->fBandDB[i], 0.0f);
		}	

		DrawRect.left = fBarX;
		DrawRect.right = fBarX + This->fBandWidth;
		DrawRect.bottom = fMaxHeight;
		DrawRect.top = fMaxHeight - This->fBandDB[i];

		ID2D1HwndRenderTarget_FillRectangle(This->pTarget, &DrawRect, (ID2D1Brush*) pFillBrush);

		fBarX += This->fBandWidth + This->fBandSpace;
		uPrevIndex = uDrawIndex;
	}

	ID2D1HwndRenderTarget_EndDraw(This->pTarget, 0, 0);	
	ID2D1SolidColorBrush_Release(pFillBrush);
}

void WA_Visualizations_Clear(WA_Visualizations* This)
{
	D2D1_COLOR_F BackgroundColor;

	BackgroundColor.a = 1.0f;
	BackgroundColor.r = GetRValue(ColorPolicy_Get_Background_Color()) / 255.0f;
	BackgroundColor.g = GetGValue(ColorPolicy_Get_Background_Color()) / 255.0f;
	BackgroundColor.b = GetBValue(ColorPolicy_Get_Background_Color()) / 255.0f;

	ID2D1HwndRenderTarget_BeginDraw(This->pTarget);
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
		
		if (This)
		{
			D2D1_SIZE_U dSize;

			dSize.width = LOWORD(lParam);
			dSize.height = HIWORD(lParam);

			This->StaticSize = dSize;
		
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

