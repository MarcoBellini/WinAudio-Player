
#ifndef DECODERMANAGER_H
#define DECODERMANAGER_H

#include "IStreamOutput.h"
#include "IStreamInput.h"
#include "CircleBuffer.h"
#include "SpectrumBuffer.h"
#include "Utility.h"

// Define Messages
#define DECODER_PLAY							(WM_APP + 0x001) // Send from main thread to Engine Thread
#define DECODER_PAUSE							(WM_APP + 0x002) // Send from main thread to Engine Thread
#define DECODER_STOP							(WM_APP + 0x003) // Send from main thread to Engine Thread
#define DECODER_SEEK							(WM_APP + 0x004) // Send from main thread to Engine Thread
#define DECODER_OPENFILE						(WM_APP + 0x005) // Send from main thread to Engine Thread
#define DECODER_CLOSEFILE						(WM_APP + 0x006) // Send from main thread to Engine Thread
#define DECODER_SETVOLUME						(WM_APP + 0x007) // Send from main thread to Engine Thread
#define DECODER_GETVOLUME						(WM_APP + 0x008) // Send from main thread to Engine Thread
#define DECODER_CLOSETHREAD						(WM_APP + 0x009) // Send from main thread to Engine Thread
#define DECODER_ENDOFSTREAM						(WM_APP + 0x00A) // ATTENTION!! Notified from the Engine to main Thread
#define DECODER_GETPLAYINGBUFFER				(WM_APP + 0x00B) // wParam = Pointer to pre-allocated buffer lParam= Size of the buffer
#define DECODER_GETPOSITION						(WM_APP + 0x00C) // Send from main thread to Engine Thread
#define DECODER_UNPAUSE							(WM_APP + 0x00D) // Send from main thread to Engine Thread
#define DECODER_ASSIGNDSPCALLBACK				(WM_APP + 0x00e) // Send from main thread to Engine Thread



// Define Some error code
#define DECODER_OK								0x000
#define DECODER_FILENOTFOUND					0x100
#define DECODER_FILENOTSUPPORTED				0x101
#define DECODER_BADWAVEFORMAT					0x102
#define DECODER_OUTPUTERROR						0x103
#define DECODER_CIRCLEBUFFERERROR				0x104
#define DECODER_DEVICENOTOPEN					0x105
#define DECODER_BADPTR							0x106

// Define Decoders (must be sequential and start from 0)
#define DECODER_WAVDECODER						0x0
#define DECODER_MFDECODER						0x1
#define DECODER_DECODER_MAX						DECODER_MFDECODER + 1
#define DECODER_INVALID_DECODER					0xFF

// Define Output (must be sequential and start from 0)
#define DECODER_WASAPIOUTPUT					0x0
#define DECODER_OUTPUT_MAX						DECODER_WASAPIOUTPUT + 1
#define DECODER_INVALID_OUTPUT					0xFF

#define DECODER_OUTPUT_MAX_LATENCY				200 // ms
#define DECODER_CBUFFER_LEN						(DECODER_OUTPUT_MAX_LATENCY * 3) // Circle Buffer size, don't use low values here
#define DECODER_OUTPUT_WAIT_TIME				(DECODER_OUTPUT_MAX_LATENCY / 2)
#define DECODER_OUTPUT_MAX_LATENCY_F			200.0f // ms


// Define Callback for DSP function (Assign a value to use it)
typedef void (*DecoderManager_DSP_Callback)(float* pLeftSamples, float* pRightSamples, uint32_t uSamplesSize);


// Used to Pass data between main thread 
// and engine thread
typedef struct tagDecoderManager
{
	// Used to Syncronize Main Thread messages
	// with Engine Thread
	HANDLE hSyncEvent;

	// Save some Handles
	HWND hMainWindowHandle;
	HANDLE hMainThreadHandle;
	DWORD dwThreadId;

	// Opened Stream Informations
	// Filled in the Engine thread 
	// Sending Proper Messages
	uint64_t uCurrentPositionMs;
	uint64_t uCurrentDurationMs;
	
	uint32_t uCurrentSamplerate;
	uint16_t uCurrentChannels;
	uint16_t uCurrentBitsPerSample;
	uint32_t uCurrentAvgBytesPerSec;
	uint16_t uCurrentBlockAlign;

	// Store error code
	uint16_t uErrorCode;

	bool bCurrentIsStreamSeekable;

}DecoderManager;


// Used to store engine thread data
typedef struct tagDecoderManagerEngine
{
	// Pointer to DecoderManager struct
	DecoderManager* pDecoderManagerData;

	// Store Handle for Output Sector Free Event
	HANDLE hOutputWriteEvent;

	// Used to store input instances
	IStreamInput InputArray[DECODER_DECODER_MAX];
	uint8_t uActiveInput;

	// Used to store output instances
	IStreamOutput OutputArray[DECODER_OUTPUT_MAX];
	uint8_t uActiveOutput;

	bool bIsFileOpen;
	bool bIsEndOfStream;
	bool bIsOutputPlaying;

	// Store Output Buffer Latency
	uint32_t uOutputMaxLatency;

	// Store Circle Buffer
	CircleBuffer* pCircleBuffer;

	// Store Spectrum buffer
	SpectrumBuffer *pSpectrumBuffer;

	// Store a pointer to DSP Callback
	DecoderManager_DSP_Callback pDspCallback;

} DecoderManagerEngine;


// Public Functions
bool DecoderManager_Initialize(DecoderManager* pDecoder, HWND hWindowHandle);
bool DecoderManager_Deinitialize(DecoderManager* pDecoder);
bool DecoderManager_OpenFile(DecoderManager* pDecoder, const wchar_t* pFilePath);
bool DecoderManager_CloseFile(DecoderManager* pDecoder);
bool DecoderManager_Play(DecoderManager* pDecoder);
bool DecoderManager_Pause(DecoderManager* pDecoder);
bool DecoderManager_UnPause(DecoderManager* pDecoder);
bool DecoderManager_Stop(DecoderManager* pDecoder);
bool DecoderManager_Position(DecoderManager* pDecoder);
bool DecoderManager_Seek(DecoderManager* pDecoder, uint64_t uMsNewPosition);
bool DecoderManager_SetVolume(DecoderManager* pDecoder, uint8_t *uVolumeValue);
bool DecoderManager_GetVolume(DecoderManager* pDecoder, uint8_t *uVolumeValue);
bool DecoderManage_GetPlayingBuffer(DecoderManager* pDecoder, int8_t* pByteBuffer, uint16_t* puBufferLen);
bool DecoderManager_AssignDSPCallback(DecoderManager* pDecoder, DecoderManager_DSP_Callback pCallback);





#endif
