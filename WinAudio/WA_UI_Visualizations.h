
#ifndef WA_UI_VISUALIZATION_H
#define WA_UI_VISUALIZATION_H


// Warning: Don't Touch Thoose Parameters
#define WA_VISUALIZATIONS_INPUT_BUFFER		WA_FFT_INPUT_SIZE 
#define WA_VISUALIZATIONS_OUTPUT_FFT		WA_FFT_OUTPUT_SIZE
#define WA_VISUALIZATIONS_OUTPUT_FFT_HALF	WA_FFT_OUTPUT_SIZE_HALF


#define WA_VISUALIZATIONS_SUBCLASS_ID				1109
#define WA_VISUALIZATIONS_BAND_WIDTH				3.0f
#define WA_VISUALIZATIONS_BAND_SPACE				1.0f
#define WA_VISUALIZATIONS_FREQUENCY_MIN				45.0f
#define WA_VISUALIZATIONS_FREQUENCY_MAX				16000.0f 
#define WA_VISUALIZATIONS_SCALE_FACTOR				0.11764706f
#define WA_VISUALIZATIONS_MIN_SAMPLE				0.1f 
#define WA_VISUALIZATIONS_FALLOFF_VELOCITY			0.8f 


struct TagWA_Visualizations;
typedef struct TagWA_Visualizations WA_Visualizations;

WA_Visualizations * WA_Visualizations_New(HWND hStatic);
void WA_Visualizations_Delete(WA_Visualizations* This);

void WA_Visualizations_UpdateFormat(WA_Visualizations* This, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample);
void WA_Visualizations_Draw(WA_Visualizations* This, int8_t* pByteBuffer);
void WA_Visualizations_Clear(WA_Visualizations* This);

int8_t* WA_Visualizations_AllocBuffer(WA_Visualizations* This, uint32_t* puBufferSize);
void WA_Visualizations_FreeBuffer(WA_Visualizations* This, int8_t* pBuffer);

#endif
