
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#define VIS_INSAMPLES				4096
#define VIS_STATIC_SUBCLASS_ID		1109
#define VIS_BAND_WIDTH				1.0f
#define VIS_BAND_SPACE				1.0f
#define FREQUENCY_MIN				60.0f
#define FREQUENCY_MAX				10025.0f //5525.0f


typedef struct tagFFT mFFT;

typedef struct tagVisualization
{
	ID2D1Factory* pFactory;
	ID2D1HwndRenderTarget* pTarget;
	ID2D1LinearGradientBrush* pGradientBrush;

	uint32_t uByteSampleSize;
	uint32_t uSamplesSize;
	uint32_t uSamplesSize2;
	HWND hStaticHandle;
	D2D1_SIZE_U StaticSize;

	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	mFFT* FFTHandle;
	float* fOldFFT;
	float* fSamples;
	float* fSpectrum;

	uint32_t FrequencyMinIndex;
	uint32_t FrequencyMaxIndex;
}Visualization;


bool vis_Init(Visualization** pHandle, HWND hStaticHandle, uint16_t uSamplesSize);
void vis_Destroy(Visualization** pHandle);
void vis_Update(Visualization* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample);
void vis_Draw(Visualization* pHandle, int8_t* pByteBuffer);
void vis_Clear(Visualization* pHandle);



#endif
