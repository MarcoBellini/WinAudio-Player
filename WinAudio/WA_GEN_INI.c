
#include "stdafx.h"
#include "WA_GEN_INI.h"

struct TagWA_Ini
{
	wchar_t lpwIniPath[MAX_PATH];
};


WA_Ini* WA_Ini_New(void)
{
	HRESULT hr;
	WA_Ini* pInstance;
	PWSTR lpwUserPath;

	pInstance = (WA_Ini*) malloc(sizeof(WA_Ini));

	if (!pInstance)
		return NULL;

	// Get User Roadming Folder
	hr = SHGetKnownFolderPath(&FOLDERID_RoamingAppData,
		KF_FLAG_DEFAULT,
		NULL,
		&lpwUserPath);

	if FAILED(hr)
	{
		free(pInstance);
		return NULL;
	}

	// Create Path to our settings folder
	PathAppend(pInstance->lpwIniPath, lpwUserPath);
	PathAppend(pInstance->lpwIniPath, L"\\WinAudio");


	// Check if Directory Exist
	if (!PathFileExists(pInstance->lpwIniPath))		
		CreateDirectory(pInstance->lpwIniPath, NULL);


	PathAppend(pInstance->lpwIniPath, L"\\Settings.ini");
	
	// Check if "Settings.ini" Exists
	if (!PathFileExists(pInstance->lpwIniPath))
	{
		HANDLE hFile;

		hFile = CreateFile(pInstance->lpwIniPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);

	}

	// See doc: https://learn.microsoft.com/it-it/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath?redirectedfrom=MSDN
	if (lpwUserPath)
		CoTaskMemFree(lpwUserPath);
	

	return pInstance;
}

void WA_Ini_Delete(WA_Ini* This)
{
	if (!This)
		return;	
	
	free(This);
	This = NULL;
	
}

bool WA_Ini_Read_Bool(WA_Ini* This, bool Default, wchar_t* Section, wchar_t* Key)
{
	return (bool) GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}

int8_t WA_Ini_Read_Int8(WA_Ini* This, int8_t Default, wchar_t* Section, wchar_t* Key)
{
	return (int8_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}

int16_t WA_Ini_Read_Int16(WA_Ini* This, int16_t Default, wchar_t* Section, wchar_t* Key)
{
	return (int16_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}


int32_t WA_Ini_Read_Int32(WA_Ini* This, int32_t Default, wchar_t* Section, wchar_t* Key)
{
	return (int32_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}


uint8_t WA_Ini_Read_UInt8(WA_Ini* This, uint8_t Default, wchar_t* Section, wchar_t* Key)
{
	return (uint8_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}


uint16_t WA_Ini_Read_UInt16(WA_Ini* This, uint32_t Default, wchar_t* Section, wchar_t* Key)
{
	return (uint16_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}


uint32_t WA_Ini_Read_UInt32(WA_Ini* This, uint32_t Default, wchar_t* Section, wchar_t* Key)
{
	return (uint32_t)GetPrivateProfileInt(Section, Key, (INT)Default, This->lpwIniPath);
}


bool WA_Ini_Read_String(WA_Ini* This, wchar_t* lpwBuffer, DWORD dwBufferSize, wchar_t* lpwDefault, wchar_t* Section, wchar_t* Key)
{
	return (GetPrivateProfileString(Section, Key, lpwDefault, lpwBuffer, dwBufferSize, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Read_Struct(WA_Ini* This, void* pStruct, DWORD dwStructSize, wchar_t* Section, wchar_t* Key)
{
	return (GetPrivateProfileStruct(Section, Key, pStruct, (UINT)dwStructSize, This->lpwIniPath) > 0) ? true : false;
}



bool WA_Ini_Write_Bool(WA_Ini* This, bool bValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[2];

	swprintf_s(Buffer, 2, L"%d\0", bValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}
bool WA_Ini_Write_Int8(WA_Ini* This, int8_t nValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[4];

	swprintf_s(Buffer, 4, L"%d\0", nValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Write_Int16(WA_Ini* This, int16_t nValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[6];

	swprintf_s(Buffer, 6, L"%d\0", nValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}


bool WA_Ini_Write_Int32(WA_Ini* This, int32_t nValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[11];

	swprintf_s(Buffer, 11, L"%d\0", nValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}


bool WA_Ini_Write_UInt8(WA_Ini* This, uint8_t uValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[4];

	swprintf_s(Buffer, 4, L"%u\0", uValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Write_UInt16(WA_Ini* This, uint32_t uValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[6];

	swprintf_s(Buffer, 6, L"%u\0", uValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Write_UInt32(WA_Ini* This, uint32_t uValue, wchar_t* Section, wchar_t* Key)
{
	wchar_t Buffer[11];

	swprintf_s(Buffer, 11, L"%u\0", uValue);

	return (WritePrivateProfileString(Section, Key, Buffer, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Write_String(WA_Ini* This, wchar_t* lpwBuffer, DWORD dwBufferSize, wchar_t* Section, wchar_t* Key)
{
	UNREFERENCED_PARAMETER(Section);

	return (WritePrivateProfileString(Section, Key, lpwBuffer, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Write_Struct(WA_Ini* This, void* pStruct, DWORD dwStructSize, wchar_t* Section, wchar_t* Key)
{
	return (WritePrivateProfileStruct(Section, Key, pStruct, (UINT)dwStructSize, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Delete_Key(WA_Ini* This, wchar_t* Section, wchar_t* Key)
{
	return (WritePrivateProfileString(Section, Key, NULL, This->lpwIniPath) > 0) ? true : false;
}

bool WA_Ini_Delete_Section(WA_Ini* This, wchar_t* Section)
{
	return (WritePrivateProfileString(Section, NULL, NULL, This->lpwIniPath) > 0) ? true : false;
}