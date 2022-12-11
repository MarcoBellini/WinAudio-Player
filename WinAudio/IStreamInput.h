#ifndef ISTREAMINPUT_H
#define ISTREAMINPUT_H


/* Max number of extensions that can be stored in
   the Extension array of _IStreamInput stuct
*/
#define INPUT_MAX_EXTENSIONS 10

/* Seek mode in Input module Seek function*/
enum SEEK_ORIGIN
{
	BEGIN = 0,
	CURRENT,
	END
};

/* Forward Reference*/
struct tagIStreamInput;
typedef struct tagIStreamInput* INPUT_HANDLE;


/* Definition of function pointers */
typedef bool (*pInput_OpenFile)(INPUT_HANDLE pHandle, const wchar_t* pFilePath);
typedef bool (*pInput_CloseFile)(INPUT_HANDLE pHandle);
typedef bool (*pInput_Seek)(INPUT_HANDLE pHandle, uint64_t uBytesNewPosition, enum SEEK_ORIGIN seekOrigin);
typedef bool (*pInput_Position)(INPUT_HANDLE pHandle, uint64_t* uBytesPosition);
typedef bool (*pInput_Duration)(INPUT_HANDLE pHandle, uint64_t* uBytesDuration);
typedef bool (*pInput_Read)(INPUT_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
typedef bool (*pInput_GetWaveFormat)(INPUT_HANDLE pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
typedef bool (*pInput_IsStreamSeekable)(INPUT_HANDLE pHandle);

/* Template for all input modules */
typedef struct tagIStreamInput
{
	
	/* Pointer to module functions */
	pInput_OpenFile input_OpenFile;
	pInput_CloseFile input_CloseFile;
	pInput_Seek input_Seek;
	pInput_Position input_Position;
	pInput_Duration input_Duration;
	pInput_Read input_Read;
	pInput_GetWaveFormat input_GetWaveFormat;
	pInput_IsStreamSeekable input_IsStreamSeekable;

	/* Store Module Extensions (MAX 10 per module) */
	wchar_t ExtensionArray[INPUT_MAX_EXTENSIONS][6];
	uint8_t uExtensionsInArray;
	
	/* Allow to store private module Data*/
	void* pModulePrivateData; 
	

} IStreamInput;


/* WAV Stream Reader Input module*/
bool StreamWav_Initialize(IStreamInput* pStreamInput);
bool StreamWav_Deinitialize(IStreamInput* pStreamInput);

/* Media Foundation Stream Input Module
*  Supported file:
*  https://docs.microsoft.com/en-us/windows/win32/medfound/supported-media-formats-in-media-foundation
*/
bool MediaFoundation_Initialize(IStreamInput* pStreamInput);
bool MediaFoundation_Deinitialize(IStreamInput* pStreamInput);



#endif
