
#include "stdafx.h"
#include "DecoderManager.h"


// Main Thread Loop (It manages read from input and write in output)(also write the spectrum buffer)
DWORD WINAPI DecoderManagerEngineProc(LPVOID lpParam);
uint8_t DecoderManagerEngine_GetDecoder(DecoderManagerEngine* pEngine, const wchar_t* pFilePath);
bool DecoderManagerEngine_IsBadWfx(uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample);
bool DecoderManagerEngine_WriteOutput(DecoderManagerEngine* pEngine);
bool DecoderManagerEngine_OpenFile(DecoderManagerEngine* pEngine, const wchar_t* pFilePath);
void DecoderManagerEngine_CloseFile(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_Play(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_Stop(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_Pause(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_UnPause(DecoderManagerEngine* pEngine);
bool DecoderManagerEngine_Seek(DecoderManagerEngine* pEngine, uint64_t uMsNewPosition);
void DecoderManagerEngine_CurrentPosition(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_ByteToMs(DecoderManagerEngine* pEngine, uint64_t uBytes, uint64_t *puMs);
void DecoderManagerEngine_MsToByte(DecoderManagerEngine* pEngine, uint64_t uMs, uint64_t *puBytes);
void DecoderManagerEngine_CheckAndFillCircle(DecoderManagerEngine* pEngine, uint32_t uBytesToWrite);
void DecoderManagerEngine_HandleEndOfStream(DecoderManagerEngine* pEngine);
void DecoderManagerEngine_SetVolume(DecoderManagerEngine* pEngine, uint8_t *uVolumeValue);
void DecoderManagerEngine_GetVolume(DecoderManagerEngine* pEngine, uint8_t *uVolumeValue);
void DecoderManagerEngine_GetPlayingBuffer(DecoderManagerEngine* pEngine, int8_t* pByteBuffer, uint16_t* puBufferLen);
void DecoderManagerEngine_AssignDSPCallback(DecoderManagerEngine* pEngine, DecoderManager_DSP_Callback pCallback);
void DecoderManagerEngine_ProcessDSP(DecoderManagerEngine* pEngine, int8_t* pByteBuffer, uint32_t uCount);




#if _DEBUG
static void DecoderManager_MessageToString(uint32_t uMessageCode, wchar_t *MessageString)
{
	switch (uMessageCode)
	{
	case DECODER_PLAY:	
		wcscpy_s(MessageString, MAX_PATH, L"Play\0");
		break;
	case DECODER_PAUSE:
		wcscpy_s(MessageString, MAX_PATH, L"Pause\0");
		break;
	case DECODER_STOP:
		wcscpy_s(MessageString, MAX_PATH, L"Stop\0");
		break;
	case DECODER_SEEK:
		wcscpy_s(MessageString, MAX_PATH, L"Seek\0");
		break;
	case DECODER_OPENFILE:
		wcscpy_s(MessageString, MAX_PATH, L"OpenFile\0");
		break;
	case DECODER_CLOSEFILE:	
		wcscpy_s(MessageString, MAX_PATH, L"CloseFile\0");
		break;
	case DECODER_SETVOLUME:	
		wcscpy_s(MessageString, MAX_PATH, L"SetVolume\0");
		break;
	case DECODER_GETVOLUME:	
		wcscpy_s(MessageString, MAX_PATH, L"GetVolume\0");
		break;
	case DECODER_CLOSETHREAD:
		wcscpy_s(MessageString, MAX_PATH, L"CloseThread\0");
		break;
	case DECODER_ENDOFSTREAM:	
		wcscpy_s(MessageString, MAX_PATH, L"EndOFStream\0");
		break;
	case DECODER_GETPLAYINGBUFFER:
		wcscpy_s(MessageString, MAX_PATH, L"GetPlayingBuffer\0");
		break;
	case DECODER_GETPOSITION:	
		wcscpy_s(MessageString, MAX_PATH, L"GetPosition\0");
		break;
	case DECODER_UNPAUSE:	
		wcscpy_s(MessageString, MAX_PATH, L"UnPause\0");
		break;
	case DECODER_ASSIGNDSPCALLBACK:
		wcscpy_s(MessageString, MAX_PATH, L"AssignDSPCallback\0");
		break;
	}
}

#endif

static bool DecoderManager_SendAndWait(DecoderManager* pDecoder, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DWORD dwRetValue;
	BOOL bRetValue;

	// Try to open file
	bRetValue = PostThreadMessage(pDecoder->dwThreadId, Msg, wParam, lParam);

	if (bRetValue == FALSE)
	{

#ifdef _DEBUG

		DWORD hr = GetLastError();
		_RPTF1(_CRT_WARN, "PostThreadMessage Error! Code: %d \n", hr);

#endif
		return false;
	}

	// Wait Engine thread answer
	dwRetValue = WaitForSingleObject(pDecoder->hSyncEvent, 2000);

	if (dwRetValue == WAIT_TIMEOUT)
	{
#if _DEBUG
		wchar_t MessageString[MAX_PATH] = { 0 };

		DecoderManager_MessageToString(Msg, MessageString);

		_RPTFW1(_CRT_WARN, L"Wait Timeout. The Engine thread doesn't respond in time - Msg: %s \n", MessageString);
#endif
		return false;
	}

	// Check for errors
	if (pDecoder->uErrorCode != DECODER_OK)
		return false;

	return true;
}


bool DecoderManager_Initialize(DecoderManager* pDecoder, HWND hWindowHandle)
{
	DWORD dwResult;

	// Check pDecoder Pointer
	if (!pDecoder)
		return false;

	pDecoder->hMainWindowHandle = hWindowHandle;
	pDecoder->hSyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	

	_ASSERT(pDecoder->hSyncEvent);

	if (!pDecoder->hSyncEvent)
		return false;

	// Create Main Thread Loop
	pDecoder->hMainThreadHandle = CreateThread(NULL, 0, DecoderManagerEngineProc, pDecoder, 0, &pDecoder->dwThreadId);

	// Wait Until Main Thread is Ready for messages (2sec for timeout)
	dwResult = WaitForSingleObject(pDecoder->hSyncEvent, 2000);

	// Check if there is an error in thread initialization
	if (dwResult == WAIT_TIMEOUT)
	{
		CloseHandle(pDecoder->hSyncEvent);
		return false;
	}

	// Use this macro for debugging
	// https://docs.microsoft.com/it-it/visualstudio/debugger/macros-for-reporting?view=vs-2019
	_RPTF0(_CRT_WARN, "Running in debug mode...\n");

	return true;
	
}

bool DecoderManager_Deinitialize(DecoderManager* pDecoder)
{
	bool bResult;

	// Request to close the thread
	bResult = DecoderManager_SendAndWait(pDecoder, DECODER_CLOSETHREAD, 0, 0);

	// Destroy Sync Event
	if (pDecoder->hSyncEvent)
	{
		CloseHandle(pDecoder->hSyncEvent);
		pDecoder->hSyncEvent = NULL;
	}
		

	return bResult;
}


bool DecoderManager_OpenFile(DecoderManager *pDecoder, const wchar_t* pFilePath)
{
	// Try to open file
	return DecoderManager_SendAndWait(pDecoder, DECODER_OPENFILE, (WPARAM) pFilePath, 0);	
}

bool DecoderManager_CloseFile(DecoderManager *pDecoder)
{
	// Try to close file
	return DecoderManager_SendAndWait(pDecoder, DECODER_CLOSEFILE, 0, 0);
}

bool DecoderManager_Play(DecoderManager* pDecoder)
{
	// Try Play the file
	return DecoderManager_SendAndWait(pDecoder, DECODER_PLAY, 0, 0);
}

bool DecoderManager_Pause(DecoderManager* pDecoder)
{
	// Try Pause the file
	return DecoderManager_SendAndWait(pDecoder, DECODER_PAUSE, 0, 0);
}

bool DecoderManager_UnPause(DecoderManager* pDecoder)
{
	// Try UnPause the file
	return DecoderManager_SendAndWait(pDecoder, DECODER_UNPAUSE, 0, 0);
}

bool DecoderManager_Stop(DecoderManager* pDecoder)
{
	// Try Stop the file
	return DecoderManager_SendAndWait(pDecoder, DECODER_STOP, 0, 0);
}

bool DecoderManager_Position(DecoderManager* pDecoder)
{
	// Try to Retive the current position in the stream in Ms
	return DecoderManager_SendAndWait(pDecoder, DECODER_GETPOSITION, 0, 0);
}

bool DecoderManager_Seek(DecoderManager* pDecoder, uint64_t uMsNewPosition)
{
	// Try to Seek to the new position in the stream in Ms
	return DecoderManager_SendAndWait(pDecoder, DECODER_SEEK, (WPARAM) uMsNewPosition, 0);
}

bool DecoderManager_SetVolume(DecoderManager* pDecoder, uint8_t *uVolumeValue)
{
	// Try to set the Volume Value
	return DecoderManager_SendAndWait(pDecoder, DECODER_SETVOLUME, (WPARAM)uVolumeValue, 0);
}

bool DecoderManager_GetVolume(DecoderManager* pDecoder, uint8_t* uVolumeValue)
{
	// Try to Get Volume Value
	return DecoderManager_SendAndWait(pDecoder, DECODER_GETVOLUME, (WPARAM)uVolumeValue, 0);
}

bool DecoderManage_GetPlayingBuffer(DecoderManager* pDecoder, int8_t* pByteBuffer, uint16_t* puBufferLen)
{
	// Try to Get Playing Data
	return DecoderManager_SendAndWait(pDecoder, DECODER_GETPLAYINGBUFFER, (WPARAM)pByteBuffer, (LPARAM)puBufferLen);
}

bool DecoderManager_AssignDSPCallback(DecoderManager* pDecoder, DecoderManager_DSP_Callback pCallback)
{
	// Try to Assign Callback
	return DecoderManager_SendAndWait(pDecoder, DECODER_ASSIGNDSPCALLBACK, (WPARAM)pCallback, 0);
}

DWORD WINAPI DecoderManagerEngineProc(LPVOID lpParam)
{
	// Store some local values
	HRESULT hr;
	DecoderManagerEngine DecoderEngine;
	MSG msg;
	bool bContinueLoop;
	DWORD dwEventValue;
		

	// Initialize COM in Single Thread Apartment
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	// Check COM Initialization
	if FAILED(hr)
		return EXIT_FAILURE;

	// New Instance of Circle Buffer
	DecoderEngine.pCircleBuffer = (CircleBuffer*) malloc(sizeof(CircleBuffer));

	if (!DecoderEngine.pCircleBuffer)
		return EXIT_FAILURE;

	// New Instance of Spectrum Buffer
	DecoderEngine.pSpectrumBuffer = (SpectrumBuffer*) malloc(sizeof(SpectrumBuffer));

	if (!DecoderEngine.pSpectrumBuffer)
		return EXIT_FAILURE;

	// Init Circle Buffer
	CircleBuffer_Initialize(DecoderEngine.pCircleBuffer);

	// Init Spectrum Buffer
	SpectrumBuffer_Initialize(DecoderEngine.pSpectrumBuffer);

	// Create Output Notification for Write Buffer
	DecoderEngine.hOutputWriteEvent = CreateEvent(NULL, FALSE, FALSE, L"Output Write Notification Event");

	// Check Event Initialization
	if (!DecoderEngine.hOutputWriteEvent)
		return EXIT_FAILURE;

	// Copy Data from Main Thread
	DecoderEngine.pDecoderManagerData = (DecoderManager*)lpParam;

	// Add Decoders
	StreamWav_Initialize(&DecoderEngine.InputArray[DECODER_WAVDECODER]);
	MediaFoundation_Initialize(&DecoderEngine.InputArray[DECODER_MFDECODER]);

	// Add Outputs
	OutWasapi_Initialize(&DecoderEngine.OutputArray[DECODER_WASAPIOUTPUT]);

	// Use Wasapi as Default Output Engine (TODO: Make this dynamic)
	DecoderEngine.uActiveOutput = DECODER_WASAPIOUTPUT;

	// Reset DSP Callback pointer
	DecoderEngine.pDspCallback = NULL;

	// Notify we are ready to get messages
	SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);

	// Start Loop
	bContinueLoop = true;

	
	do
	{

		// Process Incoming Messages
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
			case DECODER_OPENFILE:
			{
				/*
				wchar_t *pInputFile;

				pInputFile = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));

				if (pInputFile)
				{
					if (wcscpy_s(pInputFile, MAX_PATH, (wchar_t*)msg.wParam) == 0)
					{
						// Try to open the file
						DecoderManagerEngine_OpenFile(&DecoderEngine, pInputFile);
					}

					free(pInputFile);
				}

				*/
				// Try to open the file
				DecoderManagerEngine_OpenFile(&DecoderEngine, (wchar_t*)msg.wParam);



				// Notify Main Thread to continue job
				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			}

			case DECODER_CLOSEFILE:

				DecoderManagerEngine_CloseFile(&DecoderEngine);

				// Notify Main Thread to continue job
				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_CLOSETHREAD:

				// If there is a file open, close then exit
				if (DecoderEngine.bIsFileOpen)
					DecoderManagerEngine_CloseFile(&DecoderEngine);

				// Close Do-While Loop
				bContinueLoop = false;

				break;
			case DECODER_PLAY:

				DecoderManagerEngine_Play(&DecoderEngine);

				// Notify Main Thread to continue job
				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_PAUSE:
				DecoderManagerEngine_Pause(&DecoderEngine);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_UNPAUSE:
				DecoderManagerEngine_UnPause(&DecoderEngine);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_STOP:
				DecoderManagerEngine_Stop(&DecoderEngine);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_GETPOSITION:
				DecoderManagerEngine_CurrentPosition(&DecoderEngine);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_SEEK:
				DecoderManagerEngine_Seek(&DecoderEngine, (uint64_t)msg.wParam);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_SETVOLUME:
				DecoderManagerEngine_SetVolume(&DecoderEngine, (uint8_t*)msg.wParam);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_GETVOLUME:
				DecoderManagerEngine_GetVolume(&DecoderEngine, (uint8_t*)msg.wParam);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_GETPLAYINGBUFFER:
				DecoderManagerEngine_GetPlayingBuffer(&DecoderEngine, (int8_t*)msg.wParam, (uint16_t*)msg.lParam);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;
			case DECODER_ASSIGNDSPCALLBACK:
				DecoderManagerEngine_AssignDSPCallback(&DecoderEngine, (DecoderManager_DSP_Callback)msg.wParam);

				SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);
				break;

			}  // End Switch

		} // End While

		
		// Wait for an output event (100 ms timeout to allow process messages)
		dwEventValue = WaitForSingleObject(DecoderEngine.hOutputWriteEvent, DECODER_OUTPUT_WAIT_TIME);

		// if the event is signal the output need to write data (only if playing)
		if ((dwEventValue == WAIT_OBJECT_0) && (DecoderEngine.bIsOutputPlaying == true))
		{
			if (DecoderEngine.bIsEndOfStream)
			{				
				DecoderManagerEngine_HandleEndOfStream(&DecoderEngine);
			}
			else
			{
				if (DecoderManagerEngine_WriteOutput(&DecoderEngine) == false)
					Sleep(DECODER_OUTPUT_WAIT_TIME); // Wait and retry to write output
			}

			

		}

	} while (bContinueLoop == true);

	// Close Decoders
	StreamWav_Deinitialize(&DecoderEngine.InputArray[DECODER_WAVDECODER]);
	MediaFoundation_Deinitialize(&DecoderEngine.InputArray[DECODER_MFDECODER]);

	// Close Outputs
	OutWasapi_Deinitialize(&DecoderEngine.OutputArray[DECODER_WASAPIOUTPUT]);

	// Close Circle Buffer
	CircleBuffer_Uninitialize(DecoderEngine.pCircleBuffer);

	if (DecoderEngine.pCircleBuffer)
	{
		free(DecoderEngine.pCircleBuffer);
		DecoderEngine.pCircleBuffer = NULL;
	}
		

	// Free Spectrum Buffer
	SpectrumBuffer_Uninitialize(DecoderEngine.pSpectrumBuffer);

	if (DecoderEngine.pSpectrumBuffer)
	{
		free(DecoderEngine.pSpectrumBuffer);
		DecoderEngine.pSpectrumBuffer = NULL;
	}

	// Close Output Notification Event
	CloseHandle(DecoderEngine.hOutputWriteEvent);

	// Close COM
	CoUninitialize();

	// Notify Main Thread to continue job and quit
	SetEvent(DecoderEngine.pDecoderManagerData->hSyncEvent);

	_RPTF0(_CRT_WARN, "Engine Closed Successful! \n");

	return EXIT_SUCCESS;
}

/// <summary>
/// Check if there is a valid decoder for an input file
/// </summary>
/// <returns>The index of the valid decoder</returns>
uint8_t DecoderManagerEngine_GetDecoder(DecoderManagerEngine *pEngine, const wchar_t* pFilePath)
{
	wchar_t *FileExtension;
	uint8_t uDecoderIndex;
	
	// Get file extension
	FileExtension = PathFindExtension(pFilePath);

	// Assign an invalid value
	uDecoderIndex = DECODER_INVALID_DECODER;

	// Scan all installed inputs
	for (uint8_t i = 0; i < DECODER_DECODER_MAX; i++)
	{
		// Scan all extensions
		for (uint8_t j = 0; j < pEngine->InputArray[i].uExtensionsInArray; j++)
		{
			// Compare File Extension with installed input extensions
			if (wcscmp(FileExtension, pEngine->InputArray[i].ExtensionArray[j]) == 0)
			{
				uDecoderIndex = i;
				break; // We found a decoder
			}
		}
	}

	return uDecoderIndex;
}

/// <summary>
/// Check if file exist
/// </summary>
/// <returns>True on Success</returns>
static inline DecoderManagerEngine_FileExist(const wchar_t* pFilePath)
{
	return PathFileExists(pFilePath) == TRUE;
}

/// <summary>
/// Check input stream Format
/// </summary>
/// <returns>True if Wfx is Bad</returns>
bool DecoderManagerEngine_IsBadWfx(uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample)
{
	if ((uSamplerate < 8000) && (uSamplerate > 96000))
		return true;

	if ((uChannels < 1) && (uChannels > 2))
		return true;

	if ((uBitsPerSample < 8) && (uBitsPerSample > 32))
		return true;

	return false;
}

/// <summary>
/// Write PCM Data to Output
/// </summary>
/// <returns>True on success</returns>
bool DecoderManagerEngine_WriteOutput(DecoderManagerEngine* pEngine)
{
	// Check if Engine is Open
	if (!pEngine->bIsFileOpen)
		return false;

	IStreamOutput *pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
	IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];
	CircleBuffer  *pCircle = pEngine->pCircleBuffer;
	SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;
	int8_t * pBuffer;
	uint32_t uBytesToWrite;	
	uint32_t uCircleVaildData, uCircleFreeSpace;
	uint64_t uByteInputPosition, uByteInputDuration;

	bool bResult = false;

	// Check if output is ready to retive data
	if (!pOutput->output_CanWrite(pOutput))
		return false;

	// Retive number of byte to write
	if (!pOutput->output_GetByteCanWrite(pOutput, &uBytesToWrite))
		return false;

	_ASSERT(uBytesToWrite > 0);

	// Check and refill circle buffer (Apply here DSP)
	// only if there is data to read from input
	// otherwise consume circle buffer data, then stop
	pInput->input_Position(pInput, &uByteInputPosition);
	pInput->input_Duration(pInput, &uByteInputDuration);

	if ((uByteInputDuration - uByteInputPosition) > 0)
		DecoderManagerEngine_CheckAndFillCircle(pEngine, uBytesToWrite);
				
	// Get Circle Buffer Status
	pCircle->CircleBuffer_UsedSpace(pCircle, &uCircleVaildData);
	pCircle->CircleBuffer_FreeSpace(pCircle, &uCircleFreeSpace);
	
#ifdef _DEBUG

	if (uCircleVaildData < uBytesToWrite)
		_RPTF2(_CRT_WARN, "CircleBuffer Data < Requested Data uCircleVaildData: %d  ByteToWrite: %d\n", uCircleVaildData, uBytesToWrite);

#endif

	
	// Check if circle buffer has enough data avaiable
	if (uBytesToWrite <= uCircleVaildData)
	{
		pBuffer = (int8_t*)malloc(uBytesToWrite);

		if (pBuffer)
		{
			// Read Data From Buffer
			if (pCircle->CircleBuffer_Read(pCircle, pBuffer, uBytesToWrite) == true)
			{
				// Write to Output Device
				if (pOutput->output_WriteToDevice(pOutput, pBuffer, uBytesToWrite) == true)
				{
					pSpectrum->SpectrumBuffer_Write(pSpectrum, pBuffer, (uint16_t) uBytesToWrite);
					bResult = true;
				}
					
			}				

			free(pBuffer);
		}
	}
	else
	{
		if (uCircleVaildData > 0)
		{
			pBuffer = (int8_t*) malloc(uCircleVaildData);

			if (pBuffer)
			{
				// Read Data From Buffer
				if (pCircle->CircleBuffer_Read(pCircle, pBuffer, uCircleVaildData) == true)
				{
					// Write to Output Device
					if (pOutput->output_WriteToDevice(pOutput, pBuffer, uCircleVaildData) == true)
					{
						_RPTF0(_CRT_WARN, "Consuming Cirle buffer before end of stream... \n");
						bResult = true;
					}

				}

				free(pBuffer);
			}
		}
		else
		{
			pEngine->bIsEndOfStream = true;	
			_RPTF0(_CRT_WARN, "Circle Buffer Is Empty = End Of Stream\n");
		}
	}
	

	return bResult;
}


void DecoderManagerEngine_CheckAndFillCircle(DecoderManagerEngine* pEngine, uint32_t uBytesToWrite)
{
	IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];	
	CircleBuffer* pCircle = pEngine->pCircleBuffer;	
	uint32_t uCircleVaildData, uCircleFreeSpace, uBytesReaded;
	int8_t* pBuffer;

	// Get Circle Buffer Status
	pCircle->CircleBuffer_UsedSpace(pCircle, &uCircleVaildData);
	pCircle->CircleBuffer_FreeSpace(pCircle, &uCircleFreeSpace);


	// Check if we need to refill circle buffer
	if (uCircleVaildData < uBytesToWrite)
	{
		pBuffer = (int8_t*)malloc(uCircleFreeSpace);

		if (pBuffer)
		{
	
#ifdef _DEBUG
			// Read from Input in order to fill circle buffer free space
			if (pInput->input_Read(pInput, pBuffer, uCircleFreeSpace, &uBytesReaded))
			{
				
				// If we have a valid DSP callback process it first
				if (pEngine->pDspCallback)
				{
					DecoderManagerEngine_ProcessDSP(pEngine, pBuffer, uBytesReaded);
				}

				pCircle->CircleBuffer_Write(pCircle, pBuffer, uBytesReaded);
			}
				
			else
				_RPTF0(_CRT_WARN, "Buffer Underrun, Fail to Set End Of Stream Propertly\n");

			_ASSERT(uBytesReaded <= uCircleFreeSpace);
#else
			// Read from Input in order to fill circle buffer free space
			if (pInput->input_Read(pInput, pBuffer, uCircleFreeSpace, &uBytesReaded))
			{

				// If we have a valid DSP callback process it first
				if (pEngine->pDspCallback)
				{
					DecoderManagerEngine_ProcessDSP(pEngine, pBuffer, uBytesReaded);
				}

				pCircle->CircleBuffer_Write(pCircle, pBuffer, uBytesReaded);
			}
				
#endif
	

			free(pBuffer);
		}
	}
}

void DecoderManagerEngine_HandleEndOfStream(DecoderManagerEngine* pEngine)
{

	
	_RPTF0(_CRT_WARN, "Engine: End Of Stream\n");

	// Stop playback
	DecoderManagerEngine_Stop(pEngine);

	// Notify Main Window
	PostMessage(pEngine->pDecoderManagerData->hMainWindowHandle, DECODER_ENDOFSTREAM, 0, 0);
}

/// <summary>
/// Open file. Errors in 
/// pEngine->pDecoderManagerData->uErrorCode
/// </summary>
/// <param name="pEngine">Pointer to DecoderManagerEngine structure</param>
/// <param name="pFilePath">File Path</param>
/// <returns>True on success</returns>
bool DecoderManagerEngine_OpenFile(DecoderManagerEngine* pEngine, const wchar_t* pFilePath)
{
	// Used to store current stream params
	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	uint64_t uDurationMs, uByteDuration;
	uint64_t uCircleByteDuration;
	uint64_t uSpectrumByteDuration;

	// If File is open, close first
	if (pEngine->bIsFileOpen == true)
		DecoderManagerEngine_CloseFile(pEngine);

	// Check if file exist
	if (DecoderManagerEngine_FileExist(pFilePath))
	{

		// Find a proper decoder
		pEngine->uActiveInput = DecoderManagerEngine_GetDecoder(pEngine, pFilePath);


		if (pEngine->uActiveInput != DECODER_INVALID_DECODER)
		{

			// Get pointer from current active input
			IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];
			IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
			CircleBuffer* pCircle = pEngine->pCircleBuffer;
			SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;

			// Try to open file
			if (pInput->input_OpenFile(pInput, pFilePath))
			{
				// Retive Wave Format from input
				pInput->input_GetWaveFormat(pInput, &uSamplerate, &uChannels, &uBitsPerSample);

				// Check WaveFormat
				if (DecoderManagerEngine_IsBadWfx(uSamplerate, uChannels, uBitsPerSample) == false)
				{

					// Request a Buffer Lenght in ms. This variable has the value of 
					// the Buffer Latency after Create Device Function
					pEngine->uOutputMaxLatency = DECODER_OUTPUT_MAX_LATENCY;
					pOutput->hOutputWriteEvent = pEngine->hOutputWriteEvent;

					// Prepare Output
					if (pOutput->output_CreateDevice(pOutput,
						uSamplerate, uChannels, uBitsPerSample, &pEngine->uOutputMaxLatency))
					{
						// Initialize vars
						uDurationMs = 0;
						uByteDuration = 0;

						// Store Stream Information			
						pEngine->pDecoderManagerData->uCurrentSamplerate = uSamplerate;
						pEngine->pDecoderManagerData->uCurrentChannels = uChannels;
						pEngine->pDecoderManagerData->uCurrentBitsPerSample = uBitsPerSample;
						pEngine->pDecoderManagerData->uCurrentBlockAlign = (uBitsPerSample * uChannels) / 8;
						pEngine->pDecoderManagerData->uCurrentAvgBytesPerSec = pEngine->pDecoderManagerData->uCurrentBlockAlign * uSamplerate;
						pEngine->pDecoderManagerData->uCurrentPositionMs = 0;

						// Get Current Stream duration in Ms
						if (pInput->input_Duration(pInput, &uByteDuration) == true)
							DecoderManagerEngine_ByteToMs(pEngine,
								uByteDuration,
								&uDurationMs);

						pEngine->pDecoderManagerData->uCurrentDurationMs = uDurationMs;
						pEngine->pDecoderManagerData->bCurrentIsStreamSeekable = pInput->input_IsStreamSeekable(pInput);



						// Alloc Circle buffer
						DecoderManagerEngine_MsToByte(pEngine, DECODER_CBUFFER_LEN,  &uCircleByteDuration);

					
						if (pCircle->CircleBuffer_Create(pCircle, (uint32_t) uCircleByteDuration))
						{	
							// Create Spectrum buffer with the same latency of output									
							DecoderManagerEngine_MsToByte(pEngine, DECODER_OUTPUT_MAX_LATENCY, &uSpectrumByteDuration);
							if (pSpectrum->SpectrumBuffer_Create(pSpectrum, (uint16_t)uSpectrumByteDuration))
							{
								// Success
								pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
								pEngine->bIsEndOfStream = false;
								pEngine->bIsFileOpen = true;

								return true;
							}
							else
							{
								// If Create fail close file and return circle buffer error				
								pInput->input_CloseFile(pInput);
								pEngine->pDecoderManagerData->uErrorCode = DECODER_CIRCLEBUFFERERROR;
							}

						}
						else
						{
							// If Create fail close file and return circle buffer error				
							pInput->input_CloseFile(pInput);
							pEngine->pDecoderManagerData->uErrorCode = DECODER_CIRCLEBUFFERERROR;
						}
	
					}
					else
					{
						// If Create device fail close and return an error					
						pInput->input_CloseFile(pInput);
						pEngine->pDecoderManagerData->uErrorCode = DECODER_OUTPUTERROR;
					}
				}
				else
				{
					// If we have a bad WFX close and return an error					
					pInput->input_CloseFile(pInput);
					pEngine->pDecoderManagerData->uErrorCode = DECODER_BADWAVEFORMAT;
				}
			}
		}
		else
		{
			pEngine->pDecoderManagerData->uErrorCode = DECODER_FILENOTSUPPORTED;
		}
	}
	else
	{
		pEngine->pDecoderManagerData->uErrorCode = DECODER_FILENOTFOUND;
	}

	return false;
}

/// <summary>
/// Close Current file
/// </summary>
void DecoderManagerEngine_CloseFile(DecoderManagerEngine* pEngine)
{
	if (pEngine->bIsFileOpen == true)
	{
		IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
		CircleBuffer* pCircle = pEngine->pCircleBuffer;
		SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;

		// Stop Playback
		if (pOutput->output_IsPlaying(pOutput))
			DecoderManagerEngine_Stop(pEngine);


		// Unprepare Output
		pOutput->output_CloseDevice(pOutput);
		pOutput->hOutputWriteEvent = NULL;

		// Close File
		pInput->input_CloseFile(pInput);

		// Close Circle Buffer
		pCircle->CircleBuffer_Destroy(pCircle);

		// Close Spectrum Buffer
		pSpectrum->SpectrumBuffer_Destroy(pSpectrum);
	}

	// Reset Vars
	pEngine->bIsFileOpen = false;
	pEngine->bIsEndOfStream = false;
	pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
}

void DecoderManagerEngine_Play(DecoderManagerEngine* pEngine)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{		
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];		

		// Write Pre-Buffer Before Play to avoid gliches
		if (!pOutput->output_IsPlaying(pOutput))
		{
			if (DecoderManagerEngine_WriteOutput(pEngine))
			{
				_RPTF0(_CRT_WARN, "Change Status: Playing\n");

				// Play
				pOutput->output_DevicePlay(pOutput);
				pEngine->bIsOutputPlaying = true;
				pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
			}
			
			else
			{
				// If we fail to write output flush buffers and try to
				// write again
				if (pOutput->output_FlushBuffers(pOutput))
				{
					if (DecoderManagerEngine_WriteOutput(pEngine))
					{
						_RPTF0(_CRT_WARN, "Flush Buffer + Change Status: Playing\n");

						// Play
						pOutput->output_DevicePlay(pOutput);
						pEngine->bIsOutputPlaying = true;
						pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
					}
				}


			}
			
		}
	}
}

void DecoderManagerEngine_Stop(DecoderManagerEngine* pEngine)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{
		IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
		CircleBuffer* pCircle = pEngine->pCircleBuffer;
		SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;

		
		if (pOutput->output_IsPlaying(pOutput))
		{
			_RPTF0(_CRT_WARN, "Change Status: Stop\n");

			pOutput->output_DeviceStop(pOutput);					
			pInput->input_Seek(pInput, 0, BEGIN);

			// Flush Spectrum Buffer
			pSpectrum->SpectrumBuffer_Reset(pSpectrum);

			// Flush Circle Buffer
			pCircle->CircleBuffer_Reset(pCircle);

			// Flush Output
			pOutput->output_FlushBuffers(pOutput);

			pEngine->bIsEndOfStream = false;
			pEngine->bIsOutputPlaying = false;
			pEngine->pDecoderManagerData->uCurrentPositionMs = 0;
			pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
		}
		else
		{
			_RPTF0(_CRT_WARN, "Change Status: Stop from Pause\n");
			
			pInput->input_Seek(pInput, 0, BEGIN);

			// Flush Spectrum Buffer
			pSpectrum->SpectrumBuffer_Reset(pSpectrum);

			// Flush Circle Buffer
			pCircle->CircleBuffer_Reset(pCircle);

			// Flush Output
			pOutput->output_FlushBuffers(pOutput);

			pEngine->bIsEndOfStream = false;
			pEngine->bIsOutputPlaying = false;
			pEngine->pDecoderManagerData->uCurrentPositionMs = 0;
			pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
		}
			
	}
}

void DecoderManagerEngine_Pause(DecoderManagerEngine* pEngine)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];

		_RPTF0(_CRT_WARN, "Change Status: Pause\n");
		pOutput->output_DevicePause(pOutput, true);
		pEngine->bIsOutputPlaying = false;
		pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
	}
	else
	{
		pEngine->pDecoderManagerData->uErrorCode = DECODER_DEVICENOTOPEN;
	}
}

void DecoderManagerEngine_UnPause(DecoderManagerEngine* pEngine)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];

		_RPTF0(_CRT_WARN, "Change Status: UnPause\n");
		pOutput->output_DevicePause(pOutput, false);
		pEngine->bIsOutputPlaying = true;
		pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
	}
}

bool DecoderManagerEngine_Seek(DecoderManagerEngine* pEngine, uint64_t uMsNewPosition)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{
		IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];
		IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
		CircleBuffer* pCircle = pEngine->pCircleBuffer;
		SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;
		uint64_t uBytePosition;

		if (pOutput->output_IsPlaying(pOutput))
		{

			// Check if Stream is Seekable
			if (pInput->input_IsStreamSeekable(pInput))
			{
				pEngine->bIsOutputPlaying = false;

				// Stop Playing
				pOutput->output_DeviceStop(pOutput);
				

				// Convert Ms to Byte Position
				DecoderManagerEngine_MsToByte(pEngine, uMsNewPosition, &uBytePosition);

				_ASSERT(uBytePosition >= 0);

				// Perform Seek Operation
				pInput->input_Seek(pInput, uBytePosition, BEGIN);

				// Flush Buffers
				pSpectrum->SpectrumBuffer_Reset(pSpectrum);
				pCircle->CircleBuffer_Reset(pCircle);
				pOutput->output_FlushBuffers(pOutput);


				// Re-Write Buffer and Play
				if (DecoderManagerEngine_WriteOutput(pEngine))
				{
					// Play
					pOutput->output_DevicePlay(pOutput);
					pEngine->pDecoderManagerData->uCurrentPositionMs = uMsNewPosition;
					pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;

					pEngine->bIsOutputPlaying = true;
					return true;
				}
			}
		}

	}

	return false;
}

void DecoderManagerEngine_CurrentPosition(DecoderManagerEngine* pEngine)
{
	// Check if file is open
	if (pEngine->bIsFileOpen == true)
	{
		IStreamInput* pInput = &pEngine->InputArray[pEngine->uActiveInput];		

		uint64_t uPositionInBytes, uPositionInMs;

		// Get Position in Byte
		if (pInput->input_Position(pInput, &uPositionInBytes))
		{

			// Convert Position In Milliseconds
			DecoderManagerEngine_ByteToMs(pEngine, uPositionInBytes, &uPositionInMs);

			// Adjust Position removing Circle Buffer and Output Latency
			uPositionInMs = uPositionInMs - (DECODER_CBUFFER_LEN + DECODER_OUTPUT_MAX_LATENCY);
	

			// Assign the new position
			pEngine->pDecoderManagerData->uCurrentPositionMs = uPositionInMs;
			pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
		}

	}
	else
	{
		pEngine->pDecoderManagerData->uCurrentPositionMs = 0;
	}

}

void DecoderManagerEngine_SetVolume(DecoderManagerEngine* pEngine, uint8_t *uVolumeValue)
{
	IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];

	if (!pEngine->bIsFileOpen)
		pEngine->pDecoderManagerData->uErrorCode = DECODER_DEVICENOTOPEN;

	if (pOutput->output_DeviceVolume(pOutput, uVolumeValue, true))
		pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
}

void DecoderManagerEngine_GetVolume(DecoderManagerEngine* pEngine, uint8_t *uVolumeValue)
{
	IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];

	if (!pEngine->bIsFileOpen)
		pEngine->pDecoderManagerData->uErrorCode = DECODER_DEVICENOTOPEN;

	if (pOutput->output_DeviceVolume(pOutput, uVolumeValue, false))
		pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;

}


void DecoderManagerEngine_GetPlayingBuffer(DecoderManagerEngine* pEngine, int8_t* pByteBuffer, uint16_t* puBufferLen)
{
	IStreamOutput* pOutput = &pEngine->OutputArray[pEngine->uActiveOutput];
	SpectrumBuffer* pSpectrum = pEngine->pSpectrumBuffer;

	float fPlayTimeMs, fWriteTimeMs, fLatency;
	uint32_t uPositionInBytes;

	_ASSERT(*puBufferLen > 0);

	if (pEngine->bIsFileOpen)
	{
		if (pOutput->output_IsPlaying(pOutput))
		{		

			pOutput->output_GetWriteTime(pOutput, &fWriteTimeMs);
			pOutput->output_GetPlayTime(pOutput, &fPlayTimeMs);

			// Calculate Real buffer Latency
			fLatency = fWriteTimeMs - fPlayTimeMs;

			// If Latency is bigger than buffer size, consider this gap 
			// to adjust the position of play time ms
			if (fLatency > DECODER_OUTPUT_MAX_LATENCY_F)
			{
				fPlayTimeMs += fLatency - DECODER_OUTPUT_MAX_LATENCY_F; // TODO: This is userful? test! test!
			}


			// Calculate module
			fPlayTimeMs = fmodf(fPlayTimeMs, DECODER_OUTPUT_MAX_LATENCY_F);
			fWriteTimeMs = fmodf(fWriteTimeMs, DECODER_OUTPUT_MAX_LATENCY_F);

			// Round to a nearest int
			fPlayTimeMs = rintf(fPlayTimeMs);

			// Cast to an unsigned int value and convert to a byte position
			uPositionInBytes = (pEngine->pDecoderManagerData->uCurrentAvgBytesPerSec * (uint32_t)fPlayTimeMs) / 1000;

			// Align to PCM blocks
			uPositionInBytes = uPositionInBytes - (uPositionInBytes % pEngine->pDecoderManagerData->uCurrentBlockAlign);

			// Read Spectrum Buffer
			pSpectrum->SpectrumBuffer_ReadFrom(pSpectrum, pByteBuffer, (uint16_t)uPositionInBytes, *puBufferLen);

			pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;

		}
		else
		{
			pEngine->pDecoderManagerData->uErrorCode = DECODER_DEVICENOTOPEN;
		}

		
	}
	else
	{
		pEngine->pDecoderManagerData->uErrorCode = DECODER_DEVICENOTOPEN;
	}

}


void DecoderManagerEngine_AssignDSPCallback(DecoderManagerEngine* pEngine, DecoderManager_DSP_Callback pCallback)
{
	if (pCallback)
	{
		pEngine->pDspCallback = pCallback;
		pEngine->pDecoderManagerData->uErrorCode = DECODER_OK;
	}
	else
	{
		pEngine->pDecoderManagerData->uErrorCode = DECODER_BADPTR;
	}
}


void DecoderManagerEngine_ProcessDSP(DecoderManagerEngine* pEngine, int8_t* pByteBuffer, uint32_t uCount)
{
	uint32_t i, uSamples;
	float* fLeftSamples;
	float* fRightSamples;

	// Convert from Byte Count to Samples Count
	uSamples = uCount / pEngine->pDecoderManagerData->uCurrentBlockAlign;

	// lrintf


	switch (pEngine->pDecoderManagerData->uCurrentChannels)
	{
	case 1:
		switch (pEngine->pDecoderManagerData->uCurrentBitsPerSample)
		{
		case 8:
		{
			fLeftSamples = (float*)malloc(uSamples * sizeof(float));

			if (fLeftSamples)
			{
				// Convert Samples to float
				for (i = 0; i < uSamples; i++)
				{
					fLeftSamples[i] = (float)(pByteBuffer[i]) / INT8_MAX;
				}

				pEngine->pDspCallback(fLeftSamples, NULL, uSamples);


				// Convert from Float samples to Byte Samples
				for (i = 0; i < uSamples; i++)
				{
					fLeftSamples[i] = min(fLeftSamples[i], 1.0f);
					fLeftSamples[i] = max(fLeftSamples[i], -1.0f);

					pByteBuffer[i] = (int8_t)(fLeftSamples[i] * INT8_MAX);
				}

				free(fLeftSamples);
				fLeftSamples = NULL;
			}


			break;
		}
		case 16:
		{	

			fLeftSamples = (float*)malloc(uSamples * sizeof(float));

			if (fLeftSamples)
			{
				// Convert Samples to float
				for (i = 0; i < uSamples; i++)
				{
					fLeftSamples[i] = (float)(pByteBuffer[i * 2 + 0] & (pByteBuffer[i * 2 + 1] << 8)) / INT16_MAX;
				}

				pEngine->pDspCallback(fLeftSamples, NULL, uSamples);


				// Convert from Float samples to Byte Samples
				for (i = 0; i < uSamples; i++)
				{
					fLeftSamples[i] = min(fLeftSamples[i], 1.0f);
					fLeftSamples[i] = max(fLeftSamples[i], -1.0f);

					fLeftSamples[i] *= INT16_MAX;


					pByteBuffer[i * 2 + 0] = LOBYTE(fLeftSamples[i]);
					pByteBuffer[i * 2 + 1] = HIBYTE(fLeftSamples[i]);					
				}

				free(fLeftSamples);
				fLeftSamples = NULL;
			}

			break;
		}
		}
	case 2:
		switch (pEngine->pDecoderManagerData->uCurrentBitsPerSample)
		{
		case 8:
		{
			/*
			float fLeft, fRight;

			for (i = 0; i < uSamples; i++)
			{
				fLeft = (float)(pByte[i * 2 + 0]) / INT8_MAX;
				fRight = (float)(pByte[i * 2 + 1]) / INT8_MAX;

		
			}
			*/
			break;
		}
		case 16:
		{

			fLeftSamples = (float*)malloc(uSamples * sizeof(float));
			fRightSamples = (float*)malloc(uSamples * sizeof(float));

			_ASSERT(uSamples > 0);

			if ((fLeftSamples) && (fRightSamples))
			{
				ult_16b_2c_bytes_to_float_array(pByteBuffer, fLeftSamples, fRightSamples, uSamples);
				pEngine->pDspCallback(fLeftSamples, fRightSamples, uSamples);
				ult_16b_2c_float_to_bytes_array(fLeftSamples, fRightSamples, pByteBuffer, uSamples);

				free(fLeftSamples);
				fLeftSamples = NULL;

				free(fRightSamples);
				fRightSamples = NULL;
			}
			else
			{
				_RPTF0(_CRT_WARN, "Fail\n");
			}

			break;
		}
		}
	}
}


static inline void DecoderManagerEngine_ByteToMs(DecoderManagerEngine* pEngine, uint64_t uBytes, uint64_t* puMs)
{
		(*puMs) = uBytes / pEngine->pDecoderManagerData->uCurrentAvgBytesPerSec * 1000;
}

static inline void DecoderManagerEngine_MsToByte(DecoderManagerEngine* pEngine, uint64_t uMs, uint64_t* puBytes)
{
		(*puBytes) = uMs * pEngine->pDecoderManagerData->uCurrentAvgBytesPerSec / 1000;

		// Align to Block Align
		(*puBytes) = (*puBytes) - ((*puBytes) % pEngine->pDecoderManagerData->uCurrentBlockAlign);			
}

