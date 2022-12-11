
#include "stdafx.h"
#include "IStreamInput.h"


// Functions declarations (private functions)
bool StreamWav_OpenFile(IStreamInput* pHandle, const wchar_t* pFilePath);
bool StreamWav_CloseFile(IStreamInput* pHandle);
bool StreamWav_Seek(IStreamInput* pHandle, uint64_t uBytesNewPosition, enum SEEK_ORIGIN seekOrigin);
bool StreamWav_Position(IStreamInput* pHandle, uint64_t* uBytesPosition);
bool StreamWav_Duration(IStreamInput* pHandle, uint64_t* uBytesDuration);
bool StreamWav_Read(IStreamInput* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
bool StreamWav_GetWaveFormat(IStreamInput* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
bool StreamWav_IsStreamSeekable(IStreamInput* pHandle);


// Store instance value
typedef struct tagStreamWaveInstance
{
	HANDLE hFileHandle;
	bool bIsFileOpen;
	bool bIsStreamSeekable;
	uint64_t uDuration;
	uint64_t uPosition;
	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	uint64_t uHeaderOffset;
} StreamWaveInstance;

// PCM Wave Format Header
typedef struct tagWaveHeader
{
	char chunkID[4];
	uint32_t chuckSize;
	char format[4];
	char subchunk1ID[4];
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t avgbytespersec;
	uint16_t blockalign;
	uint16_t bitspersample;
	char subchunk2ID[4];
	uint32_t subchunk2Size;
}WaveHeader;



bool StreamWav_Initialize(IStreamInput* pStreamInput)
{
	StreamWaveInstance* WavInstance;

	// Clear array memory
	ZeroMemory(&pStreamInput->ExtensionArray, sizeof(pStreamInput->ExtensionArray));

	// Add extension
	wcscpy_s(pStreamInput->ExtensionArray[0], 6, L".wav");
	pStreamInput->uExtensionsInArray = 1;

	// Assign Function Pointers
	pStreamInput->input_OpenFile = &StreamWav_OpenFile;
	pStreamInput->input_CloseFile = &StreamWav_CloseFile;
	pStreamInput->input_Seek = &StreamWav_Seek;
	pStreamInput->input_Position = &StreamWav_Position;
	pStreamInput->input_Duration = &StreamWav_Duration;
	pStreamInput->input_Read = &StreamWav_Read;
	pStreamInput->input_GetWaveFormat = &StreamWav_GetWaveFormat;
	pStreamInput->input_IsStreamSeekable = &StreamWav_IsStreamSeekable;

	// Alloc Module Instance
	pStreamInput->pModulePrivateData = malloc(sizeof(StreamWaveInstance));

	// Check Pointer
	if (!pStreamInput->pModulePrivateData)
		return false;

	// Copy the address to Local pointer
	WavInstance = (StreamWaveInstance *) pStreamInput->pModulePrivateData;

	// Reset values
	WavInstance->bIsFileOpen = false;
	WavInstance->hFileHandle = INVALID_HANDLE_VALUE;
	WavInstance->uBitsPerSample = 0;
	WavInstance->uChannels = 0;
	WavInstance->uSamplerate = 0;
	WavInstance->uPosition = 0;
	WavInstance->uDuration = 0;
	WavInstance->uHeaderOffset = 0;
	WavInstance->bIsStreamSeekable = false;

	return true;
}

bool StreamWav_Deinitialize(IStreamInput* pStreamInput)
{
	// Free memory allocated in Initialize function
	if (pStreamInput->pModulePrivateData)
	{
		free(pStreamInput->pModulePrivateData);
		pStreamInput->pModulePrivateData = NULL;
	}
		

	return true;
}

bool StreamWav_OpenFile(IStreamInput* pHandle, const wchar_t* pFilePath)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;
	WaveHeader* pWaveHeader;
	DWORD dwBytesReaded;
	bool bResult;
	LARGE_INTEGER lNewPosition;

	// Check if file is already open
	if (pWavInstance->bIsFileOpen)
		StreamWav_CloseFile(pHandle);

	// Open file
	pWavInstance->hFileHandle = CreateFile(pFilePath,
										GENERIC_READ,
										FILE_SHARE_READ,
										NULL,
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL);

	// Check for errors
	if (pWavInstance->hFileHandle == INVALID_HANDLE_VALUE)
		return false;

	// Try to Read Wave Format Header
	pWaveHeader = (WaveHeader*) malloc(sizeof(WaveHeader));

	// Check pointer
	if (!pWaveHeader)
		return false;

	// Clear Memory
	ZeroMemory(pWaveHeader, sizeof(WaveHeader));

	// Read Header
	bResult = ReadFile(pWavInstance->hFileHandle,
						(LPVOID)pWaveHeader,
						sizeof(WaveHeader),
						&dwBytesReaded,
						NULL);


	// Free resources on error and return
	if (bResult == false)
	{
		free(pWaveHeader);
		CloseHandle(pWavInstance->hFileHandle);
		pWavInstance->hFileHandle = NULL;
		return false;
	}

	// Reset Result var
	bResult = false; 

	// Check if contains Ascii code 0x52494646 - "R" "I" "F" "F" 
	// NB: big-endian form
	if ((pWaveHeader->chunkID[0] == 0x52) && (pWaveHeader->chunkID[1] == 0x49) &&
		(pWaveHeader->chunkID[2] == 0x46) && (pWaveHeader->chunkID[3] == 0x46))
	{

		// Check if contains Ascii code 0x57415645 "W" "A" "V" "E"
		// NB: big-endian form
		if ((pWaveHeader->format[0] == 0x57) && (pWaveHeader->format[1] == 0x41) &&
			(pWaveHeader->format[2] == 0x56) && (pWaveHeader->format[3] == 0x45))
		{

			// Check if contains Ascii code  0x666d7420  "f""m""t"
			// NB: big-endian form
			if ((pWaveHeader->subchunk1ID[0] == 0x66) && (pWaveHeader->subchunk1ID[1] == 0x6D) &&
				(pWaveHeader->subchunk1ID[2] == 0x74) && (pWaveHeader->subchunk1ID[3] == 0x20))
			{

				// Check if WAV is PCM
				if ((pWaveHeader->subchunk1Size == 16) && (pWaveHeader->audioFormat == 1))
				{
					bResult = true;
				}
			}
		}
	}

	// Free resources on error and return
	if (bResult == false)
	{
		free(pWaveHeader);
		CloseHandle(pWavInstance->hFileHandle);
		pWavInstance->hFileHandle = NULL;
		return false;
	}
		
	// Fill Instance Informations
	pWavInstance->bIsFileOpen = true;
	pWavInstance->uHeaderOffset = (uint64_t) sizeof(WaveHeader);
	pWavInstance->uChannels = pWaveHeader->channels;
	pWavInstance->uSamplerate = pWaveHeader->samplerate;
	pWavInstance->uBitsPerSample = pWaveHeader->bitspersample;
	pWavInstance->uDuration = pWaveHeader->subchunk2Size;
	pWavInstance->uPosition = 0;

	// Try to seek
	lNewPosition.QuadPart = (LONGLONG)pWavInstance->uHeaderOffset;

	pWavInstance->bIsStreamSeekable = SetFilePointerEx(pWavInstance->hFileHandle,
										lNewPosition,
										NULL,
										FILE_BEGIN) != false;

	// Free resources
	free(pWaveHeader);

	// Success
	return true;
}

bool StreamWav_CloseFile(IStreamInput* pHandle)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;

	// Close only if there is an instance already open
	if (pWavInstance->bIsFileOpen)
	{
		CloseHandle(pWavInstance->hFileHandle);
		pWavInstance->hFileHandle = INVALID_HANDLE_VALUE;
		pWavInstance->bIsFileOpen = false;	

		return true;
	}

	return false;
}

bool StreamWav_Seek(IStreamInput* pHandle, uint64_t uBytesNewPosition, enum SEEK_ORIGIN seekOrigin)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;
	LARGE_INTEGER lNewPosition;

	if ((pWavInstance->bIsFileOpen) && (pWavInstance->bIsStreamSeekable))
	{
		// Store new position
		lNewPosition.QuadPart = (LONGLONG)(uBytesNewPosition + pWavInstance->uHeaderOffset);

		

		// Seek to new position
		switch (seekOrigin)
		{
		case BEGIN:
			// Save the new position
			pWavInstance->uPosition = uBytesNewPosition;

			return SetFilePointerEx(pWavInstance->hFileHandle,
							lNewPosition,
							NULL,
							FILE_BEGIN) != 0;
			
		case CURRENT:
			// Save the new position
			pWavInstance->uPosition += uBytesNewPosition;

			return SetFilePointerEx(pWavInstance->hFileHandle,
							lNewPosition,
							NULL,
							FILE_CURRENT) != 0;
			
		case END:
			return false; // Not supported
			
		}

		
	}

	return false;	
}

bool StreamWav_Position(IStreamInput* pHandle, uint64_t* uBytesPosition)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;

	// Check if file is open
	if (!pWavInstance->bIsFileOpen)
		return false;

	// Assign value and adjust with header offset
	*uBytesPosition = pWavInstance->uPosition - pWavInstance->uHeaderOffset;

	return true;
}

bool StreamWav_Duration(IStreamInput* pHandle, uint64_t* uBytesDuration)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;

	// Check if file is open
	if (!pWavInstance->bIsFileOpen)
		return false;

	// Assign value
	*uBytesDuration = pWavInstance->uDuration - pWavInstance->uHeaderOffset;

	return true;
}

bool StreamWav_Read(IStreamInput* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;
	BOOL bReadSuccess;

	// Check if file is open
	if (!pWavInstance->bIsFileOpen)
		return false;

	// Check if we reached the end of stream
	if (pWavInstance->uPosition == pWavInstance->uDuration)
		return false;	

	// Check if we can read requested bytes
	if (pWavInstance->uPosition + uBytesToRead > pWavInstance->uDuration)
	{
		uBytesToRead = (uint32_t) (pWavInstance->uDuration - pWavInstance->uPosition);
	}

	// Try to read file
	bReadSuccess = ReadFile(pWavInstance->hFileHandle,
							(LPVOID)pByteBuffer,
							uBytesToRead,
							uByteReaded,
							NULL);

	// Increment position on success
	if (bReadSuccess == TRUE)
	{
		pWavInstance->uPosition += (*uByteReaded);
		return true;
	}

	// Return false on Fail
	return false;
}

bool StreamWav_GetWaveFormat(IStreamInput* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;

	// Check if file is open
	if (!pWavInstance->bIsFileOpen)
		return false;

	// Fill values
	*uSamplerate = pWavInstance->uSamplerate;
	*uChannels = pWavInstance->uChannels;
	*uBitsPerSample = pWavInstance->uBitsPerSample;

	return true;
}

bool StreamWav_IsStreamSeekable(IStreamInput* pHandle)
{
	StreamWaveInstance* pWavInstance = (StreamWaveInstance*)pHandle->pModulePrivateData;

	return pWavInstance->bIsStreamSeekable;

}