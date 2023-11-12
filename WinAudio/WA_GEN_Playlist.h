#ifndef WA_GEN_PLAYLIST_H
#define WA_GEN_PLAYLIST_H


#define WA_PLAYLIST_INITIAL_MAX_SIZE 50
#define WA_PLAYLIST_REALLOC_BLOCK 100

// Sort By
enum WA_PLAYLIST_SORT_BY
{
	WA_PLAYLIST_SORT_BY_ARTIST_TITLE,
	WA_PLAYLIST_SORT_BY_ALBUM,
	WA_PLAYLIST_SORT_BY_GENRE,
	WA_PLAYLIST_SORT_BY_DURATION,
	WA_PLAYLIST_SORT_BY_SIZE,
	WA_PLAYLIST_SORT_BY_PATH
};

enum WA_PLAYLIST_SORT_ORDER
{
	WA_PLAYLIST_SORT_UP,
	WA_PLAYLIST_SORT_DOWN
};

// Opaque Type
struct TagWA_Playlist;
typedef struct TagWA_Playlist WA_Playlist;

// Callback Info
typedef struct TagWA_Playlist_Metadata
{
	bool bFileSelected;
	bool bFileReaded;
	wchar_t lpwFilePath[MAX_PATH];
	LONGLONG dwFileSizeBytes;
	uint64_t uFileDurationMs;
	WA_AudioMetadata Metadata;
} WA_Playlist_Metadata;


// Called by this function to get metadata when needed
// It fills file path and return 0 = OK 1 = fail
typedef bool (WA_Playlist_ReadCallback)(WA_Playlist_Metadata* pMetadata);
typedef void (WA_Playlist_UpdateCallback)(bool bRedrawItems);

WA_Playlist* WA_Playlist_New(uint32_t uCacheSize, WA_Playlist_ReadCallback *pRead, WA_Playlist_UpdateCallback *pUpdate);
void WA_Playlist_Delete(WA_Playlist* This);

bool WA_Playlist_Add(WA_Playlist* This, const wchar_t* pFilePath);
bool WA_Playlist_Remove(WA_Playlist* This, DWORD dwIndex);
bool WA_Playlist_RemoveAll(WA_Playlist* This);

bool WA_Playlist_ReorderIndexes(WA_Playlist* This, DWORD* pIndexesArray, DWORD dwArrayCount, DWORD dwTargetIndex);
WA_Playlist_Metadata* WA_Playlist_Get_Item(WA_Playlist* This, DWORD dwIndex);
DWORD WA_Playlist_Get_Count(WA_Playlist* This);

void WA_Playlist_UpdateView(WA_Playlist* This, bool bRedrawItems);
void WA_Playlist_UpdateCache(WA_Playlist* This, DWORD dwFrom, DWORD dwTo);

void WA_Playlist_SelectIndex(WA_Playlist* This, DWORD dwIndex);
void WA_Playlist_DeselectIndex(WA_Playlist* This, DWORD dwIndex);

bool WA_Playlist_FindByFirstChar(WA_Playlist* This, DWORD dwStartIndex, const wchar_t* lpwSearchStr, DWORD* dwFoundIndex);

bool WA_Playlist_Get_SelectedIndex(WA_Playlist* This, DWORD *dwIndex);


bool WA_Playlist_LoadM3U(WA_Playlist* This, const wchar_t* pFilePath);
bool WA_Playlist_SaveAsM3U(WA_Playlist* This, const wchar_t* pFilePath);

void WA_Playlist_CacheNextItem(WA_Playlist* This);

void WA_Playlist_Sort(WA_Playlist* This, DWORD dwSortBy, int32_t nSortOrder);


inline void WA_Playlist_Merge_Artist_Title(wchar_t* pBuffer, DWORD dwBufferSize, const wchar_t* pArtist, const wchar_t* pTitle)
{
	DWORD dwMaxCount = dwBufferSize - 1U;

	if (wcslen(pTitle) != 0U)
		if (wcslen(pArtist) != 0U)
			_snwprintf_s(pBuffer, dwBufferSize, dwMaxCount, L"%s - %s\0", pArtist, pTitle);

		else
			_snwprintf_s(pBuffer, dwBufferSize, dwMaxCount, L"%s\0", pTitle);
	else
		_snwprintf_s(pBuffer, dwBufferSize, dwMaxCount, L"%s\0", pArtist);

}

#endif