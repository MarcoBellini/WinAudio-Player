#include "pch.h"
#include "WA_Guids.h"
#include "WA_Macro.h"

// Export Function Name
#define EXPORTS _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);


bool WA_Wasapi_New(WA_Output* This);
void WA_Wasapi_Delete(WA_Output* This);

uint32_t WA_Wasapi_Open(WA_Output* This, uint32_t* puBufferLatency);
void WA_Wasapi_Close(WA_Output* This);

uint32_t WA_Wasapi_Play(WA_Output* This);
uint32_t WA_Wasapi_Pause(WA_Output* This);
uint32_t WA_Wasapi_Resume(WA_Output* This);
uint32_t WA_Wasapi_Stop(WA_Output* This);

uint32_t WA_Wasapi_Seek(WA_Output* This, uint64_t uNewPositionMs);
uint32_t WA_Wasapi_Get_Position(WA_Output* This, uint64_t* uPositionMs);

uint32_t WA_Wasapi_Set_Volume(WA_Output* This, uint8_t uNewVolume);
uint32_t WA_Wasapi_Get_Volume(WA_Output* This, uint8_t* puVolume);

uint32_t WA_Wasapi_Get_BufferData(WA_Output* This, int8_t* pBuffer, uint32_t uBufferLen);

uint32_t WA_Wasapi_Process_DSP(WA_Output* This, bool bEnable);

void WA_Wasapi_ConfigDialog(WA_Input* This, HWND hParent);

DWORD WINAPI WA_Wasapi_WriteThread(LPVOID lpParam);

static WA_Output WA_Wasapi_Table =
{
	{ // Start Common Header ---------------> 
	WA_PLUGINTYPE_OUTPUT,	// Plugin Type
	1U,						// Version
	L"WinAudio Wasapi Output\0", // Description
	NULL,					// WinAudio HWND
	false					// Enabled or Disabled
	}, // End Common Header <-----------------
	NULL, // Input 
	NULL, // Effect
	WA_Wasapi_New,
	WA_Wasapi_Delete,
	WA_Wasapi_Open,
	WA_Wasapi_Close,
	WA_Wasapi_Play,
	WA_Wasapi_Pause,
	WA_Wasapi_Resume,
	WA_Wasapi_Stop,
	WA_Wasapi_Seek,
	WA_Wasapi_Get_Position,
	WA_Wasapi_Set_Volume,
	WA_Wasapi_Get_Volume,
	WA_Wasapi_Get_BufferData,
	WA_Wasapi_Process_DSP,
	WA_Wasapi_ConfigDialog,
	NULL
};


// Store Instance Data
typedef struct TagWA_WasapiInstance
{
	// Warning: This must be the first item of the struct
	// because it's address is used to orbitain the address of
	// OutWasapiInstance address
	IMMNotificationClient MMNotificationClient;
	LONG ReferenceCounter;

	// Wasapi Interfaces
	IMMDeviceEnumerator* pDeviceEnumerator;
	IAudioClient* pAudioClient;
	IMMDevice* pIMMDevice;
	IAudioRenderClient* pRenderClient;
	ISimpleAudioVolume* pSimpleVolume;
	IAudioClock* pAudioClock;
	HANDLE hWriteEvent;
	HANDLE hAbortEvent;
	HANDLE hSeekEvent;
	HANDLE hSyncEvent;

	// Input Stream WaveFormat (Use AUTOCONVERT_PCM for now)
	WAVEFORMATEXTENSIBLE StreamWfx;

	// Store some output informations
	bool bDeviceIsOpen;
	bool bWasapiIsPlaying;
	float fWrittenTimeMs;

	// Store Opened Latency
	uint32_t uCurrentLatency;
	bool bPendingStreamSwitch;

	// Store High Resolution Performance Counter
	LARGE_INTEGER uPCFrequency;

	HANDLE hWriteThread;
	bool bProcessDSP;

} WA_WasapiInstance;

static STDMETHODIMP WA_Wasapi_QueryInterface(IMMNotificationClient* This, REFIID riid, _COM_Outptr_ void** ppvObject)
{

	if (ppvObject == NULL)
	{
		return E_POINTER;
	}

	if (IsEqualIID(riid, &IID_IMMNotificationClient) || IsEqualIID(riid, &IID_IUnknown))
	{
		*ppvObject = This;
		This->lpVtbl->AddRef(This);
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

static STDMETHODIMP_(ULONG) WA_Wasapi_AddRef(IMMNotificationClient* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This;

	return InterlockedIncrement(&pInstance->ReferenceCounter);
}

static STDMETHODIMP_(ULONG) WA_Wasapi_Release(IMMNotificationClient* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This;

	return InterlockedDecrement(&pInstance->ReferenceCounter);
}

static STDMETHODIMP WA_Wasapi_OnDeviceStateChanged(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId, _In_  DWORD dwNewState)
{
	return S_OK;
}

static STDMETHODIMP WA_Wasapi_OnDeviceAdded(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

static STDMETHODIMP WA_Wasapi_OnDeviceRemoved(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}


static STDMETHODIMP WA_Wasapi_OnDefaultDeviceChanged(IMMNotificationClient* This, _In_ EDataFlow Flow, _In_ ERole Role, _In_ LPCWSTR NewDefaultDeviceId)
{

	// Perform Stream Switch on Output Device
	if ((Flow == eRender) && (Role == eMultimedia))
	{
		WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This;		
		pInstance->bPendingStreamSwitch = true;
	}

	return S_OK;
}

static STDMETHODIMP WA_Wasapi_OnPropertyValueChanged(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId, _In_  const PROPERTYKEY key)
{
	return S_OK;
}


static struct IMMNotificationClientVtbl OutWasapi_VTable =
{
	WA_Wasapi_QueryInterface,
	WA_Wasapi_AddRef,
	WA_Wasapi_Release,
	WA_Wasapi_OnDeviceStateChanged,
	WA_Wasapi_OnDeviceAdded,
	WA_Wasapi_OnDeviceRemoved,
	WA_Wasapi_OnDefaultDeviceChanged,
	WA_Wasapi_OnPropertyValueChanged
};


WA_HMODULE* WA_Plugin_GetHeader(void)
{
	 return (WA_HMODULE*)(&WA_Wasapi_Table);
}

static bool WA_Wasapi_InitDefaultEndPoint(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;

	// Create Instance to Device Enumerator
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IMMDeviceEnumerator,
		(LPVOID)&pInstance->pDeviceEnumerator);

	if FAILED(hr)
		return false;

	// Get Default System Endpoint
	hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pInstance->pDeviceEnumerator,
		eRender,
		eMultimedia,
		&pInstance->pIMMDevice);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Create Audio Client
	hr = IMMDevice_Activate(pInstance->pIMMDevice,
		&IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(LPVOID*)&pInstance->pAudioClient);

	// Return on Fail
	if FAILED(hr)
		return false;


	return true;

}

static bool WA_Wasapi_InitAudioClient(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	PWAVEFORMATEXTENSIBLE pDeviceWfx;
	DWORD dwStreamFlags = 0;
	REFERENCE_TIME nBufferLatency;
	HRESULT hr;

	// Get Audio Engine Mix Format (for Shared Mode)
	hr = IAudioClient_GetMixFormat(pInstance->pAudioClient,
		(LPWAVEFORMATEX*)&pDeviceWfx);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Check if need to resample output
	if ((pInstance->StreamWfx.Format.nSamplesPerSec == pDeviceWfx->Format.nSamplesPerSec) &&
		(pInstance->StreamWfx.Format.nChannels == pDeviceWfx->Format.nChannels) &&
		(pInstance->StreamWfx.Format.wBitsPerSample == pDeviceWfx->Format.wBitsPerSample))
	{
		dwStreamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	}
	else
	{
		// Need to Resample Input according to output					
		dwStreamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
			AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
			AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

		// TODO: In Future Create a DMO Audio Resampler Session
		// Now use auto conversion to develop the player
	}

	// Free Device WFX
	CoTaskMemFree((LPVOID)pDeviceWfx);


	// 1 REFERENCE_TIME = 100 ns	
	nBufferLatency = 10000 * (REFERENCE_TIME)pInstance->uCurrentLatency;

	// Initialize Wasapi Device (In Shared Mode)
	hr = IAudioClient_Initialize(pInstance->pAudioClient,
		AUDCLNT_SHAREMODE_SHARED,
		dwStreamFlags,
		nBufferLatency,
		0,
		(LPWAVEFORMATEX)&pInstance->StreamWfx,
		NULL);

	// Return on Fail
	if FAILED(hr)
		return false;


	// Set Refill event handle
	hr = IAudioClient_SetEventHandle(pInstance->pAudioClient,
		pInstance->hWriteEvent);

	// Return on Fail
	if FAILED(hr)
		return false;

	return true;
}

static bool WA_Wasapi_InitAudioServices(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;


	// Audio Render Client (used to write input buffer)
	hr = IAudioClient_GetService(pInstance->pAudioClient,
		&IID_IAudioRenderClient,
		(LPVOID*)&pInstance->pRenderClient);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Simple Volume Control
	hr = IAudioClient_GetService(pInstance->pAudioClient,
		&IID_ISimpleAudioVolume,
		(LPVOID*)&pInstance->pSimpleVolume);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Audio Clock (used to retive the position in the buffer)
	hr = IAudioClient_GetService(pInstance->pAudioClient,
		&IID_IAudioClock,
		(LPVOID*)&pInstance->pAudioClock);

	// Return on Fail
	if FAILED(hr)
		return false;

	return true;
}

static bool WA_Wasapi_CleanResources(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	// Check for a valid pointer and release
	if (pInstance->pAudioClock)
		IAudioClock_Release(pInstance->pAudioClock);

	if (pInstance->pSimpleVolume)
		ISimpleAudioVolume_Release(pInstance->pSimpleVolume);

	if (pInstance->pRenderClient)
		IAudioRenderClient_Release(pInstance->pRenderClient);

	if (pInstance->pAudioClient)
		IAudioClient_Release(pInstance->pAudioClient);

	if (pInstance->pIMMDevice)
		IMMDevice_Release(pInstance->pIMMDevice);

	if (pInstance->pDeviceEnumerator)
		IMMDeviceEnumerator_Release(pInstance->pDeviceEnumerator);
	

	// Reset Pointers
	pInstance->pAudioClient = NULL;
	pInstance->pIMMDevice = NULL;
	pInstance->pRenderClient = NULL;
	pInstance->pSimpleVolume = NULL;
	pInstance->pAudioClock = NULL;
	pInstance->pDeviceEnumerator = NULL;

	return true;
}


bool WA_Wasapi_New(WA_Output* This)
{
	WA_WasapiInstance* pInstance = NULL;

	pInstance = (WA_WasapiInstance*) malloc(sizeof(WA_WasapiInstance));

	if (!pInstance)
		return false;

	// Assign VTable
	pInstance->MMNotificationClient.lpVtbl = &OutWasapi_VTable;

	// Reset values
	pInstance->bDeviceIsOpen = false;
	pInstance->bWasapiIsPlaying = false;
	pInstance->pAudioClient = NULL;
	pInstance->pIMMDevice = NULL;
	pInstance->pRenderClient = NULL;
	pInstance->pSimpleVolume = NULL;
	pInstance->pAudioClock = NULL;
	pInstance->pDeviceEnumerator = NULL;
	pInstance->fWrittenTimeMs = 0.0f;
	pInstance->uCurrentLatency = 0;
	pInstance->ReferenceCounter = 0;

	// Event-Driven Wasapi
	pInstance->hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hAbortEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hSeekEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hSyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Fail on NULL value
	if (!pInstance->hWriteEvent)
	{
		free(pInstance);
		return false;
	}


	// Cache QPC Frequency (0 = High Resoluction Not Supported)
	if (!QueryPerformanceFrequency(&pInstance->uPCFrequency))
		pInstance->uPCFrequency.QuadPart = 0;


	This->hPluginData = (HCOOKIE)pInstance;

	return true;

}

void WA_Wasapi_Delete(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*) This->hPluginData;


	if (pInstance)
	{
		// Free memory allocated in Initialize function
		if (pInstance->bDeviceIsOpen)
			WA_Wasapi_Close(This);

		CloseHandle(pInstance->hWriteEvent);
		pInstance->hWriteEvent = NULL;

		CloseHandle(pInstance->hSeekEvent);
		pInstance->hSeekEvent = NULL;

		CloseHandle(pInstance->hAbortEvent);
		pInstance->hAbortEvent = NULL;

		CloseHandle(pInstance->hSyncEvent);
		pInstance->hSyncEvent = NULL;

		free(pInstance);
		pInstance = NULL;
	}
}

uint32_t WA_Wasapi_Open(WA_Output* This, uint32_t* puBufferLatency)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	PWAVEFORMATEXTENSIBLE pWfx;
	REFERENCE_TIME nBufferLatency;
	HRESULT hr;
	WA_Input* pIn;
	WA_AudioFormat WAFormat;

	if (!This->pIn)
		return WA_ERROR_INPUTOUTPUTNOTFOUND;

	if (pInstance->bDeviceIsOpen)
		WA_Wasapi_Close(This);


	pIn = This->pIn;

	if (pIn->WA_Input_GetFormat(pIn, &WAFormat) != WA_OK)
		return WA_ERROR_BADFORMAT;


	// Get pointer from Instace
	pWfx = &pInstance->StreamWfx;


	// Create Input Stream WaveFormat and Store Locally
	pWfx->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	pWfx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	pWfx->SubFormat = WASAPI_KSDATAFORMAT_SUBTYPE_PCM;
	pWfx->Format.nSamplesPerSec = (DWORD)WAFormat.uSamplerate;

	// Use Channel mask from input plugin only if channels > 2
	if (WAFormat.uChannels <= 2)
	{
		pWfx->dwChannelMask = (WAFormat.uChannels == 1) ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO;
	}
	else
	{
		pWfx->dwChannelMask = (DWORD)WAFormat.dwChannelMask;
	}

	
	
	pWfx->Format.nChannels = (WORD)WAFormat.uChannels;
	pWfx->Format.wBitsPerSample = (WORD)WAFormat.uBitsPerSample;
	pWfx->Format.nBlockAlign = (WORD)WAFormat.uBlockAlign;
	pWfx->Format.nAvgBytesPerSec = (DWORD)WAFormat.uAvgBytesPerSec;
	pWfx->Samples.wValidBitsPerSample = (WORD)WAFormat.uBitsPerSample;

	// TODO: Use Custom Latency
	pInstance->uCurrentLatency = WA_WASAPI_DEFAULT_LATENCY_MS;

	if (!WA_Wasapi_InitDefaultEndPoint(This))
	{
		WA_Wasapi_CleanResources(This);
		return WA_ERROR_FAIL;
	}

	if (!WA_Wasapi_InitAudioClient(This))
	{
		WA_Wasapi_CleanResources(This);
		return WA_ERROR_FAIL;
	}

	if (!WA_Wasapi_InitAudioServices(This))
	{
		WA_Wasapi_CleanResources(This);
		return WA_ERROR_FAIL;
	}

	// Get Current System Latency
	hr = IAudioClient_GetStreamLatency(pInstance->pAudioClient,
		&nBufferLatency);

	// Check if we have a valid value and Update With Real Device Latency
	if (SUCCEEDED(hr) && (nBufferLatency != 0))
	{

		pInstance->uCurrentLatency = (uint32_t)(nBufferLatency / 10000);

		if (puBufferLatency)
			(*puBufferLatency) = pInstance->uCurrentLatency;

	}


	// Register Callbacks
	pInstance->MMNotificationClient.lpVtbl->AddRef(&pInstance->MMNotificationClient);

	hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	if FAILED(hr)
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}


	pInstance->bDeviceIsOpen = true;
	pInstance->bWasapiIsPlaying = false;
	pInstance->bPendingStreamSwitch = false;	


	return WA_OK;


}
void WA_Wasapi_Close(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;

	if (!pInstance->bDeviceIsOpen)
		return;

	// Stop Before Exit
	if (pInstance->bWasapiIsPlaying)
		This->WA_Output_Stop(This);

	// Unregister Callbacks
	hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	pInstance->MMNotificationClient.lpVtbl->Release(&pInstance->MMNotificationClient);

	WA_Wasapi_CleanResources(This);

	pInstance->bDeviceIsOpen = false;
	pInstance->bWasapiIsPlaying = false;
	pInstance->bPendingStreamSwitch = false;

}

// TODO: 08_12-22 Strutturare meglio la gestione errori su Play,Pausa e stop
uint32_t WA_Wasapi_Play(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	DWORD dwResult;
	uint32_t uResult;
	HRESULT hr;
	uint8_t uVolume;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	// Fill device buffer with PCM data
	uResult = WA_Wasapi_FeedOutput(This);

	if (uResult != WA_WASAPI_OK)
		return WA_ERROR_OUTPUTNOTREADY;

	// Create Write Thread and write PCM audio data into buffer (Event - Driven mode)
	pInstance->hWriteThread = CreateThread(NULL,
		0,
		WA_Wasapi_WriteThread,
		This,
		0,
		NULL);

	if (!pInstance->hWriteThread)
		return WA_ERROR_FAIL;

	// Wait Write Thread is Ready
	dwResult = WaitForSingleObject(pInstance->hSyncEvent, 10000);

	// Close Write Thread on Fail and return and error
	if (dwResult != WAIT_OBJECT_0)
	{
		SetEvent(pInstance->hAbortEvent);
		WaitForSingleObject(pInstance->hSyncEvent, INFINITE);

		CloseHandle(pInstance->hWriteThread);

		return WA_ERROR_OUTPUTNOTREADY;
	}

	// Get Current Volume
	uVolume = SendMessage(This->Header.hMainWindow, WM_WA_MSG, MSG_GETVOLUME, 0);
	WA_Wasapi_Set_Volume(This, uVolume);

	// Play
	hr = IAudioClient_Start(pInstance->pAudioClient);

	if FAILED(hr)
	{
		SetEvent(pInstance->hAbortEvent);
		WaitForSingleObject(pInstance->hSyncEvent, INFINITE);

		CloseHandle(pInstance->hWriteThread);		
		return WA_ERROR_OUTPUTNOTREADY;
	}

	pInstance->bWasapiIsPlaying = true;

	return WA_OK;
}

uint32_t WA_Wasapi_Pause(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if(!pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;

	IAudioClient_Stop(pInstance->pAudioClient);
	pInstance->bWasapiIsPlaying = false;
}
uint32_t WA_Wasapi_Resume(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;

	IAudioClient_Start(pInstance->pAudioClient);
	pInstance->bWasapiIsPlaying = true;
}


uint32_t WA_Wasapi_Stop(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	IAudioClient_Stop(pInstance->pAudioClient);
	pInstance->bWasapiIsPlaying = false;

	// Close Write Thread
	SetEvent(pInstance->hAbortEvent);
	WaitForSingleObject(pInstance->hSyncEvent, INFINITE);

	CloseHandle(pInstance->hWriteThread);

	IAudioClient_Reset(pInstance->pAudioClient);

	return WA_OK;

}

uint32_t WA_Wasapi_Seek(WA_Output* This, uint64_t uNewPositionMs)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	WA_Input* pIn;
	uint32_t uResult;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (!pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;

	pIn = This->pIn;

	// Seek to a position we can handle
	if (uNewPositionMs >= (pIn->WA_Input_Duration(pIn) - pInstance->uCurrentLatency))
		return WA_ERROR_BADPARAM;


	// Stop Playback
	IAudioClient_Stop(pInstance->pAudioClient);
	IAudioClient_Reset(pInstance->pAudioClient);

	pInstance->bWasapiIsPlaying = false;

	// Wait Output to be ready to a new write
	SetEvent(pInstance->hSeekEvent);
	WaitForSingleObject(pInstance->hSyncEvent, INFINITE);

	// Seek to a new position
	uResult = pIn->WA_Input_Seek(pIn, uNewPositionMs);

	WA_Wasapi_FeedOutput(This);
	IAudioClient_Start(pInstance->pAudioClient);

	pInstance->bWasapiIsPlaying = true;
		
	return uResult;

}
uint32_t WA_Wasapi_Get_Position(WA_Output* This, uint64_t* uPositionMs)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	WA_Input* pIn;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (!uPositionMs)
		return WA_ERROR_BADPTR;

	pIn = This->pIn;


	(*uPositionMs) = pIn->WA_Input_Position(pIn) - pInstance->uCurrentLatency;


	return WA_OK;
}

uint32_t WA_Wasapi_Set_Volume(WA_Output* This, uint8_t uNewVolume)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	FLOAT fVolumeValue;
	HRESULT hr;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	// Convert from uint8 to a Normalized float Value
	fVolumeValue = ((float)uNewVolume / UINT8_MAX);

	// Set the Master Volume: Range between 0.0 to 1.0
	hr = ISimpleAudioVolume_SetMasterVolume(pInstance->pSimpleVolume, fVolumeValue, NULL);

	return SUCCEEDED(hr) ? WA_OK : WA_ERROR_FAIL;
}
uint32_t WA_Wasapi_Get_Volume(WA_Output* This, uint8_t* puVolume)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	FLOAT fVolumeValue;
	HRESULT hr;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	// Get the Master Volume: Range between 0.0 to 1.0
	hr = ISimpleAudioVolume_GetMasterVolume(pInstance->pSimpleVolume, &fVolumeValue);

	// Convert from a float value to uint8
	(*puVolume) = (uint8_t)(fVolumeValue * UINT8_MAX);

	return SUCCEEDED(hr) ? WA_OK : WA_ERROR_FAIL;
}

uint32_t WA_Wasapi_Get_BufferData(WA_Output* This, int8_t* pBuffer, uint32_t uBufferLen)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;
}

uint32_t WA_Wasapi_Process_DSP(WA_Output* This, bool bEnable)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
}

void WA_Wasapi_ConfigDialog(WA_Input* This, HWND hParent)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
}


/// <summary>
/// Get the number of Bytes that can be written to the Device buffer
/// </summary>
/// <returns> A value >0 on success</returns>
static uint32_t WA_Wasapi_GetByteCanWrite(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	UINT32 uFrameBufferLength;
	UINT32 uFrameBufferPadding;
	HRESULT hr;

	// Check if device is open
	if (!pInstance->bDeviceIsOpen)
		return 0;

	// Get Buffer size in frames
	hr = IAudioClient_GetBufferSize(pInstance->pAudioClient, &uFrameBufferLength);

	if FAILED(hr)
		return 0;

	// Get written data in the buffer in frames
	hr = IAudioClient_GetCurrentPadding(pInstance->pAudioClient, &uFrameBufferPadding);

	if FAILED(hr)
		return 0;

	// Convert from frames to bytes
	return (uFrameBufferLength - uFrameBufferPadding) * pInstance->StreamWfx.Format.nBlockAlign;	
}


/// <summary>
/// Write a Buffer into a Device
/// </summary>
/// <param name="pBuffer">Pointer to PCM Buffer</param>
/// <param name="uByteToWrite">Number of Bytes to write</param>
/// <returns>True on success</returns>
bool WA_Wasapi_WriteToDevice(WA_Output* This, int8_t* pBuffer, uint32_t uByteToWrite)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	UINT32 uRequestedFrames;
	BYTE* pDeviceBuffer;
	HRESULT hr;
	errno_t nErr;

	if (!pInstance->bDeviceIsOpen) 
		return false;

	if(!pBuffer)
		return false;

	if(uByteToWrite == 0)
		return false;


	uRequestedFrames = (UINT32)(uByteToWrite / pInstance->StreamWfx.Format.nBlockAlign);

	// Get Device Buffer
	hr = IAudioRenderClient_GetBuffer(pInstance->pRenderClient,
		uRequestedFrames,
		&pDeviceBuffer);

	if FAILED(hr)
		return false;

	// Copy PCM data to Device Buffer
	nErr = memcpy_s(pDeviceBuffer, 
		uByteToWrite, 
		pBuffer, 
		uByteToWrite);

	if (nErr != 0)
	{
		// Fail to Write output buffer
		IAudioRenderClient_ReleaseBuffer(pInstance->pRenderClient, 0, 0);

		return false;
	}


	// Release Buffer if AUDCLNT_BUFFERFLAGS_SILENT it consider buffer as silent
	hr = IAudioRenderClient_ReleaseBuffer(pInstance->pRenderClient,
		uRequestedFrames,
		0);

	if FAILED(hr)
		return false;
	

	return true;
}

/// <summary>
/// Read PCM Data From Input, Process DSP (If Enabled) and finally write to device. 
/// Automatically allocates buffer with a required size to fill the device buffer.
/// Detect Buffer Underrun. 
/// </summary>
/// <returns>WA_WASAPI_OK on success otherwise an error code. See <see cref="WA_Macro.h">WA_Macro.h</see> </returns>
static uint32_t WA_Wasapi_FeedOutput(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	uint32_t uBytesToWrite, uVaildBytes;
	int8_t* pBuffer;
	WA_Input* pIn = This->pIn;
	WA_Effect* pEffect = This->pEffect;
	uint32_t uResult;
	bool bResult;
	DWORD dwStartMs, dwEndMs, dwElapsedMs, dwValidDataInDeviceMs;

	uBytesToWrite = WA_Wasapi_GetByteCanWrite(This);

	if (uBytesToWrite == 0)
		return WA_WASAPI_NOBYTESTOWRITE;

	pBuffer = malloc(uBytesToWrite);

	if (!pBuffer)
		return WA_WASAPI_MALLOCERROR;

	dwStartMs = GetTickCount();

	uResult = pIn->WA_Input_Read(pIn, pBuffer, uBytesToWrite, &uVaildBytes);

	// Process DSP only if Enabled
	if ((pEffect) && (pInstance->bProcessDSP))
	{
		pEffect->WA_Effect_Process(pEffect, pBuffer, uVaildBytes, &uVaildBytes);
	}

	dwEndMs = GetTickCount();
	
	// See https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount
	// The elapsed time is stored as a DWORD value. Therefore, the time will wrap around to zero if 
	// the system is run continuously for 49.7 days
	if (dwEndMs < dwStartMs)
		dwElapsedMs = dwStartMs - dwEndMs;
	else
		dwElapsedMs = dwEndMs - dwStartMs;

	dwValidDataInDeviceMs = pInstance->uCurrentLatency - ((uVaildBytes * 1000) / pInstance->StreamWfx.Format.nAvgBytesPerSec);

	// Detect End of File
	if ((uVaildBytes == 0) || (uResult == WA_ERROR_ENDOFFILE))
	{
		free(pBuffer);
		return WA_WASAPI_ENDOFFILE;
	}
	
	bResult = WA_Wasapi_WriteToDevice(This, pBuffer, uVaildBytes);
	free(pBuffer);

	// Detect a Buffer Underrun
	if (dwValidDataInDeviceMs < dwElapsedMs)
	{
		_RPTW0(_CRT_WARN, L"WASAPI: Buffer Underrun\n");
		return WA_WASAPI_BUFFERUNDERRUN;
	}
	

	return (bResult == true ? WA_WASAPI_OK : WA_WASAPI_DEVICEWRITEERROR);
}


/// <summary>
/// Write Thread
/// </summary>
/// <param name="lpParam">Pointer to Instance Struct</param>
/// <returns>0 on success</returns>
DWORD WINAPI WA_Wasapi_WriteThread(LPVOID lpParam)
{
	HANDLE hEvents[3];
	WA_Output* This = (WA_Output*) lpParam;
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	DWORD dwEvent;
	bool bContinue = true;
	uint32_t uResult;
	HRESULT hr;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);


	hEvents[0] = pInstance->hAbortEvent;
	hEvents[1] = pInstance->hSeekEvent;
	hEvents[2] = pInstance->hWriteEvent;

	// Notify we are ready to write
	SetEvent(pInstance->hSyncEvent);

	while (bContinue)
	{
		dwEvent = WaitForMultipleObjects(3, hEvents, FALSE, 100);


		switch (dwEvent)
		{
		case (WAIT_OBJECT_0 + 0): // Abort
			bContinue = false;
			SetEvent(pInstance->hSyncEvent);
			break;
		case (WAIT_OBJECT_0 + 1): // Seek
			SetEvent(pInstance->hSyncEvent);
			break;
		case (WAIT_OBJECT_0 + 2): // Write		
		
			uResult = WA_Wasapi_FeedOutput(lpParam);

			if (uResult == WA_WASAPI_ENDOFFILE)
			{
				// TODO: Notify End of Stream
				bContinue = false;
			}			

			break;
		case WAIT_TIMEOUT:
			Sleep(10);
			break;
		default: 
			//TODO: Fail -> Send stop message to manin window to close playback and send error
			bContinue = false;		
			_RPTW1(_CRT_WARN, L"Errore: %d \n", GetLastError());
		}
	}

	CoUninitialize();

	return EXIT_SUCCESS;
}