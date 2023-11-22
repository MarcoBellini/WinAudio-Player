
#include "stdafx.h"
#include "WA_GEN_Types.h"
#include "WA_GEN_Playlist.h"


struct TagWA_Playlist
{
	DWORD dwPlaylistSize;
	DWORD dwCount;
	WA_Playlist_Metadata* pMetadataArray;
	WA_Playlist_ReadCallback* pRead;
	WA_Playlist_UpdateCallback* pUpdate;
};


/// <summary>
/// Create new instance of Playlist
/// </summary>
/// <param name="uInitialSize">Initial Size of Playlist MAX: WA_PLAYLIST_MAX_CACHE</param>
/// <param name="pRead">Pointer to an user custom function to read metadata</param>
/// <param name="pUpdate">Poiter to an user function to update Display</param>
/// <returns>On Success a pointer to a valid instance otherwise NULL</returns>
WA_Playlist* WA_Playlist_New(uint32_t uInitialSize, WA_Playlist_ReadCallback* pRead, WA_Playlist_UpdateCallback* pUpdate)
{
	WA_Playlist* pPlaylist;

	pPlaylist = (WA_Playlist*) malloc(sizeof(WA_Playlist));

	if (!pPlaylist)
		return NULL;


	pPlaylist->dwCount = 0;
	pPlaylist->dwPlaylistSize = min(uInitialSize, WA_PLAYLIST_INITIAL_MAX_SIZE);
	pPlaylist->pRead = pRead;
	pPlaylist->pUpdate = pUpdate;

	pPlaylist->pMetadataArray = (WA_Playlist_Metadata*)calloc(sizeof(WA_Playlist_Metadata),  pPlaylist->dwPlaylistSize);

	if (!pPlaylist->pMetadataArray)
	{
		free(pPlaylist);
		return NULL;
	}	

	return pPlaylist;
}

/// <summary>
/// Close Playlist instance
/// </summary>
void WA_Playlist_Delete(WA_Playlist* This)
{
	WA_Playlist* pPlaylist = This;

	pPlaylist->dwCount = 0;
	pPlaylist->dwPlaylistSize = 0;
	pPlaylist->pRead = NULL;
	pPlaylist->pUpdate = NULL;

	if (pPlaylist->pMetadataArray)
		free(pPlaylist->pMetadataArray);

	pPlaylist->pMetadataArray = NULL;

	free(pPlaylist);
	pPlaylist = NULL;
	
}

/// <summary>
/// Add new Item to Playlist
/// </summary>
/// <param name="pFilePath">File Path</param>
/// <returns>True on Success</returns>
bool WA_Playlist_Add(WA_Playlist* This, const wchar_t* pFilePath)
{	
	errno_t nError;

	// Add a path to playlist. Detailed information will be calculated later when caching
	ZeroMemory(&This->pMetadataArray[This->dwCount], sizeof(WA_Playlist_Metadata));
	nError = wcscpy_s(This->pMetadataArray[This->dwCount].lpwFilePath, MAX_PATH, pFilePath);	
		
	if (nError != 0)
		return false;

	// Prevent Buffer Overflow if Realloc Fails, but overwrite last item if we ignore "False" return
	This->dwCount++;
	This->dwCount = min(This->dwCount, This->dwPlaylistSize);

	// Check if we need to increase allocated memory
	if (This->dwCount == This->dwPlaylistSize)
	{
		WA_Playlist_Metadata* pTemp = NULL;
		DWORD dwNewSize = This->dwPlaylistSize + WA_PLAYLIST_REALLOC_BLOCK;		

		// If it fails keep the currently allocated memory
		pTemp = realloc(This->pMetadataArray, sizeof(WA_Playlist_Metadata) * dwNewSize);

		if (!pTemp)
			return false;

		for (DWORD i = This->dwPlaylistSize; i < dwNewSize; i++)
		{
			pTemp[i].bFileReaded = false;
			pTemp[i].bFileSelected = false;
		}
		
		// if pTemp != NULL assign new memory location
		This->pMetadataArray = pTemp;
		This->dwPlaylistSize = dwNewSize;
	}
	
	return true;
}


/// <summary>
/// Remove an item from playlist
/// </summary>
/// <param name="dwIndex">Index of item to remove</param>
/// <returns></returns>
bool WA_Playlist_Remove(WA_Playlist* This, DWORD dwIndex)
{
	if (This->dwCount < dwIndex)
		return false;

	// Decrease Count
	This->dwCount--;

	// Move all items to index -1 only if we delete a item in the middle of the list
	if (dwIndex < This->dwCount)
	{
		for (uint32_t i = dwIndex; i < This->dwCount; i++)
		{
			This->pMetadataArray[i] = This->pMetadataArray[i + 1];
		}
	}

	// Reduce memory and keep at least WA_PLAYLIST_INITIAL_MAX_SIZE allocated
	if ((This->dwCount == (This->dwPlaylistSize - WA_PLAYLIST_REALLOC_BLOCK)) && (This->dwCount > WA_PLAYLIST_INITIAL_MAX_SIZE))
	{
		WA_Playlist_Metadata* pTemp = NULL;
		DWORD dwNewSize = This->dwPlaylistSize - WA_PLAYLIST_REALLOC_BLOCK;

		// If it fails keep the currently allocated memory
		pTemp = realloc(This->pMetadataArray, sizeof(WA_Playlist_Metadata) * dwNewSize);

		if (!pTemp)
			return false;

		// if pTemp != NULL assign new memory location
		This->pMetadataArray = pTemp;
		This->dwPlaylistSize = dwNewSize;
	}


	return true;
}

/// <summary>
/// Remove all Items
/// </summary>
/// <returns></returns>
bool WA_Playlist_RemoveAll(WA_Playlist* This)
{
	WA_Playlist_Metadata* pTemp = NULL;

	if (This->dwCount == 0)
		return false;

	if (This->dwCount > WA_PLAYLIST_INITIAL_MAX_SIZE)
	{

		// If it fails keep the currently allocated memory
		pTemp = realloc(This->pMetadataArray, sizeof(WA_Playlist_Metadata) * WA_PLAYLIST_INITIAL_MAX_SIZE);

		if (!pTemp)
			return false;

		// Update Size
		This->pMetadataArray = pTemp;
		This->dwPlaylistSize = WA_PLAYLIST_INITIAL_MAX_SIZE;
	}	

	This->dwCount = 0;

	return true;

}

/// <summary>
/// Reorder playlist items based on an array of indexes
/// </summary>
/// <param name="pIndexesArray">Array of Indexes</param>
/// <param name="dwArrayCount">Count of Indexes</param>
/// <param name="dwTargetIndex">Index Index where to move the elements</param>
/// <returns>True on Success</returns>
bool WA_Playlist_ReorderIndexes(WA_Playlist* This, DWORD *pIndexesArray, DWORD dwArrayCount, DWORD dwTargetIndex)
{
	WA_Playlist_Metadata* pTempArray;
	DWORD nMinIndex, nMaxIndex, nRange, nIndex, nMinIndex2;
	DWORD i, j;

	if ((dwArrayCount == 0U) || (!pIndexesArray))
		return false;

	if (This->dwCount < dwTargetIndex)
		return false;


	nMinIndex = dwTargetIndex;
	nMaxIndex = dwTargetIndex;
	nMinIndex2 = pIndexesArray[0];

	// Find the range between the minor index and the major index. 
	// Also consider the tatget index
	for (i = 0; i < dwArrayCount; i++)
	{
		if (pIndexesArray[i] < nMinIndex)
			nMinIndex = pIndexesArray[i];

		if (pIndexesArray[i] > nMaxIndex)
			nMaxIndex = pIndexesArray[i];

		if (pIndexesArray[i] < nMinIndex2)
			nMinIndex2 = pIndexesArray[i];
	}

	// Allocate space for the temporary array to store the sorted elements (between min and max index)
	nRange = nMaxIndex - nMinIndex + 1U;

	pTempArray = calloc(nRange, sizeof(WA_Playlist_Metadata));

	if (!pTempArray)
		return false;

	// If MinIndex is lower than Target Put selected items on top of TempArray
	nIndex = 0U;

	if (nMinIndex2 > dwTargetIndex)
	{
		for (i = 0; i < dwArrayCount; i++)
		{
			pTempArray[nIndex] = This->pMetadataArray[pIndexesArray[i]];
			nIndex++;
		}
	}

	// Copy unselected items
	for (i = nMinIndex; i <= nMaxIndex; i++)
	{
		bool bFound = false;


		for (j = 0; j < dwArrayCount; j++)
		{
			bFound = (pIndexesArray[j] == i);

			if (bFound)
				break;
		}

		if (!bFound)
		{
			pTempArray[nIndex] = This->pMetadataArray[i];
			nIndex++;
		}
	}

	// If MinIndex is greater than Target Put selected items on bottom of TempArray
	if (nMinIndex2 < dwTargetIndex)
	{
		for (i = 0; i < dwArrayCount; i++)
		{
			pTempArray[nIndex] = This->pMetadataArray[pIndexesArray[i]];
			nIndex++;
		}
	}

	// Copy the sorted elements to the destination array
	nIndex = 0U;
	for (i = nMinIndex; i <= nMaxIndex; i++)
	{
		This->pMetadataArray[i] = pTempArray[nIndex];
		nIndex++;
	}


	free(pTempArray);

	return true;
}



/// <summary>
/// Get a pointer to an Item
/// </summary>
/// <param name="dwIndex">Index of a Item to Get Data</param>
/// <returns>Pointer to an itemor NULL on fail</returns>
WA_Playlist_Metadata* WA_Playlist_Get_Item(WA_Playlist* This, DWORD dwIndex)
{	

	// Skip indices out of range
	if (This->dwCount < dwIndex)
		return NULL;

	return &This->pMetadataArray[dwIndex];	
}

/// <summary>
/// Get the number of items in Playlist
/// </summary>
/// <returns>Number of Items</returns>
DWORD WA_Playlist_Get_Count(WA_Playlist* This)
{
	return This->dwCount;
}

/// <summary>
/// Call the Update Callback function
/// </summary>
void WA_Playlist_UpdateView(WA_Playlist* This, bool bRedrawItems)
{
	if (This->pUpdate)
		This->pUpdate(bRedrawItems);

}

/// <summary>
/// Update Chache
/// </summary>
/// <param name="dwFrom">Initial Index</param>
/// <param name="dwTo">End Index</param>
void WA_Playlist_UpdateCache(WA_Playlist* This, DWORD dwFrom, DWORD dwTo)
{

	// Check if we have valid indexes
	if (This->dwCount < dwFrom)
		return;

	if (This->dwCount < dwTo)
		return;

	// Only add items to the cache if they don't already exist
	for (DWORD i = dwFrom; i <= dwTo; i++)
	{
		if (This->pMetadataArray[i].bFileReaded)
			continue;
		
		if (This->pRead(&This->pMetadataArray[i]))
			This->pMetadataArray[i].bFileReaded = true;

	}

}

/// <summary>
/// Select an Index
/// </summary>
/// <param name="dwIndex">Index to select</param>
void WA_Playlist_SelectIndex(WA_Playlist* This, DWORD dwIndex)
{

	// Skip indices out of range
	if (This->dwCount < dwIndex)
		return;

	This->pMetadataArray[dwIndex].bFileSelected = true;
}

/// <summary>
/// Deselect an Index
/// </summary>
/// <param name="dwIndex">Index to Deselect</param>
void WA_Playlist_DeselectIndex(WA_Playlist* This, DWORD dwIndex)
{
	// Skip indices out of range
	if (This->dwCount < dwIndex)
		return;

	This->pMetadataArray[dwIndex].bFileSelected = false;

}

/// <summary>
/// Returns the index of the item that contains the letter indicated by lpwSearchStr at the beginning of the filename
/// </summary>
/// <param name="dwStartIndex">Starting index from which to start the search</param>
/// <param name="lpwSearchStr">String containing the characters to search for</param>
/// <param name="dwFoundIndex">Pointer to a DWORD variable on which to insert the found index.
/// If several files are present, the function returns the various indexes in sequence</param>
/// <returns></returns>
bool WA_Playlist_FindByFirstChar(WA_Playlist* This, DWORD dwStartIndex, const wchar_t* lpwSearchStr, DWORD *dwFoundIndex)
{
	DWORD dwIndex, dwSearchStrLen;
	wchar_t* lpwFileName;

	if (This->dwCount < dwStartIndex)
		return false;

	dwSearchStrLen = (DWORD) wcslen(lpwSearchStr);
	dwIndex = dwStartIndex;

	do
	{
		
		lpwFileName = PathFindFileName(This->pMetadataArray[dwIndex].lpwFilePath);

		if (_wcsnicmp(lpwFileName, lpwSearchStr, (size_t) dwSearchStrLen) == 0)
		{
			(*dwFoundIndex) = dwIndex;
			return true;
		}

		dwIndex++;
		dwIndex = dwIndex % This->dwCount;

	} while (dwIndex != dwStartIndex);

	return false;

}

/// <summary>
/// Return the Index of current selected index
/// </summary>
/// <param name="dwIndex">The selected index (If Found)</param>
/// <returns>True on Success</returns>
bool WA_Playlist_Get_SelectedIndex(WA_Playlist* This, DWORD *dwIndex)
{

	for (DWORD i = 0U; i < This->dwCount; i++)
	{
		if (This->pMetadataArray[i].bFileSelected)
		{
			(*dwIndex) = i;
			return true;
		}
			
	}

	return false;
}

/// <summary>
/// Load M3U8 Playlist
/// </summary>
bool WA_Playlist_LoadM3U(WA_Playlist* This, const wchar_t* pFilePath)
{
	uint32_t uBytesReaded = 0;
	uint32_t uUnicodeRequiedSize, uConvertedChars;
	LARGE_INTEGER FileSize;

	wchar_t* Context = NULL;
	wchar_t* Token = NULL;
	wchar_t* UnicodeText = NULL;
	char* UFT8Text = NULL;

	HANDLE hFile;		
	BOOL bResult;
	

	hFile = CreateFile(pFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)		
		return false;


	bResult = GetFileSizeEx(hFile, &FileSize);

	// Skip file > 4MB
	if ((!bResult) || (FileSize.QuadPart > 0x3D0900))
	{
		CloseHandle(hFile);
		return false;
	}		

	UFT8Text = (char* )malloc(FileSize.LowPart + 1); // Reserve space for Null-Terminating character

	if (!UFT8Text)
	{
		CloseHandle(hFile);
		return false;
	}


	bResult = ReadFile(hFile, UFT8Text, FileSize.LowPart, &uBytesReaded, NULL);

	// Test is we read successful
	if ((uBytesReaded == 0) && (bResult))
	{
		free(UFT8Text);
		CloseHandle(hFile);
		return false;	
	}

	// Insert Null-Terminating character
	UFT8Text[FileSize.LowPart] = 0;

	// Calc the requied size of unicode buffer, in characters (Include Null Terminating Character)
	uUnicodeRequiedSize = MultiByteToWideChar(CP_UTF8, 0, UFT8Text, -1, NULL, 0);
	uUnicodeRequiedSize = uUnicodeRequiedSize * sizeof(wchar_t);


	UnicodeText = (wchar_t*)malloc(uUnicodeRequiedSize);


	if (!UnicodeText)
	{
		free(UFT8Text);
		CloseHandle(hFile);
		return false;
	}
	

	// Convert Chuck from UFT8 to wchar_t (Include Null Terminating Character)
	uConvertedChars = MultiByteToWideChar(CP_UTF8, 0, UFT8Text, -1, UnicodeText, uUnicodeRequiedSize);


	if (uConvertedChars == 0)
	{
		free(UFT8Text);
		free(UnicodeText);

		CloseHandle(hFile);
		return false;
	}


	free(UFT8Text);
	UFT8Text = NULL;

	
	
	// Split string into multiple paths
	Token = wcstok_s(UnicodeText, L"\r\n", &Context);

	while (Token != NULL)
	{
		if (wcschr(Token, L'#') == NULL)
		{
			// TODO: PathFileExists slow down the process when file is on SMB share
			// check if exist anothe function or check file existence later, when file
			// is opened by the user
			// if (PathFileExists(Token)) It slow down the process on file on SMB
			//if(PathIsNetworkPath(Token))
			//	WA_Playlist_Add(This, Token);
			//else
			//	if (PathFileExists(Token))
					WA_Playlist_Add(This, Token);
					
			
		}
	
		Token = wcstok_s(NULL, L"\r\n", &Context);
	}

	free(UnicodeText);
	CloseHandle(hFile);

	return true;
}

/// <summary>
/// Save Playlist as M3U8
/// </summary>
bool WA_Playlist_SaveAsM3U(WA_Playlist* This, const wchar_t* pFilePath)
{
	HANDLE hFile;
	char* UTF8Text;
	uint32_t uUTF8RequiedSize, uConvertedBytes, uWrittenBytes;

	hFile = CreateFile(pFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	for (uint32_t i = 0U; i < This->dwCount; i++)
	{
		wchar_t FilePath[MAX_PATH + 2];
		uint32_t PathLen;

		wcscpy_s(FilePath, (MAX_PATH + 2), This->pMetadataArray[i].lpwFilePath);
		wcscat_s(FilePath, (MAX_PATH + 2), L"\r\n");
		PathLen = (uint32_t) wcslen(FilePath);
	
		uUTF8RequiedSize = WideCharToMultiByte(CP_UTF8, 0, FilePath, PathLen, NULL, 0, NULL, NULL);

		UTF8Text = (char*) malloc(uUTF8RequiedSize);

		if (!UTF8Text)
			continue;


		uConvertedBytes = WideCharToMultiByte(CP_UTF8, 0, FilePath, PathLen, UTF8Text, uUTF8RequiedSize, NULL, NULL);

		if (uConvertedBytes > 0)
		{

			WriteFile(hFile, (LPCVOID) UTF8Text, uConvertedBytes, &uWrittenBytes, NULL);
		}

		free(UTF8Text);

	}


	CloseHandle(hFile);

	return true;

}

/// <summary>
/// Find the next item without cached info and metadata
/// </summary>
void WA_Playlist_CacheNextItem(WA_Playlist* This)
{
	DWORD dwIndexToCache;
	bool bNeedToCache = false;	

	if (This->dwCount == 0)
		return;
	

	// Find First item to be cached
	for (DWORD i = 0; i < This->dwCount; i++)
	{
		if (!This->pMetadataArray[i].bFileReaded)
		{
			dwIndexToCache = i;
			bNeedToCache = true;
			break;
		}
	}

	if (!bNeedToCache)
		return;

	// Read Metadata
	if (This->pRead(&This->pMetadataArray[dwIndexToCache]))
		This->pMetadataArray[dwIndexToCache].bFileReaded = true;;

	// Update View
	WA_Playlist_UpdateView(This, true);

	
}


/// <summary>
/// Compare function used in qsort_s
/// Compare by Path
/// </summary>
static int WA_Playlist_Cmp_Path(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	int32_t nResult;

	nResult = _wcsicmp(p1->lpwFilePath, p2->lpwFilePath);

	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		return (nResult > 0) ? 1 : ((nResult == 0) ? 0 : -1);
	case WA_PLAYLIST_SORT_DOWN:
		return (nResult > 0) ? -1 : ((nResult == 0) ? 0 : 1);
	}

	return 0;
}


/// <summary>
/// Compare function used in qsort_s
/// Compare by Artist/Title as viewed in listview
/// </summary>
static int WA_Playlist_Cmp_Artist_Title(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	wchar_t Buffer1[50];
	wchar_t Buffer2[50];
	int32_t nResult;

	WA_Playlist_Merge_Artist_Title(Buffer1, ARRAYSIZE(Buffer1), p1->Metadata.Artist, p1->Metadata.Title);
	WA_Playlist_Merge_Artist_Title(Buffer2, ARRAYSIZE(Buffer2), p2->Metadata.Artist, p2->Metadata.Title);

	nResult = _wcsicmp(Buffer1, Buffer2);
			
	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		return (nResult > 0) ? 1 : ((nResult == 0) ? 0 : -1);		
	case WA_PLAYLIST_SORT_DOWN:
		return (nResult > 0) ? -1 : ((nResult == 0) ? 0 : 1);
	}

	return 0;
}

/// <summary>
/// Compare function used in qsort_s
/// Compare by Album
/// </summary>
static int WA_Playlist_Cmp_Album(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	const WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	const WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	int32_t nResult;

	nResult = _wcsicmp(p1->Metadata.Album, p2->Metadata.Album);

	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		return (nResult > 0) ? 1 : ((nResult == 0) ? 0 : -1);
	case WA_PLAYLIST_SORT_DOWN:
		return (nResult > 0) ? -1 : ((nResult == 0) ? 0 : 1);
	}

	return 0;
}

/// <summary>
/// Compare function used in qsort_s
/// Compare by Genre
/// </summary>
static int WA_Playlist_Cmp_Genre(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	int32_t nResult;

	nResult = _wcsicmp(p1->Metadata.Genre, p2->Metadata.Genre);

	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		return (nResult > 0) ? 1 : ((nResult == 0) ? 0 : -1);
	case WA_PLAYLIST_SORT_DOWN:
		return (nResult > 0) ? -1 : ((nResult == 0) ? 0 : 1);
	}

	return 0;
}

/// <summary>
/// Compare function used in qsort_s
/// Compare by Duration (in milliseconds)
/// </summary>
static int WA_Playlist_Cmp_Duration(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	int32_t nResult = 0;


	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		if (p1->uFileDurationMs == p2->uFileDurationMs)
			nResult = 0;
		else if (p1->uFileDurationMs > p2->uFileDurationMs)
			nResult = 1;
		else
			nResult = -1;

		break;
	case WA_PLAYLIST_SORT_DOWN:
		if (p1->uFileDurationMs == p2->uFileDurationMs)
			nResult = 0;
		else if (p2->uFileDurationMs > p1->uFileDurationMs)
			nResult = 1;
		else
			nResult = -1;

	}

	return nResult;
}


/// <summary>
/// Compare function used in qsort_s
/// Compare by file Size
/// </summary>
static int WA_Playlist_Cmp_Size(void* order, const void* obj1, const void* obj2)
{
	int32_t SortOrder = *((int32_t*)order);
	WA_Playlist_Metadata* p1 = (WA_Playlist_Metadata*)obj1;
	WA_Playlist_Metadata* p2 = (WA_Playlist_Metadata*)obj2;
	LONGLONG nResult;

	nResult = p1->dwFileSizeBytes - p2->dwFileSizeBytes;

	switch (SortOrder)
	{
	case WA_PLAYLIST_SORT_UP:
		return (nResult > 0) ? 1 : ((nResult == 0) ? 0 : -1);
	case WA_PLAYLIST_SORT_DOWN:
		return (nResult > 0) ? -1 : ((nResult == 0) ? 0 : 1);
	}

	return 0;
}


/// <summary>
/// Perform a Playlist sort
/// </summary>
/// <param name="dwSortBy">See WA_PLAYLIST_SORT_BY enum</param>
/// <param name="nSortOrder">See WA_PLAYLIST_SORT_ORDER enum</param>
void WA_Playlist_Sort(WA_Playlist* This, DWORD dwSortBy, int32_t nSortOrder)
{
	switch (dwSortBy)
	{
		case WA_PLAYLIST_SORT_BY_ARTIST_TITLE:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Artist_Title, &nSortOrder);
			break;
		case WA_PLAYLIST_SORT_BY_ALBUM:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Album, &nSortOrder);
			break;
		case WA_PLAYLIST_SORT_BY_GENRE:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Genre, &nSortOrder);
			break;
		case WA_PLAYLIST_SORT_BY_DURATION:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Duration, &nSortOrder);
			break;
		case WA_PLAYLIST_SORT_BY_SIZE:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Size, &nSortOrder);
			break;
		case WA_PLAYLIST_SORT_BY_PATH:
			qsort_s(This->pMetadataArray, This->dwCount, sizeof(WA_Playlist_Metadata), WA_Playlist_Cmp_Path, &nSortOrder);
	}

	WA_Playlist_UpdateView(This, true);
}
