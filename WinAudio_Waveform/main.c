
#include "pch.h"
#include "sndfile.h"

// Define the number of temp string 
// where a single tag is stored
#define WA_SNDLIB_TEMP_TAG_LEN 255

// Export Function Name
#define EXPORTS _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);


bool WA_Sndlib_New (WA_Input* This);		
void WA_Sndlib_Delete(WA_Input* This);

uint32_t WA_Sndlib_Open(WA_Input* This, const wchar_t* lpwFilePath);
void WA_Sndlib_Close(WA_Input* This);

bool WA_Sndlib_IsStreamSeekable(WA_Input* This);

uint64_t WA_Sndlib_Duration(WA_Input* This);
uint64_t WA_Sndlib_Position(WA_Input* This);

uint32_t WA_Sndlib_Seek(WA_Input* This, uint64_t uNewPosition);
uint32_t WA_Sndlib_Read(WA_Input* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puReadedBytes);

void WA_Sndlib_ConfigDialog(WA_Input* This, HWND hParent);
void WA_Sndlib_PluginDialog(WA_Input* This, HWND hParent);

uint32_t WA_Sndlib_GetMetadata(WA_Input* This, WA_AudioMetadata* pMetadata);
uint32_t WA_Sndlib_GetFormat(WA_Input* This, WA_AudioFormat* pFormat);

uint32_t WA_Sndlib_GetFileInfo(WA_Input* This, const wchar_t* lpwFilePath, WA_AudioFormat* pFormat, WA_AudioMetadata* pMetadata, uint64_t* puDuration);


// Module Header and VTable
static WA_Input SndlibVTable = {

	{ // Start Common Header ---------------> 
	WA_PLUGINTYPE_INPUT,	// Plugin Type
	1U,					// Version
	L"WinAudio SndLib Input\0", // Description
	NULL,					// WinAudio HWND
	false					// Enabled or Disabled
	}, // End Common Header <-----------------
	L"SNDLIB Audio Files\0", // Filter Name
	L"*.wav;*.aiff;*.au;*.flac;*.ogg\0", // Filter Extensions
	WA_Sndlib_New, // WA_Input_New
	WA_Sndlib_Delete, // WA_Input_Delete
	WA_Sndlib_Open, // WA_Input_Open
	WA_Sndlib_Close, // WA_Input_Close
	WA_Sndlib_IsStreamSeekable, // WA_Input_IsStreamSeekable
	WA_Sndlib_Duration, // WA_Input_Duration
	WA_Sndlib_Position, // WA_Input_Position
	WA_Sndlib_Seek, // WA_Input_Seek
	WA_Sndlib_Read, // WA_Input_Read
	WA_Sndlib_ConfigDialog, // WA_Input_ConfigDialog
	WA_Sndlib_PluginDialog, // WA_Input_PluginDialog
	WA_Sndlib_GetMetadata, // WA_Input_GetMetadata
	WA_Sndlib_GetFormat, // WA_Input_GetFormat
	WA_Sndlib_GetFileInfo,
	NULL  // hPluginData = Create After with a Malloc
};


typedef struct TagWA_SndInstance
{
	bool bFileIsOpen;
	bool bStreamIsSeekable;
	bool bCastToSigned;
	SNDFILE* pSndFile;
	WA_AudioFormat Format;
	WA_AudioMetadata Metadata;
	uint64_t uDuration;
	uint64_t uPosition;
	wchar_t* pCurrentPath;
} WA_SndInstance;

WA_HMODULE* WA_Plugin_GetHeader(void)
{
	return (WA_HMODULE*)&SndlibVTable;
}

static uint32_t WA_Sndlib_ReadFormat(SF_INFO* Info, WA_AudioFormat* pFormat)
{
	pFormat->uSamplerate = Info->samplerate;
	pFormat->uChannels = Info->channels;


	switch (Info->format & SF_FORMAT_SUBMASK)
	{
	case SF_FORMAT_PCM_U8: // Perform Unsigned to Signed conversion later
		pFormat->uBitsPerSample = 8;
		pFormat->uSampleType = WA_UNSIGNED_SAMPLE;
		break;
	case SF_FORMAT_PCM_S8:
		pFormat->uBitsPerSample = 8;
		pFormat->uSampleType = WA_SIGNED_SAMPLE;
		break;
	case SF_FORMAT_PCM_16:
		pFormat->uBitsPerSample = 16;
		pFormat->uSampleType = WA_SIGNED_SAMPLE;
		break;
	case SF_FORMAT_PCM_24:
		pFormat->uBitsPerSample = 24;
		pFormat->uSampleType = WA_SIGNED_SAMPLE;
		break;
	case SF_FORMAT_PCM_32:
		pFormat->uBitsPerSample = 32;
		pFormat->uSampleType = WA_SIGNED_SAMPLE;
		break;
	default:
		return WA_ERROR_FILENOTSUPPORTED;
	}

	pFormat->uBlockAlign = (pFormat->uBitsPerSample / 8) * pFormat->uChannels;
	pFormat->uAvgBytesPerSec = pFormat->uSamplerate * pFormat->uBlockAlign;

	return WA_OK;
}

static uint32_t WA_Sndlib_ReadMetadata(SNDFILE* pFile, WA_AudioMetadata* pMetadata)
{
	wchar_t pTempStr[WA_SNDLIB_TEMP_TAG_LEN];
	const char* pRetStr;
	int uConvertedChars;

	if (!pMetadata)
		return WA_ERROR_BADPTR;

	if(!pFile)
		return WA_ERROR_BADPTR;

	pRetStr = sf_get_string(pFile, SF_STR_ARTIST);

	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, pRetStr, -1, pTempStr, WA_SNDLIB_TEMP_TAG_LEN);
	if (uConvertedChars > 0)
		wcsncpy_s(pMetadata->Artist, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

	pRetStr = sf_get_string(pFile, SF_STR_TITLE);

	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, pRetStr, -1, pTempStr, WA_SNDLIB_TEMP_TAG_LEN);
	if (uConvertedChars > 0)
		wcsncpy_s(pMetadata->Title, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

	pRetStr = sf_get_string(pFile, SF_STR_ALBUM);

	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, pRetStr, -1, pTempStr, WA_SNDLIB_TEMP_TAG_LEN);
	if (uConvertedChars > 0)
		wcsncpy_s(pMetadata->Album, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

	pRetStr = sf_get_string(pFile, SF_STR_GENRE);

	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, pRetStr, -1, pTempStr, WA_SNDLIB_TEMP_TAG_LEN);
	if (uConvertedChars > 0)
		wcsncpy_s(pMetadata->Genre, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

	return WA_OK;
}

bool WA_Sndlib_New(WA_Input* This)
{
	WA_SndInstance* pInstance;


	pInstance = (WA_SndInstance*)malloc(sizeof(WA_SndInstance));

	if (!pInstance)
		return false;

	ZeroMemory(pInstance, sizeof(WA_SndInstance));

	This->hPluginData = pInstance;

	return true;
}

void WA_Sndlib_Delete(WA_Input* This)
{
	WA_SndInstance* pInstance = (WA_SndInstance*) This->hPluginData;

	if (pInstance)
		free(pInstance);

	This->hPluginData = NULL;
}


uint32_t WA_Sndlib_Open(WA_Input* This, const wchar_t* lpwFilePath)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;
	SF_INFO Info;
	int nByteRate;
	size_t nPathLen;

	if (pInstance->bFileIsOpen)
		WA_Sndlib_Close(This);

	ZeroMemory(&Info, sizeof(SF_INFO));
	
	pInstance->pSndFile = sf_wchar_open(lpwFilePath, SFM_READ, &Info);

	if (!pInstance->pSndFile)
		return WA_ERROR_FILENOTSUPPORTED;

	nByteRate = sf_current_byterate(pInstance->pSndFile);

	if (nByteRate == 0)
	{
		sf_close(pInstance->pSndFile);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	if (WA_Sndlib_ReadFormat(&Info, &pInstance->Format) != WA_OK)
	{
		sf_close(pInstance->pSndFile);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	pInstance->bCastToSigned = ((Info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_U8) ? true : false;
	pInstance->bStreamIsSeekable = (Info.seekable > 0) ? true : false;	
	pInstance->uPosition = 0U;
	pInstance->uDuration = (Info.frames / Info.samplerate) * 1000;

	nPathLen = wcslen(lpwFilePath) + 1; // Inlcude Null-Terminating char
	pInstance->pCurrentPath = NULL;

	if (nPathLen > 0)
	{
		pInstance->pCurrentPath = calloc(nPathLen, sizeof(wchar_t));

		if (pInstance->pCurrentPath)
			wcscpy_s(pInstance->pCurrentPath, nPathLen, lpwFilePath);
	}

	pInstance->bFileIsOpen = true;

	return WA_OK;
}

void WA_Sndlib_Close(WA_Input* This)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	if (!pInstance->bFileIsOpen)
		return;

	sf_close(pInstance->pSndFile);

	if (pInstance->pCurrentPath)
		free(pInstance->pCurrentPath);

	pInstance->pCurrentPath = NULL;
	pInstance->uPosition = 0U;
	pInstance->uDuration = 0U;
	pInstance->bFileIsOpen = false;

}

bool WA_Sndlib_IsStreamSeekable(WA_Input* This)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	return pInstance->bStreamIsSeekable;
}

uint64_t WA_Sndlib_Duration(WA_Input* This)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	return pInstance->uDuration;
}

uint64_t WA_Sndlib_Position(WA_Input* This)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	return pInstance->uPosition;
}

uint32_t WA_Sndlib_Seek(WA_Input* This, uint64_t uNewPosition)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;
	sf_count_t nNewOffset, nOffset;

	if (uNewPosition > pInstance->uDuration)
		return WA_ERROR_BADPARAM;

	nOffset = (uNewPosition / 1000) * pInstance->Format.uSamplerate;

	nNewOffset = sf_seek(pInstance->pSndFile, nOffset, SF_SEEK_SET);

	pInstance->uPosition = (nNewOffset / pInstance->Format.uSamplerate) * 1000;

	return WA_OK;

}

uint32_t WA_Sndlib_Read(WA_Input* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puReadedBytes)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;	
	sf_count_t nFrameToRead, nFrameReaded;
	int32_t* Temp;
	uint32_t uIndex;
	
	nFrameToRead = uBufferLen / pInstance->Format.uBlockAlign;

	if (nFrameToRead == 0)
		return WA_ERROR_ENDOFFILE;

	Temp = calloc(nFrameToRead * pInstance->Format.uChannels, sizeof(int32_t));

	if (!Temp)
		return WA_ERROR_ENDOFFILE;

	nFrameReaded = sf_readf_int(pInstance->pSndFile, Temp, nFrameToRead);

	if (nFrameReaded == 0)
	{
		free(Temp);
		return WA_ERROR_ENDOFFILE;
	}		

	(*puReadedBytes) = (uint32_t)nFrameReaded * pInstance->Format.uBlockAlign;

	pInstance->uPosition += (nFrameReaded * 1000) / pInstance->Format.uSamplerate;

	uIndex = 0U;
	nFrameReaded *= pInstance->Format.uChannels;

	for (sf_count_t i = 0U; i < nFrameReaded; i++)
	{
		switch (pInstance->Format.uBitsPerSample)
		{
		case 8:
		{
			if (pInstance->bCastToSigned)
			{
				uint8_t Val = (uint8_t)(Temp[i] >> 24);
				int8_t sVal = (int8_t)(Val - 128);

				memcpy(pBuffer + uIndex, &sVal, sizeof(int8_t));
			}
			else
			{
				int8_t Val = (int8_t)(Temp[i] >> 24);

				memcpy(pBuffer + uIndex, &Val, sizeof(int8_t));				
			}	

			uIndex += 1U;

			break;
		}
		case 16:
		{
			int16_t Val = (int16_t) (Temp[i] >> 16);

			memcpy(pBuffer + uIndex, &Val, sizeof(int16_t));
			uIndex += 2U;
			break;
		}
		case 24:
		{
			int32_t Val = (int32_t)(Temp[i] >> 8);

			memcpy(pBuffer + uIndex, &Val, 3);
			uIndex += 3U;
			break;
		}
		case 32:
		{
			int32_t Val = (int32_t)(Temp[i]);

			memcpy(pBuffer + uIndex, &Val, sizeof(int32_t));
			uIndex += 4U;
			break;
		}
		}
	}


	free(Temp);

	return WA_OK;
}

void WA_Sndlib_ConfigDialog(WA_Input* This, HWND hParent)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	// TODO: Implement Config Dialog
}

void WA_Sndlib_PluginDialog(WA_Input* This, HWND hParent)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	// TODO: Implement Plugin Dialog
}

uint32_t WA_Sndlib_GetMetadata(WA_Input* This, WA_AudioMetadata* pMetadata)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	return WA_Sndlib_ReadMetadata(pInstance->pSndFile, pMetadata);
}

uint32_t WA_Sndlib_GetFormat(WA_Input* This, WA_AudioFormat* pFormat)
{
	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;

	if (!pInstance->bFileIsOpen)
		return WA_ERROR_FAIL;

	memcpy(pFormat, &pInstance->Format, sizeof(WA_AudioFormat));

	return WA_OK;
}

uint32_t WA_Sndlib_GetFileInfo(WA_Input* This, const wchar_t* lpwFilePath, WA_AudioFormat* pFormat, WA_AudioMetadata* pMetadata, uint64_t* puDuration)
{

	WA_SndInstance* pInstance = (WA_SndInstance*)This->hPluginData;
	SF_INFO Info;
	SNDFILE* pFile;

	// If path == current opened file use instance data
	if (pInstance->pCurrentPath)
	{
		if (_wcsicmp(pInstance->pCurrentPath, lpwFilePath) == 0)
		{
			if (WA_Sndlib_GetMetadata(This, pMetadata) != WA_OK)
				return WA_ERROR_BADPARAM;

			if (WA_Sndlib_GetFormat(This, pFormat) != WA_OK)
				return WA_ERROR_BADPARAM;

			if (puDuration)
				(*puDuration) = pInstance->uDuration;

			return WA_OK;
		}
	}

	ZeroMemory(&Info, sizeof(SF_INFO));

	pFile = sf_wchar_open(lpwFilePath, SFM_READ, &Info);

	if (!pFile)
		return WA_ERROR_FILENOTSUPPORTED;	

	int byterate = sf_current_byterate(pInstance->pSndFile);

	if (byterate == 0)
	{
		sf_close(pInstance->pSndFile);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	if(WA_Sndlib_ReadFormat(&Info, pFormat) != WA_OK)
		return WA_ERROR_FILENOTSUPPORTED;

	WA_Sndlib_ReadMetadata(pFile, pMetadata);
	
	(*puDuration) = (Info.frames / Info.samplerate) * 1000;

	return WA_OK;
}