
#include "stdafx.h"
#include "fft.h"
#include "Visualization.h"
#include "Utility.h"
#include "WA_UI_ColorPolicy.h"

#define RGB_TO_D2D_COLORF(this,rf,gf,bf, af) this.r = rf / 255.0f; this.g = gf/ 255.0f; this.b = bf / 255.0f; this.a = af / 255.0f

const static IID IID_ID2D1Factory = { 0x06152247,0x6F50,0x465A,0x92,0x45,0x11,0x8B,0xFD,0x3B,0x60,0x07};	// 06152247-6f50-465a-9245-118bfd3b6007

LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


static void vis_byte_to_float_samples(Visualization* pHandle, int8_t* pByte, float* pFloatSamples)
{

	uint32_t i;

	switch (pHandle->uChannels)
	{
	case 1:
		switch (pHandle->uBitsPerSample)
		{
		case 8:
		{
			for (i = 0; i < pHandle->uSamplesSize; i++)
			{
				pFloatSamples[i] = (float)(pByte[i]) / INT8_MAX;
			}

			break;
		}
		case 16:
		{
			for (i = 0; i < pHandle->uSamplesSize; i++)
			{
				pFloatSamples[i] = (float)(pByte[i * 2 + 0] | (pByte[i * 2 + 1] << 8)) / INT16_MAX;
			}

			break;
		}
		}
	case 2:
		switch (pHandle->uBitsPerSample)
		{
		case 8:
		{	

			float fLeft, fRight;

			for (i = 0; i < pHandle->uSamplesSize; i++)
			{
				fLeft = (float)(pByte[i * 2 + 0]) / INT8_MAX;
				fRight = (float)(pByte[i * 2 + 1]) / INT8_MAX;

				pFloatSamples[i] = (fLeft + fRight) / 2.0f;
			}

			break;
		}
		case 16:
		{
			ult_16b_2c_bytes_to_16b_1c_float(pByte, pFloatSamples, pHandle->uSamplesSize);
			break;
		}
		}
	}

	// TODO: IMPLEMENT 24 bits and 32 bits Samples

}



static float vis_avg_bands(float* fSamples, uint32_t uStartIndex, uint32_t uEndIndex)
{
	uint32_t i;
	float fValue = 0;

	for (i = uStartIndex; i < uEndIndex; i++)
	{
		fValue += fSamples[i];
	}

	return (fValue / (uEndIndex - uStartIndex));

}

static inline float mlogf(float x, float y)
{
	return (logf(x) / logf(y));	
}


static uint32_t vis_Band_to_index(Visualization* pHandle, uint32_t BandIndex, uint32_t MaxBand)
{
	/*
	float fLogValue;	

	
	fLogValue = 1.0f - mlogf(((float) MaxBand + 1.0f) - (float) BandIndex, ((float) MaxBand + 1.0f));
	fLogValue = fLogValue * (float)(pHandle->FrequencyMaxIndex - pHandle->FrequencyMinIndex);
	fLogValue = fLogValue + (float)(pHandle->FrequencyMinIndex);
	




	return (uint32_t)fLogValue;
	*/


	uint32_t uIndex;

	uIndex = (BandIndex * pHandle->FrequencyMaxIndex) / MaxBand;
	uIndex += pHandle->FrequencyMinIndex;

	return uIndex;


}

bool vis_Init(Visualization** pHandle, HWND hStaticHandle, uint16_t uSamplesSize)
{	
	HRESULT hr;
	D2D1_PIXEL_FORMAT PixelFormat;
	D2D1_RENDER_TARGET_PROPERTIES TargetProperies;
	D2D1_HWND_RENDER_TARGET_PROPERTIES HwndTargetProperies;
	ID2D1GradientStopCollection* pGradientStopCollection = NULL;
	D2D1_GRADIENT_STOP gradientStops[3];
	RECT StaticRect;

	
	if (!hStaticHandle)
		return false;	

	if (uSamplesSize < 256)
		return false;

	(*pHandle) = malloc(sizeof(Visualization));

	if (!(*pHandle))
		return false;
	

	(*pHandle)->hStaticHandle = hStaticHandle;
	(*pHandle)->uByteSampleSize = uSamplesSize;

	// Clear Resources
	(*pHandle)->pGradientBrush = NULL;
	(*pHandle)->pTarget = NULL;
	(*pHandle)->pFactory = NULL;
	(*pHandle)->FFTHandle = NULL;
	(*pHandle)->fOldFFT = NULL;
	(*pHandle)->fSamples = NULL;
	(*pHandle)->fSpectrum = NULL;


#if _DEBUG
	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&IID_ID2D1Factory,
		&options,
		&(*pHandle)->pFactory);

#else
	
	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_NONE;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&IID_ID2D1Factory,
		&options,
		&(*pHandle)->pFactory);

#endif

	if (SUCCEEDED(hr))
	{
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
		HwndTargetProperies.hwnd = hStaticHandle;


		if (GetClientRect(hStaticHandle, &StaticRect))
		{

			_ASSERT(StaticRect.right > 0);
			_ASSERT(StaticRect.bottom > 0);

			(*pHandle)->StaticSize.width = StaticRect.right;
			(*pHandle)->StaticSize.height = StaticRect.bottom;

			HwndTargetProperies.pixelSize = (*pHandle)->StaticSize;
		}

		HwndTargetProperies.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

		// Create Render Targer
		hr = ID2D1Factory_CreateHwndRenderTarget((*pHandle)->pFactory,
			&TargetProperies,
			&HwndTargetProperies,
			&(*pHandle)->pTarget);

		if SUCCEEDED(hr)
		{
			// Create Gradient Brush
			D2D1_COLOR_F StartColor, MiddleColor, EndColor;		

			// Define Color stops
			/*
			RGB_TO_D2D_COLORF(StartColor, 163.0f, 47.0f, 246.0f, 245.0f);
			RGB_TO_D2D_COLORF(MiddleColor, 57.0f, 214.0f, 246.0f, 250.0f);
			RGB_TO_D2D_COLORF(EndColor, 57.0f, 80.0f, 235.0f, 230.0f);
			*/

			StartColor.a = 1.0f;
			StartColor.r = GetRValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			StartColor.g = GetGValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			StartColor.b = GetBValue(ColorPolicy_Get_Primary_Color()) / 255.0f;

			MiddleColor.a = 1.0f;
			MiddleColor.r = GetRValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			MiddleColor.g = GetGValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			MiddleColor.b = GetBValue(ColorPolicy_Get_Primary_Color()) / 255.0f;

			EndColor.a = 1.0f;
			EndColor.r = GetRValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			EndColor.g = GetGValue(ColorPolicy_Get_Primary_Color()) / 255.0f;
			EndColor.b = GetBValue(ColorPolicy_Get_Primary_Color()) / 255.0f;

			gradientStops[0].color = StartColor;
			gradientStops[0].position = 0.0f;
			gradientStops[1].color = MiddleColor;
			gradientStops[1].position = 0.5f;
			gradientStops[2].color = EndColor;
			gradientStops[2].position = 1.0f;

			// Create Gradient Stop Collection
			hr = ID2D1HwndRenderTarget_CreateGradientStopCollection((*pHandle)->pTarget,
				gradientStops,
				3,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStopCollection);

			if SUCCEEDED(hr)
			{

				D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES LinearGradientProperties;
				D2D1_BRUSH_PROPERTIES BrushProperties;
				D2D1_POINT_2F StartPoint, EndPoint;
				D2D1_MATRIX_3X2_F IdentityMatrix;

				// Start Point of Gradient
				StartPoint.x = 0.0f;
				StartPoint.y = 0.0f;

				// End Point of Gradient
				EndPoint.x = 0.0f;
				EndPoint.y = (FLOAT)StaticRect.bottom;

				LinearGradientProperties.startPoint = StartPoint;
				LinearGradientProperties.endPoint = EndPoint;


				// Identity Matrix = No trasformations
				IdentityMatrix._11 = 1.0f;
				IdentityMatrix._12 = 0.0f;
				IdentityMatrix._21 = 0.0f;
				IdentityMatrix._22 = 1.0f;
				IdentityMatrix._31 = 0.0f;
				IdentityMatrix._32 = 0.0f;

				BrushProperties.opacity = 1.0f;
				BrushProperties.transform = IdentityMatrix;

				hr = ID2D1HwndRenderTarget_CreateLinearGradientBrush((*pHandle)->pTarget,
					&LinearGradientProperties,
					&BrushProperties,
					pGradientStopCollection,
					&(*pHandle)->pGradientBrush);

				if SUCCEEDED(hr)
				{
					ID2D1GradientStopCollection_Release(pGradientStopCollection);

					// Subclass to intercept size events
					SetWindowSubclass(hStaticHandle, StaticSubclassProc, VIS_STATIC_SUBCLASS_ID, 0);


					// Copy D2D1 Instance to use in subclass
					SetProp(hStaticHandle, L"VIS_HANDLE", pHandle);


					_RPTF0(_CRT_WARN, "Direct2D Successful Init! \n");
					return true;
				}
				
			}
		}
	}

	// On Error Safe Release Resources
	if (pGradientStopCollection)
		ID2D1GradientStopCollection_Release(pGradientStopCollection);	

	if ((*pHandle)->pGradientBrush)
		ID2D1LinearGradientBrush_Release((*pHandle)->pGradientBrush);

	if ((*pHandle)->pTarget)
		ID2D1HwndRenderTarget_Release((*pHandle)->pTarget);

	if ((*pHandle)->pFactory)
		ID2D1Factory_Release((*pHandle)->pFactory);


	pGradientStopCollection = NULL;
	(*pHandle)->pGradientBrush = NULL;
	(*pHandle)->pTarget = NULL;
	(*pHandle)->pFactory = NULL;

	return false;
}

void vis_Destroy(Visualization** pHandle)
{
	if ((*pHandle))
	{
		RemoveWindowSubclass((*pHandle)->hStaticHandle, StaticSubclassProc, VIS_STATIC_SUBCLASS_ID);

		if ((*pHandle)->pGradientBrush)
			ID2D1LinearGradientBrush_Release((*pHandle)->pGradientBrush);

		if ((*pHandle)->pTarget)
			ID2D1HwndRenderTarget_Release((*pHandle)->pTarget);

		if ((*pHandle)->pFactory)
			ID2D1Factory_Release((*pHandle)->pFactory);

		// Free FFT Resources
		if ((*pHandle)->FFTHandle)
			fft_Destroy(&(*pHandle)->FFTHandle);

		if ((*pHandle)->fOldFFT)
			free((*pHandle)->fOldFFT);

		if ((*pHandle)->fSamples)
		{
			free((*pHandle)->fSamples);
			(*pHandle)->fSamples = NULL;
		}

		if ((*pHandle)->fSpectrum)
		{
			free((*pHandle)->fSpectrum);
			(*pHandle)->fSpectrum = NULL;
		}


		(*pHandle)->pGradientBrush = NULL;
		(*pHandle)->pTarget = NULL;
		(*pHandle)->pFactory = NULL;
		(*pHandle)->fOldFFT = NULL;
		(*pHandle)->FFTHandle = NULL;

		free((*pHandle));

		_RPTF0(_CRT_WARN, "Direct2D Successful Destroyed! \n");
	}
}

void vis_Update(Visualization* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample)
{
	pHandle->uSamplerate = uSamplerate;
	pHandle->uChannels = uChannels;
	pHandle->uBitsPerSample = uBitsPerSample;
	pHandle->uSamplesSize = pHandle->uByteSampleSize / ((uBitsPerSample * uChannels) / 8);
	pHandle->uSamplesSize2 = pHandle->uSamplesSize / 2;
	pHandle->FrequencyMinIndex = (uint32_t)((FREQUENCY_MIN / (uSamplerate / 2.0f)) * pHandle->uSamplesSize2);
	//pHandle->FrequencyMaxIndex = (uint32_t)(((uSamplerate / 4.0f) / (uSamplerate / 2.0f)) * (pHandle->uSamplesSize / 2.0f));
	pHandle->FrequencyMaxIndex = (uint32_t)((FREQUENCY_MAX / (uSamplerate / 2.0f)) * pHandle->uSamplesSize2);

	

	_ASSERT(pHandle->uSamplesSize >= 256);

	if (pHandle->FFTHandle)
		fft_Destroy(&pHandle->FFTHandle);

	pHandle->FFTHandle = NULL;

	// Update FFT (code should be smart enough to success init)
	fft_Init(&pHandle->FFTHandle, pHandle->uSamplesSize, pHandle->uSamplesSize);

	// Check if there is an already allocated memory
	ult_safe_release_malloc(&pHandle->fOldFFT);

	if (ult_safe_new_malloc(&pHandle->fOldFFT, pHandle->uSamplesSize * sizeof(float)))
		ZeroMemory(pHandle->fOldFFT, pHandle->uSamplesSize * sizeof(float));

	ult_safe_release_malloc(&pHandle->fSamples);
	ult_safe_release_malloc(&pHandle->fSpectrum);

	// Allocate Memory to store Samples and FFT
	ult_safe_new_malloc(&pHandle->fSamples, pHandle->uSamplesSize * sizeof(float));
	ult_safe_new_malloc(&pHandle->fSpectrum, pHandle->uSamplesSize * sizeof(float));


}

void vis_Draw(Visualization* pHandle, int8_t* pByteBuffer)
{
	D2D1_COLOR_F BackgroundColor;
	D2D1_RECT_F DrawRect;
	uint32_t uVisibleBand, i, uDrawIndex, uPrevIndex;
	float *fSamples = NULL;
	float *fSpectrum = NULL;
	float fDrawValue, fBarX, fMaxHeight;


	// Check if FFT could be performed
	if (pHandle->FFTHandle)		
	{		

		// Calculate the number of visible bars
		uVisibleBand = (uint32_t)(pHandle->StaticSize.width / (VIS_BAND_WIDTH + VIS_BAND_SPACE));
		uVisibleBand = min(uVisibleBand, pHandle->uSamplesSize2);

		fMaxHeight = (float) pHandle->StaticSize.height;
		
		fSamples = pHandle->fSamples;
		fSpectrum = pHandle->fSpectrum;

		if ((fSamples) && (fSpectrum))
		{
			RGB_TO_D2D_COLORF(BackgroundColor, 0.0f, 0.0f, 0.0f, 255.0f);

			// Convert from Byte to Float Samples
			vis_byte_to_float_samples(pHandle, pByteBuffer, fSamples);

			// Perform FFT
			fft_TimeToFrequencyDomain(pHandle->FFTHandle, fSamples, fSpectrum, true);

			// Start Drawing
			ID2D1HwndRenderTarget_BeginDraw(pHandle->pTarget);

			// Clear Background
			ID2D1HwndRenderTarget_Clear(pHandle->pTarget, &BackgroundColor);

			// Draw Bands
			uPrevIndex = 0;
			uDrawIndex = 0;
			fBarX = 1.0f;

			for (i = 1; i <= uVisibleBand; i++)
			{				
				uDrawIndex = vis_Band_to_index(pHandle, i, uVisibleBand);
				fDrawValue = vis_avg_bands(fSpectrum, uPrevIndex, uDrawIndex);
		
				fDrawValue = fDrawValue / 4.0f * fMaxHeight;

				
				
				// Clip value and draw
				if (pHandle->fOldFFT[uDrawIndex] < fDrawValue)
				{
					pHandle->fOldFFT[uDrawIndex] = fDrawValue;				
				}					
				else
				{
					if (pHandle->fOldFFT[uDrawIndex] > 1.0f)
					{			
						pHandle->fOldFFT[uDrawIndex] -= 1.0f; 					
					}
				}
	
				pHandle->fOldFFT[uDrawIndex] = max(1.0f, pHandle->fOldFFT[uDrawIndex]);			
				pHandle->fOldFFT[uDrawIndex] = min(fMaxHeight, pHandle->fOldFFT[uDrawIndex]);	

				DrawRect.left = fBarX;
				DrawRect.right = fBarX + VIS_BAND_WIDTH;
				DrawRect.bottom = (float) pHandle->StaticSize.height;
				DrawRect.top = (float) pHandle->StaticSize.height - pHandle->fOldFFT[uDrawIndex];

				ID2D1HwndRenderTarget_FillRectangle(pHandle->pTarget, &DrawRect, (ID2D1Brush*) pHandle->pGradientBrush);
					
				fBarX += VIS_BAND_WIDTH + VIS_BAND_SPACE;
				uPrevIndex = uDrawIndex + 1;
			}


			// End Drawing
			ID2D1HwndRenderTarget_EndDraw(pHandle->pTarget, 0, 0);


		}
	}
}

void vis_Clear(Visualization* pHandle)
{
	D2D1_COLOR_F BackgroundColor;

	RGB_TO_D2D_COLORF(BackgroundColor, 0.0f, 0.0f, 0.0f, 255.0f);

	ID2D1HwndRenderTarget_BeginDraw(pHandle->pTarget);

	// Clear Background
	ID2D1HwndRenderTarget_Clear(pHandle->pTarget, &BackgroundColor);
	

	ID2D1HwndRenderTarget_EndDraw(pHandle->pTarget, 0, 0);
}


LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

	switch (uMsg)
	{
	case WM_SIZE:
	{
		Visualization** pHandle;

		// Get D2D1 Instance
		pHandle = GetProp(hWnd, L"VIS_HANDLE");

		// Check if we have a valid pointer
		if ((*pHandle))
		{
			D2D1_SIZE_U dSize;

			dSize.width = LOWORD(lParam);
			dSize.height = HIWORD(lParam);

			(*pHandle)->StaticSize = dSize;

			ID2D1HwndRenderTarget_Resize((*pHandle)->pTarget, &dSize);
			_RPTF0(_CRT_WARN, "Static: Resize\n");
		}				
	}

		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


