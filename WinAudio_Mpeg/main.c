
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
	WA_AudioMetadata Metadata;
} WA_MpegInstance;


static inline uint64_t WA_Mpeg_Samples_To_Ms(WA_AudioFormat* Format, uint64_t InSamples)
{
	return (InSamples / Format->uSamplerate) * 1000;
}

static inline uint64_t WA_Mpeg_Ms_To_Samples(WA_AudioFormat* Format, uint64_t InMs)
{
	return (InMs * Format->uSamplerate) / 1000;
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
	int nError;
	WA_MpegInstance* pInstance = (WA_MpegInstance*)This->hPluginData;
	long mpg123_lRate;
	int mpg123_nChannels;
	int mpg123_nEncoding;
	long* mpg123_lRates = NULL;
	uint32_t uRateCount = 0;

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


	// Get Stream Params
	nError = mpg123_getformat(pInstance->hMpg123, &mpg123_lRate, &mpg123_nChannels, &mpg123_nEncoding);


	if (nError != MPG123_OK)
	{
		CloseHandle(pInstance->hFile);
		mpg123_close(pInstance->hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}


	/* Ensure that this output format will not change
       (it might, when we allow it). */
	nError = mpg123_format_none(pInstance->hMpg123);
	nError = mpg123_format(pInstance->hMpg123, mpg123_lRate, mpg123_nChannels, mpg123_nEncoding);

	if (nError != MPG123_OK)
	{
		CloseHandle(pInstance->hFile);
		mpg123_close(pInstance->hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;
	}

	pInstance->Format.uSamplerate = mpg123_lRate;
	pInstance->Format.uChannels = mpg123_nChannels;
	pInstance->Format.dwChannelMask = 0;
	
	switch (mpg123_nEncoding)
	{
	case MPG123_ENC_SIGNED_32:
	case MPG123_ENC_UNSIGNED_32:
	case MPG123_ENC_32:
		pInstance->Format.uBitsPerSample = 32;
		break;
	case MPG123_ENC_SIGNED_24:
	case MPG123_ENC_UNSIGNED_24:
	case MPG123_ENC_24:
		pInstance->Format.uBitsPerSample = 24;
		break;
	case MPG123_ENC_SIGNED_16:
	case MPG123_ENC_UNSIGNED_16:
	case MPG123_ENC_16:
		pInstance->Format.uBitsPerSample = 16;
		break;
	case MPG123_ENC_SIGNED_8:
	case MPG123_ENC_UNSIGNED_8:
	case MPG123_ENC_8:
		pInstance->Format.uBitsPerSample = 8;		
	default:
		// Return If Format is not supported
		CloseHandle(pInstance->hFile);
		mpg123_close(pInstance->hMpg123);
		return WA_ERROR_FILENOTSUPPORTED;

	}

	pInstance->Format.uBlockAlign = pInstance->Format.uBitsPerSample / 8 * pInstance->Format.uChannels;
	pInstance->Format.uAvgBytesPerSec = pInstance->Format.uSamplerate * pInstance->Format.uBlockAlign;
	pInstance->Format.uSampleType = (mpg123_nEncoding & MPG123_ENC_SIGNED) ? WA_SIGNED_SAMPLE : WA_UNSIGNED_SAMPLE;

	// Find Duration
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


	nError = mpg123_read(pInstance->hMpg123, pBuffer, uBufferLen, puReadedBytes);

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
	mpg123_id3v1 *id3v1;
	mpg123_id3v2 *id3v2;
	int uConvertedChars;
	wchar_t pTempStr[WA_MPEG_TEMP_TAG_LEN];
	bool bID3v1Found;
	int nResult;

	if (!pMetadata)
		return WA_ERROR_BADPTR;

	nResult = mpg123_id3(pInstance->hMpg123, &id3v1, &id3v2);

	if (nResult != MPG123_OK)
		return WA_ERROR_FILENOTSUPPORTED;

	ZeroMemory(pMetadata, sizeof(WA_AudioMetadata));
	

	// Check if "TAG" string is present in ID3v1 tags
	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->tag, 3, pTempStr, WA_MPEG_TEMP_TAG_LEN);
	bID3v1Found = false;
	if (uConvertedChars > 0)	
		bID3v1Found = (wcscmp(pTempStr, L"TAG") == 0) ? true : false;
	
	if (bID3v1Found)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->artist, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Artist, WA_METADATA_MAX_LEN, pTempStr);

		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->title, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Title, WA_METADATA_MAX_LEN, pTempStr);

		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v1->album, 30, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Album, WA_METADATA_MAX_LEN, pTempStr);

		if (id3v1->genre < 80)
			wcscpy_s(pMetadata->Genre, WA_METADATA_MAX_LEN, GenresArray[id3v1->genre]);

		return WA_OK;
	}
	
	// Find ID3v2
	if (id3v2->artist)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->artist->p, id3v2->artist->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Artist, WA_METADATA_MAX_LEN, pTempStr);
	}

	if (id3v2->title)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->title->p, id3v2->title->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Title, WA_METADATA_MAX_LEN, pTempStr);
	}


	if (id3v2->album)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->album->p, id3v2->album->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Album, WA_METADATA_MAX_LEN, pTempStr);
	}


	if (id3v2->genre)
	{
		uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, id3v2->genre->p, id3v2->genre->fill, pTempStr, WA_MPEG_TEMP_TAG_LEN);
		if (uConvertedChars > 0)
			wcscpy_s(pMetadata->Genre, WA_METADATA_MAX_LEN, pTempStr);
	}

	

	return WA_OK;
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