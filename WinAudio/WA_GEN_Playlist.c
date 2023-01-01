
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
bool WA_Playlist_Add(WA_Playlist* This, wchar_t* pFilePath)
{	
	errno_t nError;

	// Add a path to playlist. Detailed information will be calculated later when caching
	nError = wcscpy_s(This->pMetadataArray[This->dwCount].lpwFilePath, MAX_PATH, pFilePath);
	This->pMetadataArray[This->dwCount].bFileReaded = false;

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

	This->dwCount = 0;

	// If it fails keep the currently allocated memory
	pTemp = realloc(This->pMetadataArray, sizeof(WA_Playlist_Metadata) * WA_PLAYLIST_INITIAL_MAX_SIZE);

	if (!pTemp)
		return false;

	// Update Size
	This->pMetadataArray = pTemp;
	This->dwPlaylistSize = WA_PLAYLIST_INITIAL_MAX_SIZE;

	return true;

}

/// <summary>
/// Move an index to a new Index value
/// </summary>
/// <param name="dwIndex">Current index value</param>
/// <param name="dwNewIndex">New Index value</param>
/// <returns>True on Success</returns>
bool WA_Playlist_MoveToIndex(WA_Playlist* This, DWORD dwIndex, DWORD dwNewIndex)
{
	WA_Playlist_Metadata pTemp;

	if ((This->dwCount < dwIndex) || (This->dwCount < dwNewIndex))
		return false;

	// Swap Items
	pTemp = This->pMetadataArray[dwIndex];
	This->pMetadataArray[dwIndex] = This->pMetadataArray[dwNewIndex];
	This->pMetadataArray[dwNewIndex] = pTemp;

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
	

	// Add an item to the cache that is not present
	if (!This->pMetadataArray[dwIndex].bFileReaded)
	{
		DWORD dwFrom, dwTo;

		dwFrom = dwIndex;
		dwTo = dwIndex + This->dwPlaylistSize; // Try to update current block
		dwTo = min(dwTo, This->dwCount);

		WA_Playlist_UpdateCache(This, dwFrom, dwTo);		
	}


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
		
		if (!This->pMetadataArray[i].bFileReaded)
		{
		
			if (This->pRead(&This->pMetadataArray[i]))
				This->pMetadataArray[i].bFileReaded = true;
		}

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

bool WA_Playlist_FindByFirstChar(WA_Playlist* This, DWORD dwStartIndex, wchar_t* lpwSearchStr, DWORD *dwFoundIndex)
{
	DWORD dwIndex, dwSearchStrLen;
	wchar_t* lpwFileName;

	if (This->dwCount < dwStartIndex)
		return false;

	dwSearchStrLen = wcslen(lpwSearchStr);
	dwIndex = dwStartIndex;

	do
	{
		
		lpwFileName = PathFindFileName(This->pMetadataArray[dwIndex].lpwFilePath);

		if (_wcsnicmp(lpwFileName, lpwSearchStr, dwSearchStrLen) == 0)
		{
			(*dwFoundIndex) = dwIndex;
			return true;
		}

		dwIndex++;
		dwIndex = dwIndex % This->dwCount;

	} while (dwIndex != dwStartIndex);

	return false;

}