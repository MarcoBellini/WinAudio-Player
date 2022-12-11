
#include "stdafx.h"
#include "IStreamInput.h"

#define COBJMACROS

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

// Linker Library
#pragma comment(lib , "mfplat.lib")
#pragma comment(lib , "mfreadwrite.lib")
#pragma comment(lib , "mfuuid.lib")

bool MediaFoundation_OpenFile(IStreamInput* pHandle, const wchar_t* pFilePath);
bool MediaFoundation_CloseFile(IStreamInput* pHandle);
bool MediaFoundation_Seek(IStreamInput* pHandle, uint64_t uBytesNewPosition, enum SEEK_ORIGIN seekOrigin);
bool MediaFoundation_Position(IStreamInput* pHandle, uint64_t* uBytesPosition);
bool MediaFoundation_Duration(IStreamInput* pHandle, uint64_t* uBytesDuration);
bool MediaFoundation_Read(IStreamInput* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
bool MediaFoundation_GetWaveFormat(IStreamInput* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
bool MediaFoundation_IsStreamSeekable(IStreamInput* pHandle);

typedef struct tagMediaFoundationInstance
{
	bool bIsFileOpen;
	bool bIsStreamSeekable;
	uint64_t uDuration;
	uint64_t uPosition;
	uint32_t uSamplerate;
	uint32_t uChannels;
	uint32_t uBitsPerSample;
	uint32_t uAvgBytesPerSec;
	uint32_t uAvgMFSampleSize;

	IMFSourceReader* pReader;	

	// Read Media Foundation Sample and
	// store in this buffer data that is > of requested
	// data
	int8_t* pRemainingData;
	uint32_t uRemainingDataLen;
} MediaFoundationInstance;


bool MediaFoundation_Initialize(IStreamInput* pStreamInput)
{
	MediaFoundationInstance* MFInstance;

	// Clear array memory
	ZeroMemory(&pStreamInput->ExtensionArray, sizeof(pStreamInput->ExtensionArray));

	// Add extension
	wcscpy_s(pStreamInput->ExtensionArray[0], 6, L".mp3");
	wcscpy_s(pStreamInput->ExtensionArray[1], 6, L".wma");
	pStreamInput->uExtensionsInArray = 2;

	// Assign Function Pointers
	pStreamInput->input_OpenFile = &MediaFoundation_OpenFile;
	pStreamInput->input_CloseFile = &MediaFoundation_CloseFile;
	pStreamInput->input_Seek = &MediaFoundation_Seek;
	pStreamInput->input_Position = &MediaFoundation_Position;
	pStreamInput->input_Duration = &MediaFoundation_Duration;
	pStreamInput->input_Read = &MediaFoundation_Read;
	pStreamInput->input_GetWaveFormat = &MediaFoundation_GetWaveFormat;
	pStreamInput->input_IsStreamSeekable = &MediaFoundation_IsStreamSeekable;

	// Alloc Module Instance
	pStreamInput->pModulePrivateData = malloc(sizeof(MediaFoundationInstance));

	// Check Pointer
	if (!pStreamInput->pModulePrivateData)
		return false;

	// Copy the address to Local pointer
	MFInstance = (MediaFoundationInstance*)pStreamInput->pModulePrivateData;

	// Reset vars
	MFInstance->bIsFileOpen = false;
	MFInstance->bIsStreamSeekable = false;
	MFInstance->uBitsPerSample = 0;
	MFInstance->uChannels = 0;
	MFInstance->uDuration = 0;
	MFInstance->uPosition = 0;
	MFInstance->uSamplerate = 0;
	MFInstance->uAvgMFSampleSize = 0;
	MFInstance->pReader = NULL;
	MFInstance->uRemainingDataLen = 0;
	MFInstance->pRemainingData = NULL;

	return true;
}

bool MediaFoundation_Deinitialize(IStreamInput* pStreamInput)
{
	// Free memory allocated in Initialize function
	if (pStreamInput->pModulePrivateData)
	{
		free(pStreamInput->pModulePrivateData);
		pStreamInput->pModulePrivateData = NULL;
		return true;
	}

	return false;
	
}

bool MediaFoundation_OpenFile(IStreamInput* pHandle, const wchar_t* pFilePath)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;
	IMFMediaType* pMediaType;
	PROPVARIANT pVarAttribute;	
	HRESULT hr;

	// Check if there is an already open stream
	if (MFInstance->bIsFileOpen)
		MediaFoundation_CloseFile(pHandle);

	// Reset Pointers
	pMediaType = NULL;
	MFInstance->pReader = NULL;


	/* Initialized the COM library in file
	   DecoderManager.c Line: 199 */

	hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);

	if SUCCEEDED(hr)
	{
		hr = MFCreateSourceReaderFromURL(pFilePath, NULL, &MFInstance->pReader);

		if SUCCEEDED(hr)
		{
			// Stream only first audio stream in the file
			hr = IMFSourceReader_SetStreamSelection(MFInstance->pReader, MF_SOURCE_READER_ALL_STREAMS, FALSE);
			_ASSERT(SUCCEEDED(hr));

			hr = IMFSourceReader_SetStreamSelection(MFInstance->pReader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
			_ASSERT(SUCCEEDED(hr));

			hr = MFCreateMediaType(&pMediaType);

			if SUCCEEDED(hr)
			{
				// We want only audio in PCM format
				hr = IMFMediaType_SetGUID(pMediaType, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
				_ASSERT(SUCCEEDED(hr));

				hr = IMFMediaType_SetGUID(pMediaType, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
				_ASSERT(SUCCEEDED(hr));

				// Set Partial Media Type to load the correct decoder
				hr = IMFSourceReader_SetCurrentMediaType(MFInstance->pReader, 
					MF_SOURCE_READER_FIRST_AUDIO_STREAM,
					NULL, 
					pMediaType);

				if SUCCEEDED(hr)
				{
					IMFMediaType_Release(pMediaType);
					pMediaType = NULL;

					// Get Full Media Type of the Stream
					hr = IMFSourceReader_GetCurrentMediaType(MFInstance->pReader, 
						MF_SOURCE_READER_FIRST_AUDIO_STREAM, 
						&pMediaType);

					if SUCCEEDED(hr)
					{
						// Samplerate
						IMFMediaType_GetUINT32(pMediaType, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &MFInstance->uSamplerate);
						_ASSERT(MFInstance->uSamplerate > 0);

						// Channels
						IMFMediaType_GetUINT32(pMediaType, &MF_MT_AUDIO_NUM_CHANNELS, &MFInstance->uChannels);
						_ASSERT(MFInstance->uChannels > 0);

						// Bits per Samples
						IMFMediaType_GetUINT32(pMediaType, &MF_MT_AUDIO_BITS_PER_SAMPLE, &MFInstance->uBitsPerSample);
						_ASSERT(MFInstance->uBitsPerSample > 0);

						// Avg Bytes Per Sec
						IMFMediaType_GetUINT32(pMediaType, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &MFInstance->uAvgBytesPerSec);
						_ASSERT(MFInstance->uAvgBytesPerSec > 0);

						// Duration
						ZeroMemory(&pVarAttribute, sizeof(PROPVARIANT));

						hr = IMFSourceReader_GetPresentationAttribute(MFInstance->pReader, 
							MF_SOURCE_READER_MEDIASOURCE, 
							&MF_PD_DURATION, 
							&pVarAttribute);

						_ASSERT(SUCCEEDED(hr));
									

						// 
						MFInstance->uPosition = 0;
						MFInstance->uDuration = (uint64_t)((pVarAttribute.uhVal.QuadPart / 10000000) * MFInstance->uAvgBytesPerSec);
						MFInstance->bIsFileOpen = true;
						MFInstance->bIsStreamSeekable = true;
					
						// Release Unused Interfaces
						IMFMediaType_Release(pMediaType);
						pMediaType = NULL;

						return true;
					}

				}
			}
		}

		// Safe Release Resource on Error

		if (pMediaType)
		{
			IMFMediaType_Release(pMediaType);
			pMediaType = NULL;
		}

		if (MFInstance->pReader)
		{
			IMFSourceReader_Release(MFInstance->pReader);
			MFInstance->pReader = NULL;
		}
	}

	return false;
}

bool MediaFoundation_CloseFile(IStreamInput* pHandle)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;

	if (!MFInstance->bIsFileOpen)
		return false;


	if (MFInstance->pReader)
	{
		IMFSourceReader_Release(MFInstance->pReader);
		MFInstance->pReader = NULL;
	}

	MFShutdown();

	// Close Any Pending Data
	if (MFInstance->uRemainingDataLen > 0)
	{
		MFInstance->uRemainingDataLen = 0;

		if(MFInstance->pRemainingData)
			free(MFInstance->pRemainingData);

		MFInstance->pRemainingData = NULL;
	}

	MFInstance->bIsFileOpen = false;
	MFInstance->bIsStreamSeekable = false;
	MFInstance->uBitsPerSample = 0;
	MFInstance->uChannels = 0;
	MFInstance->uDuration = 0;
	MFInstance->uPosition = 0;
	MFInstance->uSamplerate = 0;
	MFInstance->uAvgMFSampleSize = 0;

	return true;

}

bool MediaFoundation_Seek(IStreamInput* pHandle, uint64_t uBytesNewPosition, enum SEEK_ORIGIN seekOrigin)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;
	PROPVARIANT pVarAttribute;
	uint64_t uOffset;
	HRESULT hr;

	if (!MFInstance->bIsFileOpen)
		return false;


	ZeroMemory(&pVarAttribute, sizeof(PROPVARIANT));

	pVarAttribute.vt = VT_I8;
	uOffset = 0;

	switch (seekOrigin)
	{
	case BEGIN:		
		pVarAttribute.uhVal.QuadPart = (uBytesNewPosition / MFInstance->uAvgBytesPerSec) * 10000000;
		uOffset = uBytesNewPosition;
		break;
	case CURRENT:
		pVarAttribute.uhVal.QuadPart = ((MFInstance->uPosition + uBytesNewPosition) / MFInstance->uAvgBytesPerSec) * 10000000;
		uOffset = MFInstance->uPosition  + uBytesNewPosition;
		break;
	case END:
		pVarAttribute.uhVal.QuadPart = ((MFInstance->uDuration - uBytesNewPosition) / MFInstance->uAvgBytesPerSec) * 10000000;
		uOffset = MFInstance->uDuration - uBytesNewPosition;
		break;
	}

	hr = IMFSourceReader_SetCurrentPosition(MFInstance->pReader, &GUID_NULL, &pVarAttribute);

	if FAILED(hr)
		return false;

	// Reset allocated data on seek (if therse is any)
	if (MFInstance->uRemainingDataLen > 0)
	{		
		MFInstance->uRemainingDataLen = 0;

		if (MFInstance->pRemainingData)
			free(MFInstance->pRemainingData);

		MFInstance->pRemainingData = NULL;
	}

	// Update Positionon Success
	MFInstance->uPosition = uOffset;

	return true;
}

bool MediaFoundation_Position(IStreamInput* pHandle, uint64_t* uBytesPosition)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;

	if (!MFInstance->bIsFileOpen)
		return false;

	(*uBytesPosition) = MFInstance->uPosition;

	return true;
}

bool MediaFoundation_Duration(IStreamInput* pHandle, uint64_t* uBytesDuration)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;

	if (!MFInstance->bIsFileOpen)
		return false;

	(*uBytesDuration) = MFInstance->uDuration;

	return true;
}

static bool MediaFoundation_ReadSample(IStreamInput* pHandle, 
	int8_t **pByteBuffer, 
	uint32_t *uBufferSize)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;
	IMFSample* pSample;
	IMFMediaBuffer* pBuffer;
	HRESULT hr;
	DWORD pdwStreamFlags, dwBufferLen;
	BYTE* MFByteBuffer;

	// Reset Values
	(*pByteBuffer) = NULL;
	(*uBufferSize) = 0;

	// Read A Sample in the stream
	hr = IMFSourceReader_ReadSample(MFInstance->pReader,
		MF_SOURCE_READER_FIRST_AUDIO_STREAM,
		0,
		NULL,
		&pdwStreamFlags,
		NULL,
		&pSample);	


	// On Other Errors return false
	if (FAILED(hr) || 
		(pSample == NULL) || 
		(pdwStreamFlags == MF_SOURCE_READERF_ENDOFSTREAM))
		return false;


	hr = IMFSample_ConvertToContiguousBuffer(pSample, &pBuffer);

	// On Other Errors return false
	if FAILED(hr)
		return false;


	hr = IMFMediaBuffer_Lock(pBuffer, &MFByteBuffer, NULL, &dwBufferLen);

	if SUCCEEDED(hr)
	{
		// Copy PCM data to output buffer
		(*pByteBuffer) = (int8_t* ) malloc(dwBufferLen);

		if ((*pByteBuffer))
		{
			memcpy((*pByteBuffer), MFByteBuffer, dwBufferLen);
			(*uBufferSize) = dwBufferLen;
		}


		IMFMediaBuffer_Unlock(pBuffer);
	}


	IMFMediaBuffer_Release(pBuffer);
	IMFSample_Release(pSample);	

	return true;

}

bool MediaFoundation_Read(IStreamInput* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;
	int8_t* pSourceBuffer;
	uint32_t uSourceBufferSize;
	uint32_t uDataReaded;
	bool bContinueReading;

	if (!MFInstance->bIsFileOpen)
		return false;		

	// 1. Check if there is any pending data on the buffer
	uDataReaded = 0;
	(*uByteReaded) = 0;

	if (MFInstance->uRemainingDataLen > 0)
	{

		if (MFInstance->uRemainingDataLen <= uBytesToRead)
		{
			memcpy(pByteBuffer, MFInstance->pRemainingData, MFInstance->uRemainingDataLen);
			uDataReaded = MFInstance->uRemainingDataLen;
			MFInstance->uRemainingDataLen = 0;

			free(MFInstance->pRemainingData);
			MFInstance->pRemainingData = NULL;

			_RPTF1(_CRT_WARN, "(1) Pending data size: %u \n", uDataReaded);
		}
		else
		{
					
			memcpy(pByteBuffer, MFInstance->pRemainingData, uBytesToRead);

			uDataReaded = uBytesToRead;
			MFInstance->uRemainingDataLen -= uBytesToRead;

			// Translate Buffer to left (remove readed data and start valid data from 0)
			memmove(MFInstance->pRemainingData, 
				MFInstance->pRemainingData + uDataReaded, 
				MFInstance->uRemainingDataLen);	

			_RPTF1(_CRT_WARN, "(2) Pending data size: %u \n", uDataReaded);
		}


	}

	// 2. If uDataReaded < uBytesToRead read until we have enouth data to cover request
	bContinueReading = (uDataReaded < uBytesToRead) ? true : false;

	while (bContinueReading)
	{
		// On success copy data, on fail end of stream
		if (MediaFoundation_ReadSample(pHandle, &pSourceBuffer, &uSourceBufferSize))
		{

			// 3. if readed data + new data is less the requested size
			// copy data and read again, otherwise copy util we fill the buffer
			// and store data for next read
			if (uSourceBufferSize + uDataReaded <= uBytesToRead)
			{
				memcpy(pByteBuffer + uDataReaded, pSourceBuffer, uSourceBufferSize);
				uDataReaded += uSourceBufferSize;

				free(pSourceBuffer);
				pSourceBuffer = NULL;
				uSourceBufferSize = 0;
			}
			else
			{
				uint32_t uChuckSize = uBytesToRead - uDataReaded;

				memcpy(pByteBuffer + uDataReaded, pSourceBuffer, uChuckSize);

				// Copy data to remaining buffer
				MFInstance->pRemainingData = (int8_t*)malloc(uSourceBufferSize - uChuckSize);

				if (MFInstance->pRemainingData)
				{
					memcpy(MFInstance->pRemainingData, 
						pSourceBuffer + uChuckSize, 
						uSourceBufferSize - uChuckSize);

					MFInstance->uRemainingDataLen = uSourceBufferSize - uChuckSize;
				}

				uDataReaded += uChuckSize;

				free(pSourceBuffer);
				pSourceBuffer = NULL;
				uSourceBufferSize = 0;				

				bContinueReading = false;
			}			
		}
		else
		{
			bContinueReading = false;
		}
	}

	// 4. If we don't read any data return false
	// otherwise update position
	if (uDataReaded == 0)
		return false;
	
	(*uByteReaded) = uDataReaded;

	// Update Position Index
	MFInstance->uPosition += uDataReaded;

	return true;

}

bool MediaFoundation_GetWaveFormat(IStreamInput* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;

	if (!MFInstance->bIsFileOpen)
		return false;

	(*uSamplerate) = MFInstance->uSamplerate;
	(*uChannels) = MFInstance->uChannels;
	(*uBitsPerSample) = MFInstance->uBitsPerSample;

	return true;


}

bool MediaFoundation_IsStreamSeekable(IStreamInput* pHandle)
{
	MediaFoundationInstance* MFInstance = (MediaFoundationInstance*)pHandle->pModulePrivateData;

	return MFInstance->bIsStreamSeekable;
}