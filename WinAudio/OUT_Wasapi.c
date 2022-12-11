
#include "stdafx.h"
#include "IStreamOutput.h"


#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS


#include <initguid.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>


// Sources
// https://gist.github.com/t-mat/10a9e691c91997da3957
// https://github.com/pauldotknopf/WindowsSDK7-Samples/blob/master/multimedia/audio/DuckingMediaPlayer/MediaPlayer.cpp
// https://forum.pellesc.de/index.php?topic=6762.0
// https://github.com/andrewrk/libsoundio/blob/master/src/wasapi.c
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/multimedia/audio/RenderSharedEventDriven/WASAPIRenderer.cpp
// 
// 
// Define Vars fo CoCreateInstance
const static CLSID CLSID_MMDeviceEnumerator = { 0xBCDE0395,0xE52F,0x467C,0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E };// BCDE0395-E52F-467C-8E3D-C4579291692E
const static IID IID_IMMDeviceEnumerator = { 0xA95664D2,0x9614,0x4F35,0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6 };	// A95664D2-9614-4F35-A746-DE8DB63617E6
const static IID IID_IAudioClient = { 0x1CB9AD4C,0xDBFA,0x4C32,0xB1,0x78,0xC2,0xF5,0x68,0xA7,0x03,0xB2 };			// 1CB9AD4C-DBFA-4c32-B178-C2F568A703B2
const static IID IID_IAudioRenderClient = { 0xF294ACFC,0x3146,0x4483,0xA7,0xBF,0xAD,0xDC,0xA7,0xC2,0x60,0xE2 };	// F294ACFC-3146-4483-A7BF-ADDCA7C260E2
const static IID IID_IAudioClock = { 0xCD63314F,0x3FBA,0x4A1B,0x81,0x2C,0xEF,0x96,0x35,0x87,0x28,0xE7 };			// CD63314F-3FBA-4a1b-812C-EF96358728E7
const static IID IID_ISimpleAudioVolume = { 0x87CE5498,0x68D6,0x44E5,0x92,0x15,0x6D,0xA4,0x7E,0xF8,0x83,0xD8 };	// 87CE5498-68D6-44E5-9215-6DA47EF883D8

const static GUID WASAPI_KSDATAFORMAT_SUBTYPE_PCM = {
	0x00000001,0x0000,0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };


bool outWasapi_CreateDevice(IStreamOutput* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample, uint32_t* uMaxLatencyMs);
bool outWasapi_CloseDevice(IStreamOutput* pHandle);
bool outWasapi_CanWrite(IStreamOutput* pHandle);
bool outWasapi_GetByteCanWrite(IStreamOutput* pHandle, uint32_t* uByteToWrite);
bool outWasapi_WriteToDevice(IStreamOutput* pHandle, int8_t* pByteBuffer, uint32_t uByteToWrite);
bool outWasapi_FlushBuffers(IStreamOutput* pHandle);
bool outWasapi_IsPlaying(IStreamOutput* pHandle);
bool outWasapi_DevicePlay(IStreamOutput* pHandle);
bool outWasapi_DevicePause(IStreamOutput* pHandle, bool bIsInPause);
bool outWasapi_DeviceStop(IStreamOutput* pHandle);
bool outWasapi_GetWriteTime(IStreamOutput* pHandle, float* fWriteTimeMs);
bool outWasapi_GetPlayTime(IStreamOutput* pHandle, float* fPlayTimeMs);
bool outWasapi_DeviceVolume(IStreamOutput* pHandle, uint8_t* uVolumeValue, bool bGetOrSet);


// Memorize Instance Data
typedef struct tagOutWasapiInstance
{
	// Wasapi Interfaces
	IAudioClient* pAudioClient;
	IMMDevice* pIMMDevice;
	IAudioRenderClient *pRenderClient;
	ISimpleAudioVolume* pSimpleVolume;
	IAudioClock* pAudioClock;

	// Input Stream WaveFormat (Use AUTOCONVERT_PCM for now)
	WAVEFORMATEXTENSIBLE StreamWfx;

	// Store some output informations
	bool bDeviceIsOpen;
	bool bWasapiIsPlaying;	
	float fWrittenTimeMs;

} OutWasapiInstance;



bool OutWasapi_Initialize(IStreamOutput* pStreamOutput)
{
	OutWasapiInstance* pWasapiInstance;

	// Assign Functions pointers
	pStreamOutput->output_CreateDevice = &outWasapi_CreateDevice;
	pStreamOutput->output_CloseDevice = &outWasapi_CloseDevice;
	pStreamOutput->output_CanWrite = &outWasapi_CanWrite;
	pStreamOutput->output_GetByteCanWrite = &outWasapi_GetByteCanWrite;
	pStreamOutput->output_WriteToDevice = &outWasapi_WriteToDevice;
	pStreamOutput->output_FlushBuffers = &outWasapi_FlushBuffers;
	pStreamOutput->output_IsPlaying = &outWasapi_IsPlaying;
	pStreamOutput->output_DevicePlay = &outWasapi_DevicePlay;
	pStreamOutput->output_DevicePause = &outWasapi_DevicePause;
	pStreamOutput->output_DeviceStop = &outWasapi_DeviceStop;
	pStreamOutput->output_GetWriteTime = &outWasapi_GetWriteTime;
	pStreamOutput->output_GetPlayTime = &outWasapi_GetPlayTime;
	pStreamOutput->output_DeviceVolume = &outWasapi_DeviceVolume;

	// Alloc Module Instance
	pStreamOutput->pModulePrivateData = malloc(sizeof(OutWasapiInstance));

	// Check Pointer
	if (!pStreamOutput->pModulePrivateData)
		return false;

	// Copy the address to Local pointer
	pWasapiInstance = (OutWasapiInstance*)pStreamOutput->pModulePrivateData;

	// Reset values
	pWasapiInstance->bDeviceIsOpen = false;
	pWasapiInstance->bWasapiIsPlaying = false;
	pWasapiInstance->pAudioClient = NULL;
	pWasapiInstance->pIMMDevice = NULL;
	pWasapiInstance->pRenderClient = NULL;
	pWasapiInstance->pSimpleVolume = NULL;
	pWasapiInstance->pAudioClock = NULL;
	pWasapiInstance->fWrittenTimeMs = 0.0f;

	return true;
}

bool OutWasapi_Deinitialize(IStreamOutput* pStreamOutput)
{
	// Free memory allocated in Initialize function
	if (pStreamOutput->pModulePrivateData)
	{
		free(pStreamOutput->pModulePrivateData);
		pStreamOutput->pModulePrivateData = NULL;
	}
		

	return true;
}

bool outWasapi_CreateDevice(IStreamOutput* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample, uint32_t* uMaxLatencyMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	IMMDeviceEnumerator* pDeviceEnumerator;
	HRESULT hr;
	DWORD dwStreamFlags = 0;
	REFERENCE_TIME nBufferLatency;
	PWAVEFORMATEXTENSIBLE pWfx;
	PWAVEFORMATEXTENSIBLE pDeviceWfx;
	bool bResult = false;

	// Check if Device is Already Open
	if (pWasapiInstance->bDeviceIsOpen)
		outWasapi_CloseDevice(pHandle);

	// Create Instance to Device Enumerator
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IMMDeviceEnumerator,
		(LPVOID)&pDeviceEnumerator);


	if SUCCEEDED(hr)
	{
		// Get Default System Endpoint
		hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pDeviceEnumerator,
			eRender,
			eMultimedia,
			&pWasapiInstance->pIMMDevice);


		if SUCCEEDED(hr)
		{
			// Create the Audio Client Interface
			hr = IMMDevice_Activate(pWasapiInstance->pIMMDevice,
				&IID_IAudioClient,
				CLSCTX_ALL,
				NULL,
				(LPVOID*)&pWasapiInstance->pAudioClient);



			if SUCCEEDED(hr)
			{
				// Get pointer from Instace
				pWfx = &pWasapiInstance->StreamWfx;


				// Create Input Stream WaveFormat
				pWfx->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
				pWfx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
				pWfx->SubFormat = WASAPI_KSDATAFORMAT_SUBTYPE_PCM;
				pWfx->dwChannelMask = (uChannels == 1) ? SPEAKER_FRONT_LEFT : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
				pWfx->Format.nSamplesPerSec = (DWORD)uSamplerate;
				pWfx->Format.nChannels = (WORD)uChannels;
				pWfx->Format.wBitsPerSample = (WORD)uBitsPerSample;
				pWfx->Format.nBlockAlign = (WORD)(uBitsPerSample * uChannels / 8);
				pWfx->Format.nAvgBytesPerSec = (DWORD)(pWfx->Format.nBlockAlign * uSamplerate);
				pWfx->Samples.wValidBitsPerSample = (WORD)uBitsPerSample;


				// Get Audio Engine Mix Format (for Shared Mode)
				hr = IAudioClient_GetMixFormat(pWasapiInstance->pAudioClient,
					(LPWAVEFORMATEX*)&pDeviceWfx);


				if SUCCEEDED(hr)
				{
					// Check if need to resample output
					if (pWfx->Format.nSamplesPerSec == pDeviceWfx->Format.nSamplesPerSec)
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
					nBufferLatency = 10000 * (REFERENCE_TIME)(*uMaxLatencyMs);

					// Initialize Wasapi Device (In Shared Mode)
					hr = IAudioClient_Initialize(pWasapiInstance->pAudioClient,
						AUDCLNT_SHAREMODE_SHARED,
						dwStreamFlags,
						nBufferLatency,
						0,
						(LPWAVEFORMATEX) pWfx,
						NULL);



					if SUCCEEDED(hr)
					{
						// Set Refill event handle
						hr = IAudioClient_SetEventHandle(pWasapiInstance->pAudioClient,
							pHandle->hOutputWriteEvent);

						if SUCCEEDED(hr)
						{
							// Get Current System Latency
							hr = IAudioClient_GetStreamLatency(pWasapiInstance->pAudioClient,
								&nBufferLatency);

							// Check if we have a valid value
							if (SUCCEEDED(hr) && (nBufferLatency != 0))
								*uMaxLatencyMs = (uint32_t)(nBufferLatency / 10000);

							// Audio Render Client (used to write input buffer)
							hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
								&IID_IAudioRenderClient,
								(LPVOID*)&pWasapiInstance->pRenderClient);

							if SUCCEEDED(hr)
							{
								// Simple Volume Control
								hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
									&IID_ISimpleAudioVolume,
									(LPVOID*)&pWasapiInstance->pSimpleVolume);

								if SUCCEEDED(hr)
								{
									// Audio Clock (used to retive the position in the buffer)
									hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
										&IID_IAudioClock,
										(LPVOID*)&pWasapiInstance->pAudioClock);

									if SUCCEEDED(hr)
									{
										// Success
										bResult = true;
										pWasapiInstance->bDeviceIsOpen = true;
									}
									else
									{
										// Release Audio Client on Error
										IAudioClient_Release(pWasapiInstance->pAudioClient);
									}
								}
								else
								{
									// Release Audio Client on Error
									IAudioClient_Release(pWasapiInstance->pAudioClient);
								}


							}
							else
							{
								// Release Audio Client on Error
								IAudioClient_Release(pWasapiInstance->pAudioClient);
							}


						}
						else
						{
							// Release Audio Client on Error
							IAudioClient_Release(pWasapiInstance->pAudioClient);
						}			
					}
				}
			}

			// Release Device
			IMMDevice_Release(pWasapiInstance->pIMMDevice);
		}

		// Release Enumerator
		IMMDeviceEnumerator_Release(pDeviceEnumerator);
	}

	return bResult;
}

bool outWasapi_CloseDevice(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	// Check if the instance is open
	if (pWasapiInstance->bDeviceIsOpen == false)
		return false;

	// Check for a valid pointer and release
	if (pWasapiInstance->pAudioClock)
		IAudioClock_Release(pWasapiInstance->pAudioClock);

	if (pWasapiInstance->pSimpleVolume)
		ISimpleAudioVolume_Release(pWasapiInstance->pSimpleVolume);

	if (pWasapiInstance->pRenderClient)
		IAudioRenderClient_Release(pWasapiInstance->pRenderClient);

	if (pWasapiInstance->pAudioClient)
		IAudioClient_Release(pWasapiInstance->pAudioClient);

	// Reset Pointers
	pWasapiInstance->pAudioClient = NULL;
	pWasapiInstance->pIMMDevice = NULL;
	pWasapiInstance->pRenderClient = NULL;
	pWasapiInstance->pSimpleVolume = NULL;
	pWasapiInstance->pAudioClock = NULL;

	// Set Current Instance to Close
	pWasapiInstance->bDeviceIsOpen = false;
	pWasapiInstance->bWasapiIsPlaying = false;
	pWasapiInstance->fWrittenTimeMs = 0.0f;

	return true;
}

bool outWasapi_CanWrite(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uFrameBufferLength;
	UINT32 uFrameBufferPadding;
	HRESULT hr;

	// Check if devise is open
	if (pWasapiInstance->bDeviceIsOpen) 
	{	
		// Get Buffer size in frames
		hr = IAudioClient_GetBufferSize(pWasapiInstance->pAudioClient, &uFrameBufferLength);

		if SUCCEEDED(hr)
		{
			// Get written data in the buffer in frames
			hr = IAudioClient_GetCurrentPadding(pWasapiInstance->pAudioClient, &uFrameBufferPadding);

			if SUCCEEDED(hr)
			{
				// Check if buffer can contain some frames
				if ((uFrameBufferLength - uFrameBufferPadding) > 0)
				{
					return true;
				}
			}
		}	
	}

	return false;
}

bool outWasapi_GetByteCanWrite(IStreamOutput* pHandle, uint32_t* uByteToWrite)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uFrameBufferLength;
	UINT32 uFrameBufferPadding;
	HRESULT hr;

	// Check if devise is open
	if (pWasapiInstance->bDeviceIsOpen)
	{
		// Get Buffer size in frames
		hr = IAudioClient_GetBufferSize(pWasapiInstance->pAudioClient, &uFrameBufferLength);

		if SUCCEEDED(hr)
		{
			// Get written data in the buffer in frames
			hr = IAudioClient_GetCurrentPadding(pWasapiInstance->pAudioClient, &uFrameBufferPadding);

			if SUCCEEDED(hr)
			{
				// Check if buffer can contain some frames
				if ((uFrameBufferLength - uFrameBufferPadding) > 0)
				{
					// Convert from frames to bytes
					(*uByteToWrite) = (uFrameBufferLength - uFrameBufferPadding) * pWasapiInstance->StreamWfx.Format.nBlockAlign;
					return true;
				}
			}
		}		
	}

	return false;
}

bool outWasapi_WriteToDevice(IStreamOutput* pHandle, int8_t* pByteBuffer, uint32_t uByteToWrite)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uRequestedFrames;
	BYTE* pWasapiBuffer;
	HRESULT hr;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		// Check if we have a valid pointer
		if ((pByteBuffer) && (uByteToWrite > 0))
		{
			uRequestedFrames = (UINT32) (uByteToWrite / pWasapiInstance->StreamWfx.Format.nBlockAlign);

			// Get Device Buffer
			hr = IAudioRenderClient_GetBuffer(pWasapiInstance->pRenderClient,
				uRequestedFrames,
				&pWasapiBuffer);

			if SUCCEEDED(hr)
			{
				if (memcpy_s(pWasapiBuffer, uByteToWrite, pByteBuffer, uByteToWrite) == 0)
				{
					// Release Buffer if AUDCLNT_BUFFERFLAGS_SILENT it consider buffer as silent
					IAudioRenderClient_ReleaseBuffer(pWasapiInstance->pRenderClient,
						uRequestedFrames,
						0);

					
					// Increment Written Time in Milliseconds
					pWasapiInstance->fWrittenTimeMs += (uByteToWrite / 
														(float) pWasapiInstance->StreamWfx.Format.nAvgBytesPerSec * 
														1000);

					
					// Success
					return true;
				}
				else
				{
					// Fail to Write output buffer
					IAudioRenderClient_ReleaseBuffer(pWasapiInstance->pRenderClient, 0, 0);
				}


			}
		}
	}

	return false;
}

bool outWasapi_FlushBuffers(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if ((pWasapiInstance->bDeviceIsOpen))
	{
		// Reset Pending data and Clock timer to 0
		HRESULT hr = IAudioClient_Reset(pWasapiInstance->pAudioClient);
		pWasapiInstance->fWrittenTimeMs = 0.0f;

		_ASSERT(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	
	return false;

}

bool outWasapi_IsPlaying(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if ((pWasapiInstance->bDeviceIsOpen) && (pWasapiInstance->bWasapiIsPlaying))
		return true;

	return false;
}

bool outWasapi_DevicePlay(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		IAudioClient_Start(pWasapiInstance->pAudioClient);
		pWasapiInstance->bWasapiIsPlaying = true;
		return true;
	}

	return false;
}

bool outWasapi_DevicePause(IStreamOutput* pHandle, bool bPause)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		if (!bPause)
		{
			// Resume Playing
			IAudioClient_Start(pWasapiInstance->pAudioClient);
			pWasapiInstance->bWasapiIsPlaying = true;
			return true;
		}
		else
		{
			// Pause Playing
			IAudioClient_Stop(pWasapiInstance->pAudioClient);
			pWasapiInstance->bWasapiIsPlaying = false;
			return true;
		}
	}

	return false;
}

bool outWasapi_DeviceStop(IStreamOutput* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		HRESULT hr;

		// Stop Playing
		hr = IAudioClient_Stop(pWasapiInstance->pAudioClient);

		if SUCCEEDED(hr)
		{
			pWasapiInstance->bWasapiIsPlaying = false;
			// pWasapiInstance->fWrittenTimeMs = 0.0f;
			return true;
		}		
	}

	return false;
}

bool outWasapi_GetWriteTime(IStreamOutput* pHandle, float *fWriteTimeMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		(*fWriteTimeMs) = pWasapiInstance->fWrittenTimeMs;
		return true;
	}

	return false;
}

bool outWasapi_GetPlayTime(IStreamOutput* pHandle, float* fPlayTimeMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT64 uDeviceFrequency;
	UINT64 uDevicePosition;
	UINT64 uDevicePositionQPC;
	HRESULT hr;
	// LARGE_INTEGER qpFreq, qpCounter, qpResult;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		hr = IAudioClock_GetFrequency(pWasapiInstance->pAudioClock, 
									  &uDeviceFrequency);

		if SUCCEEDED(hr)
		{
			hr = IAudioClock_GetPosition(pWasapiInstance->pAudioClock, 
										 &uDevicePosition, 
										 &uDevicePositionQPC);

			if SUCCEEDED(hr)
			{
				/*
				QueryPerformanceFrequency(&qpFreq);
				QueryPerformanceCounter(&qpCounter);

				qpCounter.QuadPart *= 10000000;
				qpCounter.QuadPart /= qpFreq.QuadPart;
				qpResult.QuadPart = qpCounter.QuadPart - uDevicePositionQPC;
				//qpResult.QuadPart /= 10000000;
				*/

				(*fPlayTimeMs) = (float)(uDevicePosition) /(float)(uDeviceFrequency);
				

				// Convert from Seconds to Milliseconds
				(*fPlayTimeMs) = (*fPlayTimeMs) * 1000.0f;
				
				return true;
			}
		}		
	}

	return false;
}

bool outWasapi_DeviceVolume(IStreamOutput* pHandle, uint8_t* uVolumeValue, bool bGetOrSet)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	float fVolumeValue;
	HRESULT hr;

	// Fail if devise is not open
	if (!pWasapiInstance->bDeviceIsOpen)
		return false;

	// true = Set ; false = Get
	if (bGetOrSet)
	{
		// Convert from uint8 to a Normalized float Value
		fVolumeValue = ((float)(*uVolumeValue) / UINT8_MAX);

		// Set the Master Volume: Range between 0.0 to 1.0
		hr = ISimpleAudioVolume_SetMasterVolume(pWasapiInstance->pSimpleVolume, fVolumeValue, NULL);

		return SUCCEEDED(hr);
	}
	else
	{
		// Get the Master Volume: Range between 0.0 to 1.0
		hr = ISimpleAudioVolume_GetMasterVolume(pWasapiInstance->pSimpleVolume, &fVolumeValue);

		// Convert from a float value to uint8
		(*uVolumeValue) = (uint8_t)(fVolumeValue * UINT8_MAX);

		return SUCCEEDED(hr);
	}

}

