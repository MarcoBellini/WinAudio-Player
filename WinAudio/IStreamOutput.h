

#ifndef ISTREAMOUTPUT_H
#define ISTREAMOUTPUT_H


/* Forward Reference*/
struct tagIStreamOutput;
typedef struct tagIStreamOutput* OUTPUT_HANDLE;


/* Definition of function pointers */
typedef bool (*pOutput_CreateDevice)(OUTPUT_HANDLE pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample, uint32_t *uMaxLatencyMs);
typedef bool (*pOutput_CloseDevice)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_CanWrite)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_GetByteCanWrite)(OUTPUT_HANDLE pHandle, uint32_t* uByteToWrite);
typedef bool (*pOutput_WriteToDevice)(OUTPUT_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uByteToWrite);
typedef bool (*pOutput_FlushBuffers)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_IsPlaying)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_DevicePlay)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_DevicePause)(OUTPUT_HANDLE pHandle, bool bIsInPause);
typedef bool (*pOutput_DeviceStop)(OUTPUT_HANDLE pHandle);
typedef bool (*pOutput_GetWriteTime)(OUTPUT_HANDLE pHandle, float* fWriteTimeMs);
typedef bool (*pOutput_GetPlayTime)(OUTPUT_HANDLE pHandle, float* fPlayTimeMs);
typedef bool (*pOutput_DeviceVolume)(OUTPUT_HANDLE pHandle, uint8_t* uVolumeValue, bool bGetOrSet);


/* Template for all output modules */
typedef struct tagIStreamOutput
{
	/* Pointer to module functions */
	pOutput_CreateDevice output_CreateDevice;
	pOutput_CloseDevice output_CloseDevice;
	pOutput_CanWrite output_CanWrite;
	pOutput_GetByteCanWrite output_GetByteCanWrite;
	pOutput_WriteToDevice output_WriteToDevice;
	pOutput_FlushBuffers output_FlushBuffers;
	pOutput_IsPlaying output_IsPlaying;
	pOutput_DevicePlay output_DevicePlay;
	pOutput_DevicePause output_DevicePause;
	pOutput_DevicePlay output_DeviceStop;
	pOutput_GetWriteTime output_GetWriteTime;
	pOutput_GetPlayTime output_GetPlayTime;
	pOutput_DeviceVolume output_DeviceVolume;

	// Handle Output Need to Write Event
	// (Value assigned by Decoder Manager)
	HANDLE hOutputWriteEvent;

	/* Allow to store private module Data*/
	void* pModulePrivateData;

}IStreamOutput;

// Wasapi Output
bool OutWasapi_Initialize(IStreamOutput* pStreamOutput);
bool OutWasapi_Deinitialize(IStreamOutput* pStreamOutput);

#endif
