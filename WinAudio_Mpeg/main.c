
#include "pch.h"
#include "mpg123.h"
#include "WA_ID3_Genres.h"

// Length in Chars used in Temp string var to read id3 tags
#define WA_MPEG_TEMP_TAG_LEN 255

// Export Function Name
#define EXPORTS _declspec(dllexport)

EXPORTS WA_HMODULE* WA_Plugin_GetHeader(void);

// Instance Functions
bool WA_Mpeg_New(WA_Input* This);
void WA_Mpeg_Delete(WA_Input* This);
uint32_t WA_Mpeg_Open(WA_Input* This, const wchar_t* lpwFilePath);
void WA_Mpeg_Close(WA_Input* This);
bool WA_Mpeg_IsStreamSeekable(WA_Input* This);
uint64_t WA_Mpeg_Duration(WA_Input* This);
uint64_t WA_Mpeg_Position(WA_Input* This);
uint32_t WA_Mpeg_Seek(WA_Input* This, uint64_t uNewPosition);
uint32_t WA_Mpeg_Read(WA_Input* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puReadedBytes);
void WA_Mpeg_ConfigDialog(WA_Input* This, HWND hParent);
void WA_Mpeg_PluginDialog(WA_Input* This, HWND hParent);
uint32_t WA_Mpeg_GetMetadata(WA_Input* This, WA_AudioMetadata* pMetadata);
uint32_t WA_Mpeg_GetFormat(WA_Input* This, WA_AudioFormat* pFormat);
uint32_t WA_Mpeg_GetFileInfo(WA_Input* This, const wchar_t* lpwFilePath, WA_AudioFormat* pFormat, WA_AudioMetadata* pMetadata, uint64_t *puDuration);

// Custom I/O Functions
mpg123_ssize_t WA_Mpeg_Custom_Read(void* pHandle, void* buf, size_t count);
off_t WA_Mpeg_Custom_Seek(void* pHandle, off_t offset, int whence);
void WA_Mpeg_Custom_Cleanup(void* pHandle);


// Module Header and VTable
static WA_Input MpegVTable = {
	
	{ // Start Common Header ---------------> 
	WA_PLUGINTYPE_INPUT,	// Plugin Type
	1U,					// Version
	L"WinAudio Mpeg Input\0", // Description
	NULL,					// WinAudio HWND
	false					// Enabled or Disabled
	}, // End Common Header <-----------------
	L"MPEG Audio Files\0", // Filter Name
	L"*.mp3;*.mp2;*.mp1\0", // Filter Extensions
	WA_Mpeg_New, // WA_Input_New
	WA_Mpeg_Delete, // WA_Input_Delete
	WA_Mpeg_Open, // WA_Input_Open
	WA_Mpeg_Close, // WA_Input_Close
	WA_Mpeg_IsStreamSeekable, // WA_Input_IsStreamSeekable
	WA_Mpeg_Duration, // WA_Input_Duration
	WA_Mpeg_Position, // WA_Input_Position
	WA_Mpeg_Seek, // WA_Input_Seek
	WA_Mpeg_Read, // WA_Input_Read
	WA_Mpeg_ConfigDialog, // WA_Input_ConfigDialog
	WA_Mpeg_PluginDialog, // WA_Input_PluginDialog
	WA_Mpeg_GetMetadata, // WA_Input_GetMetadata
	WA_Mpeg_GetFormat, // WA_Input_GetFormat
	WA_Mpeg_GetFileInfo,
	NULL  // hPluginData = Create After with a Malloc
};


// Store Instance Data
typedef struct TagWA_MpegInstance
{
	mpg123_handle* hMpg123;
	bool bFileIsOpen;
	HANDLE hFile;
	uint64_t uDuration;
	uint64_t uPosition;
	WA_AudioFormat Format;
	wchar_t* pCurrentPath;
} WA_MpegInstance;


static inline uint64_t WA_Mpeg_Samples_To_Ms(WA_AudioFormat* Format, uint64_t InSamples)
{
	return (InSamples / Format->uSamplerate) * 1000;
}

static inline uint64_t WA_Mpeg_Ms_To_Samples(WA_AudioFormat* Format, uint64_t InMs)
{
	return (InMs * Format->uSamplerate) / 1000;
}

static uint32_t WA_Mpeg_ReadID3(mpg123_handle* hMpg123, WA_AudioMetadata* pMetadata)
{

	mpg123_id3v1* id3v1;
	mpg123_id3v2* id3v2;
	int uConvertedChars;
	wchar_t pTempStr[WA_MPEG_TEMP_TAG_LEN];
	bool bID3v1Found;
	int nResult;

	if (!pMetadata)
		return WA_ERROR_BADPTR;

	nResult = mpg123_id3(hMpg123, &id3v1, &id3v2);

	if (nResult != MPG123_OK)
		return WA_ERROR_FILENOTSUPPORTED;

	ZeroMemory(pMetadata, sizeof(WA_AudioMetadata));


	// Check if "TAG" string is present in ID3v1 tags
	bID3v1Found = false;

	if (id3v1)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->tag, 3, pTempStr, WA_MPEG_TEMP_TAG_LEN);

		if (uConvertedChars > 0)
			bID3v1Found = (wcscmp(pTempStr, L"TAG") == 0) ? true : false;
	}


	if (bID3v1Found)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->artist, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcsncpy_s(pMetadata->Artist, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->title, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcsncpy_s(pMetadata->Title, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->album, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcsncpy_s(pMetadata->Album, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);

		if (id3v1->genre < 80)
			wcsncpy_s(pMetadata->Genre, WA_METADATA_MAX_LEN, GenresArray[id3v1->genre], WA_METADATA_MAX_LEN - 1);

		return WA_OK;
	}

	if (id3v2)
	{

		// Find ID3v2
		if (id3v2->artist)
		{
			uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->artist->p, (int)id3v2->artist->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
			if (uConvertedChars > 0)
				wcsncpy_s(pMetadata->Artist, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);
		}

		if (id3v2->title)
		{
			uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->title->p, (int)id3v2->title->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
			if (uConvertedChars > 0)
				wcsncpy_s(pMetadata->Title, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);
		}


		if (id3v2->album)
		{
			uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->album->p, (int)id3v2->album->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
			if (uConvertedChars > 0)
				wcsncpy_s(pMetadata->Album, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);
		}


		if (id3v2->genre)
		{
			uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->genre->p, (int)id3v2->genre->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
			if (uConvertedChars > 0)
				wcsncpy_s(pMetadata->Genre, WA_METADATA_MAX_LEN, pTempStr, WA_METADATA_MAX_LEN - 1);
		}
	}



	return WA_OK;
}

static uint32_t WA_Mpeg_ReadFormat(mpg123_handle* hMpg123, WA_AudioFormat* pFormat, int* pnEncoding)
{
	int nError;
	long mpg123_lRate;
	int mpg123_nChannels;
	int mpg123_nEncoding;

	// Get Stream Params
	nError = mpg123_getformat(hMpg123, &mpg123_lRate, &mpg123_nChannels, &mpg123_nEncoding);

	if (nError != MPG123_OK)
		return WA_ERROR_BADFORMAT;

	if (pnEncoding)
		(*pnEncoding) = mpg123_nEncoding;

	pFormat->uSamplerate = mpg123_lRate;
	pFormat->uChannels = mpg123_nChannels;
	pFormat->dwChannelMask = 0;

	switch (mpg123_nEncoding)
	{
	case MPG123_ENC_SIGNED_32:
	case MPG123_ENC_UNSIGNED_32:
	case MPG123_ENC_32:
		pFormat->uBitsPerSample = 32;
		break;
	case MPG123_ENC_SIGNED_24:
	case MPG123_ENC_UNSIGNED_24:
	case MPG123_ENC_24:
		pFormat->uBitsPerSample = 24;
		break;
	case MPG123_ENC_SIGNED_16:
	case MPG123_ENC_UNSIGNED_16:
	case MPG123_ENC_16:
		pFormat->uBitsPerSample = 16;
		break;
	case MPG123_ENC_SIGNED_8:
	case MPG123_ENC_UNSIGNED_8:
	case MPG123_ENC_8:
		pFormat->uBitsPerSample = 8;
	default:
		return WA_ERROR_FILENOTSUPPORTED;
	}

	pFormat->uBlockAlign = pFormat->uBitsPerSample / 8 * pFormat->uChannels;
	pFormat->uAvgBytesPerSec = pFormat->uSamplerate * pFormat->uBlockAlign;
	pFormat->uSampleType = (mpg123_nEncoding & MPG123_ENC_SIGNED) ? WA_SIGNED_SAMPLE : WA_UNSIGNED_SAMPLE;

	return WA_OK;
}

WA_HMODULE* WA_Plugin_GetHeader(void)
{
	return (WA_HMODULE*)(&MpegVTable);
}

bool WA_Mpeg_New(WA_Input* This)
{
	WA_MpegInstance* pInstance;	

	This->hPluginData = malloc(sizeof(WA_MpegInstance));

	if (!This->hPluginData)
		return false;

	pInstance = (WA_MpegInstance*) This->hPluginData;

	pInstance->bFileIsOpen = false;
	pInstance->pCurrentPath = NULL;
	pInstance->hFile = NULL;
	pInstance->uPosition = 0U;
	pInstance->uDuration = 0U;


#if _DEBUG
	int iMpg123Error;
	pInstance->hMpg123 = mpg123_new(NULL, &iMpg123Error);

	if (!pInstance->hMpg123)
	{
		_RPT1(_CRT_WARN, "%s \n", mpg123_plain_strerror(iMpg123Error));
		free(pInstance);
		pInstance = NULL;

		return false;
		
	}

	char** pDec = mpg123_supported_decoders();
	uint32_t i = 0;

	while (pDec[i])
	{
		//Print Supported Decoders
		_RPT1(_CRT_WARN, "%s \n", pDec[i]);
		i++;
	}


#else
	pInstance->hMpg123 = mpg123_new(NULL, NULL);

	if (!pInstance->hMpg123)
	{
		free(pInstance);
		pInstance = NULL;

		return false;
	}
#endif

	/*
	Disable support for Frankenstein streams (different MPEG streams stiched together). 
	Do not accept serious change of MPEG header inside a single stream. With this flag, the audio output 
	format cannot change during decoding unless you open a new stream. This also stops decoding after an 
	announced end of stream (Info header contained a number of frames and this number has been reached). 
	This makes your MP3 files behave more like ordinary media files with defined structure, rather than stream 
	dumps with some sugar.
	*/
	mpg123_param2(pInstance->hMpg123, MPG123_FLAGS, MPG123_NO_FRANKENSTEIN, 0);

	return true;

}

void WA_Mpeg_Delete(WA_Input* This)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	if (pInstance)
	{
		if (pInstance->hMpg123)
			mpg123_delete(pInstance->hMpg123);

		free(This->hPluginData);		
		This->hPluginData = NULL;
	}

}

uint32_t WA_Mpeg_Open(WA_Input* This, const wchar_t* lpwFilePath)
{	
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	int mpg123_nEncoding, nError;
	long* mpg123_lRates = NULL;
	size_t uPathLen, uRateCount;

	// Check if file is already open
	if (pInstance->bFileIsOpen)
		WA_Mpeg_Close(This);

	// Open file
	pInstance->hFile = CreateFile(lpwFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// Check for errors
	if (pInstance->hFile == INVALID_HANDLE_VALUE)
		return WA_ERROR_FILENOTFOUND;


	// Use Our Custom I/O Functions
	nError = mpg123_replace_reader_handle(pInstance->hMpg123,
		WA_Mpeg_Custom_Read,
		WA_Mpeg_Custom_Seek,
		WA_Mpeg_Custom_Cleanup);

	if (nError != MPG123_OK)
	{
		CloseHandle(pInstance->hFile);
		return WA_ERROR_FILENOTSUPPORTED;
	}


	// Set Supported Only Integer PCM Samples	
	mpg123_format_none(pInstance->hMpg123);
	mpg123_rates(&mpg123_lRates, &uRateCount);

	for (uint32_t i = 0; i < uRateCount; i++)
		mpg123_format(pInstance->hMpg123, 
			mpg123_lRates[i], MPG123_MONO | MPG123_STEREO,
			MPG123_ENC_8 | MPG123_ENC_16 | MPG123_ENC_24 | MPG123_ENC_32 |
			MPG123_ENC_SIGNED_16 | MPG123_ENC_SIGNED_24 | MPG123_ENC_SIGNED_32 | MPG123_ENC_SIGNED_8 |
			MPG123_ENC_UNSIGNED_16 | MPG123_ENC_UNSIGNED_24 | MPG123_ENC_UNSIGNED_32 | MPG123_ENC_UNSIGNED_8);

	

	nError = mpg123_open_handle(pInstance->hMpg123, pInstance->hFile);

	if (nError != MPG123_OK)
	{
		CloseHandle(pInstance->hFile);
		return WA_ERROR_FILENOTSUPPORTED;
	}


	// Get Stream Wave Format
	if (WA_Mpeg_ReadFormat(pInstance->hMpg123, &pInstance->Format, &mpg123_nEncoding) != WA_OK)
	{
		CloseHandle(pInstance->hFile);
		mpg123_close(pInstance->hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}


	/* Ensure that this output format will not change
       (it might, when we allow it). */
	nError = mpg123_format_none(pInstance->hMpg123);
	nError = mpg123_format(pInstance->hMpg123, pInstance->Format.uSamplerate, pInstance->Format.uChannels, mpg123_nEncoding);

	if (nError != MPG123_OK)
	{
		CloseHandle(pInstance->hFile);
		mpg123_close(pInstance->hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}	

	
	uPathLen = wcslen(lpwFilePath) + 1; // Inlcude Null-Terminating char
	pInstance->pCurrentPath = NULL;

	if (uPathLen > 0)
	{
		pInstance->pCurrentPath = calloc(uPathLen, sizeof(wchar_t));

		if(pInstance->pCurrentPath)
			wcscpy_s(pInstance->pCurrentPath, uPathLen, lpwFilePath);
	}

	pInstance->uDuration = WA_Mpeg_Samples_To_Ms(&pInstance->Format, mpg123_length(pInstance->hMpg123));
	pInstance->uPosition = 0U;
	pInstance->bFileIsOpen = true;

	return WA_OK;
}

void WA_Mpeg_Close(WA_Input* This)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	if (pInstance->bFileIsOpen)
	{
		mpg123_close(pInstance->hMpg123);		

		CloseHandle(pInstance->hFile);

		if (pInstance->pCurrentPath)
			free(pInstance->pCurrentPath);

		pInstance->pCurrentPath = NULL;
		pInstance->hFile = NULL;
		pInstance->uPosition = 0U;
		pInstance->uDuration = 0U;
		pInstance->bFileIsOpen = false;
	}
}

bool WA_Mpeg_IsStreamSeekable(WA_Input* This)
{
	return true;
}

uint64_t WA_Mpeg_Duration(WA_Input* This)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	return pInstance->uDuration;
}

uint64_t WA_Mpeg_Position(WA_Input* This)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	return pInstance->uPosition;
}

uint32_t WA_Mpeg_Seek(WA_Input* This, uint64_t uNewPosition)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	uint64_t uNewPositionInSamples = 0;
	off_t uAdjustedPositionInSamples = 0;

	if (uNewPosition > pInstance->uDuration)
		return WA_ERROR_BADPARAM;

	uNewPositionInSamples = WA_Mpeg_Ms_To_Samples(&pInstance->Format, uNewPosition);
	uAdjustedPositionInSamples = mpg123_seek(pInstance->hMpg123, (off_t) uNewPositionInSamples, SEEK_SET);

	if (uAdjustedPositionInSamples < 0)
		return WA_ERROR_STREAMNOTSEEKABLE;

	pInstance->uPosition = WA_Mpeg_Samples_To_Ms(&pInstance->Format, (uint64_t)uAdjustedPositionInSamples);

	return WA_OK;
}

uint32_t WA_Mpeg_Read(WA_Input* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puReadedBytes)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	int nError;
	size_t nReadedBytes;

	nError = mpg123_read(pInstance->hMpg123, pBuffer, (size_t) uBufferLen, &nReadedBytes);
	(*puReadedBytes) = (uint32_t)nReadedBytes;

	if (nError == MPG123_DONE)
		return WA_ERROR_ENDOFFILE;

	if (nError != MPG123_OK)
		return WA_ERROR_FAIL;

	// Increase Position in Ms
	pInstance->uPosition += ((uint64_t)(*puReadedBytes) * 1000) / pInstance->Format.uAvgBytesPerSec;

	return WA_OK;
}

void  WA_Mpeg_ConfigDialog(WA_Input* This, HWND hParent)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	MessageBox(hParent, L"Config Dialog Message", L"Config Dialog", MB_OK);
}

void  WA_Mpeg_PluginDialog(WA_Input* This, HWND hParent)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	MessageBox(hParent, L"Plugin Dialog Message", L"Plugin Dialog", MB_OK);

}


uint32_t WA_Mpeg_GetMetadata(WA_Input* This, WA_AudioMetadata* pMetadata)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	
	return WA_Mpeg_ReadID3(pInstance->hMpg123, pMetadata);
}

uint32_t WA_Mpeg_GetFormat(WA_Input* This, WA_AudioFormat* pFormat)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;

	if (!pFormat)
		return WA_ERROR_BADPTR;

	// Copy Internal Audio Format
	memcpy(pFormat, &pInstance->Format, sizeof(WA_AudioFormat));

	return WA_OK;
}

uint32_t WA_Mpeg_GetFileInfo(WA_Input* This, const wchar_t* lpwFilePath, WA_AudioFormat* pFormat, WA_AudioMetadata* pMetadata, uint64_t *puDuration)
{
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	mpg123_handle *hMpg123; 
	HANDLE hFile;
	int nError;
	
	// If path == current opened file use instance data
	if (pInstance->pCurrentPath)
	{
		if (_wcsicmp(pInstance->pCurrentPath, lpwFilePath) == 0)
		{
			if (WA_Mpeg_GetMetadata(This, pMetadata) != WA_OK)
				return WA_ERROR_BADPARAM;

			if (WA_Mpeg_GetFormat(This, pFormat) != WA_OK)
				return WA_ERROR_BADPARAM;

			if (puDuration)
				(*puDuration) = pInstance->uDuration;

			return WA_OK;
		}
	}

	// Otherwise create new mpg123 instance & get file infos
	hMpg123 = mpg123_new(NULL, NULL);

	if (!hMpg123)
		return WA_ERROR_FAIL;

	// Open file
	hFile = CreateFile(lpwFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// Check for errors
	if (hFile == INVALID_HANDLE_VALUE)
	{
		mpg123_delete(hMpg123);
		return WA_ERROR_FILENOTFOUND;
	}
		


	// Use Our Custom I/O Functions
	nError = mpg123_replace_reader_handle(hMpg123,
		WA_Mpeg_Custom_Read,
		WA_Mpeg_Custom_Seek,
		WA_Mpeg_Custom_Cleanup);

	if (nError != MPG123_OK)
	{
		CloseHandle(hFile);
		mpg123_delete(hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	nError = mpg123_open_handle(hMpg123, hFile);

	if (nError != MPG123_OK)
	{
		CloseHandle(hFile);
		mpg123_delete(hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}


	// Get Stream Wave Format
	if (WA_Mpeg_ReadFormat(hMpg123, pFormat, NULL) != WA_OK)
	{
		mpg123_close(hMpg123);
		CloseHandle(hFile);
		mpg123_delete(hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	// Get Stream Metadata
	if (WA_Mpeg_ReadID3(hMpg123, pMetadata) != WA_OK)
	{
		mpg123_close(hMpg123);
		CloseHandle(hFile);
		mpg123_delete(hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	if (puDuration)	
		(*puDuration) =	WA_Mpeg_Samples_To_Ms(pFormat, mpg123_length(hMpg123));	

	
	mpg123_close(hMpg123);
	CloseHandle(hFile);
	mpg123_delete(hMpg123);	

	return WA_OK;
}



mpg123_ssize_t WA_Mpeg_Custom_Read (void* pHandle, void* buf, size_t count)
{
	DWORD dwBytesReaded;
	BOOL bResult;

	if (!pHandle)
		return 0;

	bResult = ReadFile(pHandle, buf, (DWORD) count, &dwBytesReaded, NULL);

	// Check End of File
	if (bResult && (dwBytesReaded == 0))
		return 0;

	return (mpg123_ssize_t) dwBytesReaded;
}


off_t WA_Mpeg_Custom_Seek (void* pHandle, off_t offset, int whence)
{
	LARGE_INTEGER liOffset;
	LARGE_INTEGER liNewOffset;
	BOOL bResult = FALSE;

	if (!pHandle)
		return 0;

	liOffset.QuadPart = (LONGLONG) offset;	
	liNewOffset.QuadPart = 0;

	switch (whence)
	{
	case SEEK_SET:
		bResult = SetFilePointerEx(pHandle, liOffset, &liNewOffset, FILE_BEGIN);
		break;
	case SEEK_CUR:
		bResult = SetFilePointerEx(pHandle, liOffset, &liNewOffset, FILE_CURRENT);
		break;
	case SEEK_END:
		bResult = SetFilePointerEx(pHandle, liOffset, &liNewOffset, FILE_END);
	}

	if (!bResult)
		return -1;

	// Return new position
	return (off_t)liNewOffset.QuadPart;
}

void WA_Mpeg_Custom_Cleanup(void* pHandle)
{
	// Nothing to do here...maybe in future??
}