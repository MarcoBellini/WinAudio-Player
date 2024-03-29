#include "pch.h"
#include "WA_Guids.h"
#include "WA_Macro.h"
#include "WA_CircleBuffer.h"

// Export Function Name
#define EXPORTS _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);

// Exported Functions
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

uint32_t WA_Wasapi_Enable_DSP(WA_Output* This, bool bEnable);
void WA_Wasapi_ConfigDialog(WA_Output* This, HWND hParent);


// Local Functions
DWORD WINAPI WA_Wasapi_WriteThread(LPVOID lpParam);
static uint32_t WA_Wasapi_FeedOutput(WA_Output* This);

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
	WA_Wasapi_Enable_DSP,
	WA_Wasapi_ConfigDialog,
	NULL
};


// Store Instance Data
typedef struct TagWA_WasapiInstance
{
	// Warning: This must be the first item of the struct
	// because it's address is used to orbitain the address of
	// WA_WasapiInstance address
	IMMNotificationClient MMNotificationClient;
	LONG ReferenceCounter;

	// Wasapi Interfaces
	IMMDeviceEnumerator* pDeviceEnumerator;
	IAudioClient* pAudioClient;
	IMMDevice* pIMMDevice;
	IAudioRenderClient* pRenderClient;
	ISimpleAudioVolume* pSimpleVolume;
	IAudioClock* pAudioClock;

	// Multithread Events
	HANDLE hWriteEvent;
	HANDLE hAbortEvent;
	HANDLE hSeekEvent;
	HANDLE hSyncEvent;
	HANDLE hStreamSwitchEvent;

	HANDLE hWriteThread;

	// Input Stream WaveFormat (Use AUTOCONVERT_PCM for now)
	WAVEFORMATEXTENSIBLE StreamWfx;

	// Store some output informations
	bool bDeviceIsOpen;
	bool bWasapiIsPlaying;
	bool bEndOfStreamHasStoppedPlayback;
	bool bEnableDSP;
	bool bPendingEndOfStream;

	// Store Opened Latency
	uint32_t uCurrentLatencyMs;

	CRITICAL_SECTION CriticalSection;
	WA_CircleBuffer* pCircle;
	

#if WA_WASAPI_USE_HIGH_RES_TIMER
	// Store High Resolution Performance Counter
	LARGE_INTEGER uPCFrequency;
#endif

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
	
		// Notify Stream switch only if device is open (See play function)
		if(pInstance->bDeviceIsOpen)
			SetEvent(pInstance->hStreamSwitchEvent);				
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


EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void)
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
	}

	// Free Device WFX
	CoTaskMemFree((LPVOID)pDeviceWfx);


	// 1 REFERENCE_TIME = 100 ns	
	nBufferLatency = 10000 * (REFERENCE_TIME)pInstance->uCurrentLatencyMs;

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

/// <summary>
/// Try to close Write Thread and Call CloseHandle to free memory opened
/// with CreateThread function
/// </summary>
/// <param name="This">Current Instance</param>
/// <param name="bNeedToSignalAbort">If True Signal hAbortEvent and wait before close</param>
static inline void WA_Wasapi_ReleaseWriteThread(WA_Output* This, bool bNeedToSignalAbort)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	if (bNeedToSignalAbort)
	{
		DWORD dwResult;

		if (SetEvent(pInstance->hAbortEvent))
		{
			dwResult = WaitForSingleObject(pInstance->hSyncEvent, WA_WASAPI_MAX_WAIT_TIME_MS);

			if (dwResult != WAIT_OBJECT_0)
			{
				BOOL bResult;

				/*
				* WARNING!!
					This function should never be called, but kill the write 
					thread if necessary and report the serious event 
					in the debug information.
				*/
				bResult = TerminateThread(pInstance->hWriteThread, ERROR_POSSIBLE_DEADLOCK);

				_ASSERT(bResult > 0);
				_RPTW0(_CRT_WARN, L"Call TerminateThread. Something went wrong\n");
			}
		
			
		}
		
	}

	if (pInstance->hWriteThread)
	{
		CloseHandle(pInstance->hWriteThread);
		pInstance->hWriteThread = NULL;
	}
}

/// <summary>
/// Perform a stream switch when user change output Device
/// </summary>
/// <returns>True on Success or False on Fail</returns>
static bool WA_Wasapi_PerformStreamSwitch(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	bool bIsPlaying, bResult;
	HRESULT hr;
	uint32_t uCircleBufferSize, uResult;
	uint8_t uVolume;
	REFERENCE_TIME nBufferLatency;

	if (!pInstance->bDeviceIsOpen)
		return false;
		
	EnterCriticalSection(&pInstance->CriticalSection);
	bIsPlaying = pInstance->bWasapiIsPlaying;
	LeaveCriticalSection(&pInstance->CriticalSection);

	if (bIsPlaying)
	{
		hr = IAudioClient_Stop(pInstance->pAudioClient);

		if FAILED(hr)
			return false;
	
		EnterCriticalSection(&pInstance->CriticalSection);
		pInstance->bWasapiIsPlaying = false;
		LeaveCriticalSection(&pInstance->CriticalSection);	
	}

	// Clear Circle Buffer
	if (pInstance->pCircle)
		WA_CircleBuffer_Delete(pInstance->pCircle);

	pInstance->pCircle = NULL;

	
	// Unregister Callbacks
	hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	IMMNotificationClient_Release(&pInstance->MMNotificationClient);


	bResult = WA_Wasapi_CleanResources(This);

	if (!bResult)
		return false;

	pInstance->uCurrentLatencyMs = WA_WASAPI_DEFAULT_LATENCY_MS;

	if (!WA_Wasapi_InitDefaultEndPoint(This))
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}

	if (!WA_Wasapi_InitAudioClient(This))
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}

	if (!WA_Wasapi_InitAudioServices(This))
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}

	// Get Current System Latency
	hr = IAudioClient_GetStreamLatency(pInstance->pAudioClient,
		&nBufferLatency);

	// Check if we have a valid value and Update With Real Device Latency
	if (SUCCEEDED(hr) && (nBufferLatency != 0))
		pInstance->uCurrentLatencyMs = (uint32_t)(nBufferLatency / 10000U);

	
	// Register Callbacks(For Stream Switch Event)
	IMMNotificationClient_AddRef(&pInstance->MMNotificationClient);

	hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	if FAILED(hr)
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}	

	// Create Cicle Buffer(PCM Aligned)
	uCircleBufferSize = (pInstance->uCurrentLatencyMs * pInstance->StreamWfx.Format.nAvgBytesPerSec) / 1000U;
	uCircleBufferSize = uCircleBufferSize - (uCircleBufferSize % pInstance->StreamWfx.Format.nBlockAlign);


	pInstance->pCircle = WA_CircleBuffer_New(uCircleBufferSize);

	if (bIsPlaying)
	{
		// Fill device buffer with PCM data
		uResult = WA_Wasapi_FeedOutput(This);

		if(uResult != WA_WASAPI_OK)
		{
			WA_Wasapi_CleanResources(This);

			// Clear Circle Buffer
			if (pInstance->pCircle)
				WA_CircleBuffer_Delete(pInstance->pCircle);

			pInstance->pCircle = NULL;

			return false;
		}

		// Get Current Volume from WinAudio UI Player
		uVolume = (uint8_t)SendMessage(This->Header.hMainWindow, WM_WA_MSG, MSG_GETVOLUME, 0);
		WA_Wasapi_Set_Volume(This, uVolume);

		hr = IAudioClient_Start(pInstance->pAudioClient);

		if SUCCEEDED(hr)
		{
			EnterCriticalSection(&pInstance->CriticalSection);
			pInstance->bWasapiIsPlaying = true;
			LeaveCriticalSection(&pInstance->CriticalSection);
		}
	}

	return true;
}

/// <summary>
/// Create a new instance of the plugin
/// </summary>
/// <returns>Return true on success</returns>
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
	pInstance->uCurrentLatencyMs = 0U;
	pInstance->ReferenceCounter = 0U;

	// Event-Driven Wasapi
	pInstance->hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hAbortEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hSeekEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hSyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pInstance->hStreamSwitchEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Fail on NULL value. Free resources and return False
	if ((!pInstance->hWriteEvent) || 
		(!pInstance->hAbortEvent) ||
		(!pInstance->hSeekEvent) ||
		(!pInstance->hSyncEvent) ||
		(!pInstance->hStreamSwitchEvent))
	{

		if(pInstance->hWriteEvent)
			CloseHandle(pInstance->hWriteEvent);

		if (pInstance->hAbortEvent)
			CloseHandle(pInstance->hAbortEvent);

		if (pInstance->hSeekEvent)
			CloseHandle(pInstance->hSeekEvent);

		if (pInstance->hSyncEvent)
			CloseHandle(pInstance->hSyncEvent);

		if (pInstance->hStreamSwitchEvent)
			CloseHandle(pInstance->hStreamSwitchEvent);

		pInstance->hWriteEvent = NULL;
		pInstance->hAbortEvent = NULL;
		pInstance->hSeekEvent = NULL;
		pInstance->hSyncEvent = NULL;
		pInstance->hStreamSwitchEvent = NULL;

		free(pInstance);
		return false;
	}

#if WA_WASAPI_USE_HIGH_RES_TIMER
	// Cache QPC Frequency (0 = High Resoluction Not Supported)
	if (!QueryPerformanceFrequency(&pInstance->uPCFrequency))
		pInstance->uPCFrequency.QuadPart = 0;
#endif


	This->hPluginData = (HCOOKIE)pInstance;

	return true;

}

/// <summary>
/// Close Plugin Instance
/// </summary>
void WA_Wasapi_Delete(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*) This->hPluginData;


	if (pInstance)
	{
		// Free memory allocated in Initialize function
		if (pInstance->bDeviceIsOpen)
			WA_Wasapi_Close(This);

		CloseHandle(pInstance->hWriteEvent);
		CloseHandle(pInstance->hSeekEvent);
		CloseHandle(pInstance->hAbortEvent);
		CloseHandle(pInstance->hSyncEvent);		
		CloseHandle(pInstance->hStreamSwitchEvent);

		pInstance->hWriteEvent = NULL;
		pInstance->hAbortEvent = NULL;
		pInstance->hSeekEvent = NULL;
		pInstance->hSyncEvent = NULL;
		pInstance->hStreamSwitchEvent = NULL;

		free(pInstance);
		pInstance = NULL;
	}
}

/// <summary>
/// Open a new Stream.Remember to assign a value to the pointer variables 
/// pIn and pEffect(See the header for more information)
/// </summary>
/// <param name="puBufferLatency">Pointer to uint32_t var to store current buffer latency, may change on Steam Switch</param>
/// <returns>WA_OK on success Otherwise an error code</returns>
uint32_t WA_Wasapi_Open(WA_Output* This, uint32_t* puBufferLatency)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	PWAVEFORMATEXTENSIBLE pWfx;
	REFERENCE_TIME nBufferLatency;
	HRESULT hr;
	WA_Input* pIn;
	WA_AudioFormat WAFormat;
	uint32_t uCircleBufferSize;

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
		pWfx->dwChannelMask = (WAFormat.uChannels == 1U) ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO;
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

	// Set latency before initialize wasapi
	pInstance->uCurrentLatencyMs = WA_WASAPI_DEFAULT_LATENCY_MS;

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
		pInstance->uCurrentLatencyMs = (uint32_t)(nBufferLatency / 10000U);


	if (puBufferLatency)
		(*puBufferLatency) = pInstance->uCurrentLatencyMs;


	// Register Callbacks(For Stream Switch Event)
	IMMNotificationClient_AddRef(&pInstance->MMNotificationClient);

	hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	if FAILED(hr)
	{
		WA_Wasapi_CleanResources(This);
		return false;
	}

	// Create Cicle Buffer(PCM Aligned)
	uCircleBufferSize = (pInstance->uCurrentLatencyMs * pWfx->Format.nAvgBytesPerSec) / 1000U;
	uCircleBufferSize = uCircleBufferSize - (uCircleBufferSize % pWfx->Format.nBlockAlign);


	pInstance->pCircle = WA_CircleBuffer_New(uCircleBufferSize);

	// Create Critical Section
	InitializeCriticalSection(&pInstance->CriticalSection);

	pInstance->bDeviceIsOpen = true;
	pInstance->bWasapiIsPlaying = false;
	pInstance->bPendingEndOfStream = false;	
	pInstance->bEndOfStreamHasStoppedPlayback = false;
	pInstance->hWriteThread = NULL;

	return WA_OK;


}

/// <summary>
/// Close the stream previously opened with the Open function.
/// </summary>
void WA_Wasapi_Close(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;

	if (!pInstance->bDeviceIsOpen)
		return;

	// Stop Before Exit (On End of Stream is Already Stopped)
	WA_Wasapi_Stop(This);

	// Unregister Callbacks
	hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(pInstance->pDeviceEnumerator,
		&pInstance->MMNotificationClient);

	IMMNotificationClient_Release(&pInstance->MMNotificationClient);

	// Close All Wasapi Interfaces
	WA_Wasapi_CleanResources(This);

	// Delete Critical Section
	DeleteCriticalSection(&pInstance->CriticalSection);

	// Check if Thread Handle is to close after an End of Stream
	WA_Wasapi_ReleaseWriteThread(This, false);

	// Delete Circle Buffer
	if (pInstance->pCircle)
		WA_CircleBuffer_Delete(pInstance->pCircle);

	pInstance->bDeviceIsOpen = false;
	pInstance->bWasapiIsPlaying = false;
	pInstance->bPendingEndOfStream = false;
	pInstance->bEndOfStreamHasStoppedPlayback = false;
}

/// <summary>
/// Play the open stream. Start the Write Thread, update the 
/// current position and write to the Buffer from which to read 
/// the data to the speaker.
/// </summary>
/// <returns>WA_OK on success</returns>
uint32_t WA_Wasapi_Play(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	DWORD dwResult;
	uint32_t uResult;
	HRESULT hr;
	uint8_t uVolume;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;


	// Perform a stream switch if there is one pending on Device Open
	if (WaitForSingleObject(pInstance->hStreamSwitchEvent, 0U) == WAIT_OBJECT_0)
	{
		if (!WA_Wasapi_PerformStreamSwitch(This))
			return WA_ERROR_OUTPUTNOTREADY;
	}

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

	// Wait Until Write Thread is Ready or a timeout occour
	dwResult = WaitForSingleObject(pInstance->hSyncEvent, WA_WASAPI_MAX_WAIT_TIME_MS);

	// Try to Close Write Thread on Fail and return and error
	if (dwResult != WAIT_OBJECT_0)
	{
		WA_Wasapi_ReleaseWriteThread(This, true);
		return WA_ERROR_OUTPUTTHREADNOTREADY;
	}

	// Get Current Volume from WinAudio UI Player
	uVolume = (uint8_t) SendMessage(This->Header.hMainWindow, WM_WA_MSG, MSG_GETVOLUME, 0);
	WA_Wasapi_Set_Volume(This, uVolume);

	// Play
	hr = IAudioClient_Start(pInstance->pAudioClient);

	if FAILED(hr)
	{
		WA_Wasapi_ReleaseWriteThread(This, true);
		return WA_ERROR_OUTPUTNOTREADY;
	}

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = true;
	LeaveCriticalSection(&pInstance->CriticalSection);

	pInstance->bEndOfStreamHasStoppedPlayback = false;
	pInstance->bPendingEndOfStream = false;
	
	
	return WA_OK;
}

/// <summary>
/// Pause the currently playing stream
/// </summary>
/// <returns>WA_OK on success</returns>
uint32_t WA_Wasapi_Pause(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if(!pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;

	hr = IAudioClient_Stop(pInstance->pAudioClient);

	if FAILED(hr)
		return WA_ERROR_FAIL;

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = false;
	LeaveCriticalSection(&pInstance->CriticalSection);

	return WA_OK;
}

/// <summary>
/// Resume from a Pause state
/// </summary>
/// <returns>WA_OK on success</returns>
uint32_t WA_Wasapi_Resume(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;
	uint8_t uVolume;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;

	// Get and set Current Volume (In a case someone changed volume value in pause state)
	uVolume = (uint8_t) SendMessage(This->Header.hMainWindow, WM_WA_MSG, MSG_GETVOLUME, 0);
	WA_Wasapi_Set_Volume(This, uVolume);

	hr = IAudioClient_Start(pInstance->pAudioClient);

	if FAILED(hr)
		return WA_ERROR_FAIL;

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = true;
	LeaveCriticalSection(&pInstance->CriticalSection);

	return WA_OK;
}

/// <summary>
/// Stop playback
/// </summary>
/// <returns>WA_OK on success</returns>
uint32_t WA_Wasapi_Stop(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;


	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	// On "End of Stream Event" bWasapiIsPlaying is set to False
	// and "bEndOfStreamHasStoppedPlayback" is set to True
	if ((pInstance->bWasapiIsPlaying == false) &&
		(pInstance->bEndOfStreamHasStoppedPlayback == false))
		return WA_ERROR_OUTPUTNOTREADY;

	EnterCriticalSection(&pInstance->CriticalSection);

	if (pInstance->bEndOfStreamHasStoppedPlayback == false)
	{
		hr = IAudioClient_Stop(pInstance->pAudioClient);
		_ASSERT(SUCCEEDED(hr));

		pInstance->bPendingEndOfStream = false;
	}

	LeaveCriticalSection(&pInstance->CriticalSection);


	hr = IAudioClient_Reset(pInstance->pAudioClient);
	_ASSERT(SUCCEEDED(hr));
	

	// Close Write Thread
	WA_Wasapi_ReleaseWriteThread(This, true);	

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = false;
	LeaveCriticalSection(&pInstance->CriticalSection);

	pInstance->bEndOfStreamHasStoppedPlayback = false;

	// Clear Circle Buffer
	if (pInstance->pCircle)
		WA_CircleBuffer_Reset(pInstance->pCircle);
	

	return WA_OK;

}

/// <summary>
/// Seek to a new Position in Ms
/// </summary>
/// <param name="uNewPositionMs">New Position in Ms</param>
/// <returns>WA_OK on success. With error code, the Write Thread is not started. 
/// Remember to close the open device. </returns>
uint32_t WA_Wasapi_Seek(WA_Output* This, uint64_t uNewPositionMs)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	WA_Input* pIn;
	uint32_t uResult;
	HRESULT hr;
	BOOL bResult;
	DWORD dwResult;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (!pInstance->bWasapiIsPlaying)
		return WA_ERROR_OUTPUTNOTREADY;


	pIn = This->pIn;

	if (!pIn->WA_Input_IsStreamSeekable(pIn))
		return WA_ERROR_STREAMNOTSEEKABLE;

	// Seek to a position we can handle
	if (uNewPositionMs >= (pIn->WA_Input_Duration(pIn) - pInstance->uCurrentLatencyMs))
		return WA_ERROR_BADPARAM;


	// Stop Playback
	hr = IAudioClient_Stop(pInstance->pAudioClient);
	_ASSERT(SUCCEEDED(hr));	

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = false;
	LeaveCriticalSection(&pInstance->CriticalSection);

	// Wait Output to be ready to a new write after a seek event
	bResult = SetEvent(pInstance->hSeekEvent);

	if(!bResult)
		return WA_ERROR_OUTPUTTHREADNOTREADY;

	// Wait Until Write Thread is Ready or a timeout occour
	dwResult = WaitForSingleObject(pInstance->hSyncEvent, WA_WASAPI_MAX_WAIT_TIME_MS);

	// Fail to Seek
	if (dwResult != WAIT_OBJECT_0)
		return WA_ERROR_OUTPUTTHREADNOTREADY;

	// Reset Device
	hr = IAudioClient_Reset(pInstance->pAudioClient);
	_ASSERT(SUCCEEDED(hr));


	// Seek to a new position
	EnterCriticalSection(&pInstance->CriticalSection);
	uResult = pIn->WA_Input_Seek(pIn, uNewPositionMs);
	LeaveCriticalSection(&pInstance->CriticalSection);

	if (uResult != WA_OK)
	{
		WA_Wasapi_ReleaseWriteThread(This, true);
		return WA_ERROR_STREAMNOTSEEKABLE;
	}

	// Clear Circle Buffer
	if (pInstance->pCircle)
		WA_CircleBuffer_Reset(pInstance->pCircle);
		

	// Fill device buffer with PCM data
	uResult = WA_Wasapi_FeedOutput(This);


	if (uResult != WA_WASAPI_OK)
	{
		WA_Wasapi_ReleaseWriteThread(This, true);
		return WA_ERROR_OUTPUTNOTREADY;
	}

	// Play on a new Position
	hr = IAudioClient_Start(pInstance->pAudioClient);

	if FAILED(hr)
	{
		WA_Wasapi_ReleaseWriteThread(This, true);
		return WA_ERROR_OUTPUTNOTREADY;
	}

	EnterCriticalSection(&pInstance->CriticalSection);
	pInstance->bWasapiIsPlaying = true;
	LeaveCriticalSection(&pInstance->CriticalSection);
		
	return WA_OK;

}

/// <summary>
/// Get current position 
/// </summary>
/// <param name="uPositionMs">Pointer to a uint64_t var</param>
/// <returns>WA_OK or an error code</returns>
uint32_t WA_Wasapi_Get_Position(WA_Output* This, uint64_t* uPositionMs)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	WA_Input* pIn;

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	if (!uPositionMs)
		return WA_ERROR_BADPTR;

	pIn = This->pIn;

	// Position in a stream can only be obtained if it is seekable 
	if (!pIn->WA_Input_IsStreamSeekable(pIn))
		return WA_ERROR_STREAMNOTSEEKABLE;

	EnterCriticalSection(&pInstance->CriticalSection);
	(*uPositionMs) = pIn->WA_Input_Position(pIn) - pInstance->uCurrentLatencyMs;
	LeaveCriticalSection(&pInstance->CriticalSection);

	return WA_OK;
}

/// <summary>
/// Set current stream volume (Min=0 | Max=255)
/// </summary>
/// <param name="uNewVolume">uint8_t Value</param>
/// <returns>WA_OK on success</returns>
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


/// <summary>
/// Get Current Stream Volume (Min=0 | Max=255)
/// </summary>
/// <param name="puVolume">Pointer to a uint8_t var</param>
/// <returns>WA_OK on success</returns>
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

/// <summary>
/// Get a copy of the currently playing data on the playback device.
/// The requested size must not exceed the latency of the output 
/// buffer, otherwise the function failsand returns an error code
/// </summary>
/// <param name="pBuffer">Pointer to an allocated byte buffer</param>
/// <param name="uBufferLen">Length of pBuffer</param>
/// <returns>WA_OK on success otherwise an error code</returns>
uint32_t WA_Wasapi_Get_BufferData(WA_Output* This, int8_t* pBuffer, uint32_t uBufferLen)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	UINT64 uDeviceFrequency;
	UINT64 uDevicePosition;	
	UINT64 uPositionMs;
	uint32_t uPositionInBytes;
	HRESULT hr;	
	bool bResult;
	

	if (!pInstance->bDeviceIsOpen)
		return WA_ERROR_OUTPUTNOTREADY;

	// Check if Circle Buffer is Valid		
	if (!pInstance->pCircle)
		return WA_ERROR_FAIL;

	hr = IAudioClock_GetFrequency(pInstance->pAudioClock,
		&uDeviceFrequency);

	if FAILED(hr)
		return WA_ERROR_OUTPUTNOTREADY;



#if WA_WASAPI_USE_HIGH_RES_TIMER
	LARGE_INTEGER llNowCounter;
	UINT64 uCounterValue, uDelayValue100Ns;
	UINT64 uDevicePositionQPC;

	hr = IAudioClock_GetPosition(pInstance->pAudioClock,
		&uDevicePosition,
		&uDevicePositionQPC);

	if FAILED(hr)
		return WA_ERROR_OUTPUTNOTREADY;

	if ((pInstance->uPCFrequency.QuadPart > 0LL) && QueryPerformanceCounter(&llNowCounter))
	{
		// Position = Device Position / Device Frequency
		llNowCounter.QuadPart *= 10000000;
		llNowCounter.QuadPart = llNowCounter.QuadPart / pInstance->uPCFrequency.QuadPart;
		uDelayValue100Ns = (UINT64)(llNowCounter.QuadPart) - uDevicePositionQPC;	
		

		uPositionMs = (uDevicePosition * 1000ULL) / uDeviceFrequency;	
		uPositionMs += uDelayValue100Ns / 10000;
	}
	else
	{
		// Position in Seconds = Device Position / Device Frequency
		uPositionMs = (uDevicePosition * 1000ULL) / uDeviceFrequency;
	}
#else

	hr = IAudioClock_GetPosition(pInstance->pAudioClock,
		&uDevicePosition,
		NULL);

	if FAILED(hr)
		return WA_ERROR_OUTPUTNOTREADY;

	// Position in Seconds = Device Position / Device Frequency
	uPositionMs = (uDevicePosition * 1000ULL) / uDeviceFrequency;

#endif


	// Convert to Circle Buffer Index
	uPositionMs = uPositionMs % (UINT64) pInstance->uCurrentLatencyMs;


	// Cast to an unsigned int value and convert to a byte position
	uPositionInBytes = (uint32_t) ((pInstance->StreamWfx.Format.nAvgBytesPerSec * (DWORD)uPositionMs) / 1000U);

	// Align to PCM blocks
	uPositionInBytes = uPositionInBytes - (uPositionInBytes % pInstance->StreamWfx.Format.nBlockAlign);

	EnterCriticalSection(&pInstance->CriticalSection);
	bResult = WA_CircleBuffer_ReadFrom(pInstance->pCircle, pBuffer, uBufferLen, uPositionInBytes);
	LeaveCriticalSection(&pInstance->CriticalSection);

	return  bResult ? WA_OK : WA_ERROR_FAIL;
}

uint32_t WA_Wasapi_Enable_DSP(WA_Output* This, bool bEnable)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	// If Device == Close CriticalSection is invalid
	if (pInstance->bDeviceIsOpen)
	{
		EnterCriticalSection(&pInstance->CriticalSection);
		pInstance->bEnableDSP = bEnable;
		LeaveCriticalSection(&pInstance->CriticalSection);
	}
	else
	{
		pInstance->bEnableDSP = bEnable;
	}

	return WA_OK;
}

void WA_Wasapi_ConfigDialog(WA_Output* This, HWND hParent)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;

	MessageBox(hParent, L"Wasapi Output Plugin", L"Hello World!", MB_OK);
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
		return 0U;

	// Get Buffer size in frames
	hr = IAudioClient_GetBufferSize(pInstance->pAudioClient, &uFrameBufferLength);

	if FAILED(hr)
		return 0U;

	// Get written data in the buffer in frames
	hr = IAudioClient_GetCurrentPadding(pInstance->pAudioClient, &uFrameBufferPadding);

	if FAILED(hr)
		return 0U;

	// Convert from frames to bytes
	return (uint32_t)((uFrameBufferLength - uFrameBufferPadding) * pInstance->StreamWfx.Format.nBlockAlign);	
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
	bool bResult, bProcessDSP;


	uBytesToWrite = WA_Wasapi_GetByteCanWrite(This);

	if (uBytesToWrite == 0)
		return WA_WASAPI_NOBYTESTOWRITE;

	pBuffer = (int8_t*) malloc(uBytesToWrite);

	if (!pBuffer)
		return WA_WASAPI_MALLOCERROR;


	EnterCriticalSection(&pInstance->CriticalSection);
	uResult = pIn->WA_Input_Read(pIn, pBuffer, uBytesToWrite, &uVaildBytes);
	LeaveCriticalSection(&pInstance->CriticalSection);

	// Detect End of File
	if ((uVaildBytes == 0) || (uResult == WA_ERROR_ENDOFFILE))
	{
		free(pBuffer);
		return WA_WASAPI_ENDOFFILE;
	}

	// Process DSP only if Enabled
	EnterCriticalSection(&pInstance->CriticalSection);
	bProcessDSP = pInstance->bEnableDSP;
	LeaveCriticalSection(&pInstance->CriticalSection);

	if ((pEffect) && (bProcessDSP))
	{
		
		uResult = pEffect->WA_Effect_Process(pEffect, pBuffer, uVaildBytes, &uVaildBytes);
		

		// Detect End of File
		if (uVaildBytes == 0)
		{
			free(pBuffer);
			return WA_WASAPI_ENDOFFILE;
		}
	}
	

	// Write Data Into Circle Buffer		
	if (pInstance->pCircle)
	{
		EnterCriticalSection(&pInstance->CriticalSection);
		WA_CircleBuffer_Write(pInstance->pCircle, pBuffer, uVaildBytes);
		LeaveCriticalSection(&pInstance->CriticalSection);
	}
		
	
	bResult = WA_Wasapi_WriteToDevice(This, pBuffer, uVaildBytes);
	free(pBuffer);	

	return (bResult == true ? WA_WASAPI_OK : WA_WASAPI_DEVICEWRITEERROR);
}

/// <summary>
/// Handle End of Stream. Stop Playing and Notify Main Window
/// </summary>
static void WA_Wasapi_Handle_EndOfStream(WA_Output* This, DWORD dwSleepTime)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	HRESULT hr;


	EnterCriticalSection(&pInstance->CriticalSection);

	// Prevent multiple calls if end of stream is already in progress
	if (!pInstance->bPendingEndOfStream)
	{
		// Wait before stop (es. Waiting for the playing buffer to be empty)
		if (dwSleepTime > 0)
			Sleep(dwSleepTime);

		hr = IAudioClient_Stop(pInstance->pAudioClient);

		_ASSERT(SUCCEEDED(hr));

		pInstance->bWasapiIsPlaying = false;
		pInstance->bEndOfStreamHasStoppedPlayback = true;
		pInstance->bPendingEndOfStream = true;

		// Notify Main window to Stop and Close Output (Post and leave)
		PostMessage(This->Header.hMainWindow, WM_WA_MSG, MSG_NOTIFYENDOFSTREAM, 0);
	}	

	LeaveCriticalSection(&pInstance->CriticalSection);

}

/// <summary>
/// Handle Wasapi Write Event. Our Write Function return one of those values
/// defined in WA_Macro.h
/// 
/// WA_WASAPI_OK					0x0000
/// WA_WASAPI_ENDOFFILE				0x0001
/// WA_WASAPI_INPUTERROR			0x0002
/// WA_WASAPI_MALLOCERROR			0x0003
/// WA_WASAPI_NOBYTESTOWRITE		0x0004
/// WA_WASAPI_DEVICEWRITEERROR		0x0005
/// 
/// </summary>
/// <returns>True on Success</returns>
static bool WA_Wasapi_Handle_WriteEvent(WA_Output* This)
{
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	uint32_t uResult, uWaitTime;
	bool bSkipWrite;

	// If Wasapi is not playing continue while-loop but don't write data to buffer
	// In Event-Drive mode wasapi send multiple request to write data into buffer
	// event if Playback is stopped in Main Thread
	EnterCriticalSection(&pInstance->CriticalSection);
	bSkipWrite = (pInstance->bWasapiIsPlaying == false);
	LeaveCriticalSection(&pInstance->CriticalSection);

	// On Stop, Seek or Pause don't write data into buffer!
	if (bSkipWrite)
		return true;

	// Write PCM Data into buffer
	uResult = WA_Wasapi_FeedOutput(This);	

	switch (uResult)
	{
	case WA_WASAPI_ENDOFFILE:

		// Wait Until Buffer is Empty
		uWaitTime = pInstance->uCurrentLatencyMs - (WA_Wasapi_GetByteCanWrite(This) / pInstance->StreamWfx.Format.nAvgBytesPerSec);
		uWaitTime = min(uWaitTime, pInstance->uCurrentLatencyMs);

		WA_Wasapi_Handle_EndOfStream(This, uWaitTime);
		return true;
	case WA_WASAPI_INPUTERROR:
	case WA_WASAPI_MALLOCERROR:
	case WA_WASAPI_DEVICEWRITEERROR:
		WA_Wasapi_Handle_EndOfStream(This, 0U);
		return false;
	case WA_WASAPI_NOBYTESTOWRITE:		
		return true;
	}



	return true;
}

/// <summary>
/// Handle error on WaitForMultipleObjects
/// </summary>
static void WA_Wasapi_Handle_WaitError(WA_Output* This)
{	
	WA_Wasapi_Handle_EndOfStream(This, 0U);
	_RPTW1(_CRT_WARN, L"Wasapi error code: %d \n", GetLastError());
}


/// <summary>
/// Write Thread
/// </summary>
/// <param name="lpParam">Pointer to Instance Struct</param>
/// <returns>0 on success</returns>
DWORD WINAPI WA_Wasapi_WriteThread(LPVOID lpParam)
{
	HANDLE hEvents[4];
	WA_Output* This = (WA_Output*) lpParam;
	WA_WasapiInstance* pInstance = (WA_WasapiInstance*)This->hPluginData;
	DWORD dwEvent, dwTaskIndex;
	bool bContinue = true;
	HRESULT hr;
	HANDLE hThreadPriority;

	// Initialize MultiThread Apartament Model
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	// Only Windows 7 and Above
	dwTaskIndex = 0U;
	hThreadPriority = AvSetMmThreadCharacteristics(L"Audio", &dwTaskIndex);


	if FAILED(hr)
		return EXIT_FAILURE;


	hEvents[0] = pInstance->hAbortEvent;
	hEvents[1] = pInstance->hSeekEvent;
	hEvents[2] = pInstance->hWriteEvent;
	hEvents[3] = pInstance->hStreamSwitchEvent;


	// Notify Main Thread we are ready to write
	SetEvent(pInstance->hSyncEvent);


	while (bContinue)
	{
		dwEvent = WaitForMultipleObjects(4, hEvents, FALSE, WA_WASAPI_MAX_WAIT_TIME_MS);


		switch (dwEvent)
		{
		case (WAIT_OBJECT_0 + 0): // Abort (Close Thread)
			bContinue = false;
			SetEvent(pInstance->hSyncEvent);
			break;
		case (WAIT_OBJECT_0 + 1): // Seek Event (Used to Sync Operatons)
			SetEvent(pInstance->hSyncEvent);
			break;
		case (WAIT_OBJECT_0 + 2): // Write Event (Write PCM Data)
			bContinue = WA_Wasapi_Handle_WriteEvent(This);
			break;
		case (WAIT_OBJECT_0 + 3) : // Stream Switch Event

			// Close Thread on fail
			if (WA_Wasapi_PerformStreamSwitch(This) == false)
				WA_Wasapi_Handle_EndOfStream(This, 0U);			

			_RPTW0(_CRT_WARN, L"Stream Switch Event!\n");

			break;		
		case WAIT_TIMEOUT: 
			Sleep(25);
			break;
		default: // On Error Notify Main Window and Close Current Thread
			WA_Wasapi_Handle_WaitError(This);
			bContinue = false;
		}
	}

	if (hThreadPriority)
		AvRevertMmThreadCharacteristics(hThreadPriority);

	CoUninitialize();

	return EXIT_SUCCESS;
}