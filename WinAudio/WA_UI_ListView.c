#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_UI_ListView.h"
#include "WA_GEN_Playlist.h"
#include "WA_GEN_INI.h"
#include "WA_GEN_Playback_Engine.h"
#include "WA_UI_Visualizations.h"
#include "Globals2.h"
#include "resource.h"

// String of Columns
static wchar_t WA_Listview_Column_Status[] = L"Status\0";
static wchar_t WA_Listview_Column_Index[] = L"Index\0";
static wchar_t WA_Listview_Column_TileArtist[] = L"Title / Artist\0";
static wchar_t WA_Listview_Column_Album[] = L"Album\0";
static wchar_t WA_Listview_Column_Duration[] = L"Duration\0";
static wchar_t WA_Listview_Column_Genre[] = L"Genre\0";
static wchar_t WA_Listview_Column_Size[] = L"File Size(MB)\0";
static wchar_t WA_Listview_Column_Path[] = L"File Path\0";

// Process Custom Draw (Recived in a form of WM_NOTIFY)
LRESULT WA_UI_Listview_CustomDraw(HWND hWnd, LPNMLVCUSTOMDRAW lplvcd);

WA_Listview_Column Columns[WA_LISTVIEW_COLUMNS_COUNT];

static inline bool WA_UI_Listview_IsRowSelected(HWND hWnd, int nRow)
{
    return ListView_GetItemState(hWnd, nRow, LVIS_SELECTED) != 0;
}

static void WA_UI_Listview_AddColumn(HWND hListview, wchar_t* pwColumnText, int32_t nColumnWidth, int32_t nColumnIndex, int32_t nFlags)
{
    LV_COLUMN Column;
    int32_t nReturn;

    ZeroMemory(&Column, sizeof(LV_COLUMN));

    Column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.mask |= ((nFlags > 0) ? LVCF_FMT : 0);

    Column.cx = nColumnWidth;
    Column.pszText = pwColumnText;
    Column.cchTextMax = (int) wcslen(Column.pszText);
    Column.iSubItem = nColumnIndex + 1;
    Column.fmt = nFlags;

    nReturn = ListView_InsertColumn(hListview, nColumnIndex, &Column);
    _ASSERT(nReturn == nColumnIndex);
}

static void WA_UI_Listview_RemoveColumn(HWND hListview, int32_t nColumnIndex)
{
    ListView_DeleteColumn(hListview, nColumnIndex);
}

static inline WA_Listview_Column* WA_UI_Listview_GetColumnFromID(DWORD dwColumnID)
{
    WA_Listview_Column* pColumn = NULL;
    bool bFound = false;
    DWORD dwIndex = 0U;

    while ((!bFound) && (dwIndex < WA_LISTVIEW_COLUMNS_COUNT))
    {
        if (Columns[dwIndex].dwColumn == dwColumnID)
        {
            pColumn = &Columns[dwIndex];
            bFound = true;
        }

        dwIndex++;
    }

    return pColumn;
}

static inline wchar_t* WA_Listview_GetStringFromID(DWORD dwColumnID)
{
    switch (dwColumnID)
    {
    case WA_LISTVIEW_COLUMN_STATUS:
        return WA_Listview_Column_Status;
    case WA_LISTVIEW_COLUMN_INDEX:
        return WA_Listview_Column_Index;
    case WA_LISTVIEW_COLUMN_TITLE_ARTIST:
        return WA_Listview_Column_TileArtist;
    case WA_LISTVIEW_COLUMN_ALBUM:
        return WA_Listview_Column_Album;
    case WA_LISTVIEW_COLUMN_DURATION:
        return WA_Listview_Column_Duration;
    case WA_LISTVIEW_COLUMN_GENRE:
        return WA_Listview_Column_Genre;
    case WA_LISTVIEW_COLUMN_SIZE:
        return WA_Listview_Column_Size;
    case WA_LISTVIEW_COLUMN_PATH:
        return WA_Listview_Column_Path;
    }

    return NULL;
}

static inline DWORD WA_UI_Listview_GetIDFromIndex(DWORD dwSubitemIndex)
{
    DWORD dwIndex = 0U;

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
            if (Columns[i].dwColumnIndex == dwSubitemIndex)
                return Columns[i].dwColumn;

            dwIndex++;
        }
    }

    return 0U;
}

static inline void WA_UI_Listview_RebuildIdexes()
{
    DWORD dwIndex = 0U;

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
            Columns[i].dwColumnIndex = dwIndex;
            dwIndex++;
        }
    }
}

static inline DWORD WA_UI_Listview_CountVisibleColumns()
{
    DWORD dwCount = 0U;

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)           
            dwCount++;
    }

    return dwCount;
}

static inline void WA_UI_Listview_RebuildOrder(HWND hListview)
{
    DWORD dwIndex = 0U;
    DWORD dwCount = WA_UI_Listview_CountVisibleColumns();
    int32_t nOrderArray[WA_LISTVIEW_COLUMNS_COUNT];

    ListView_GetColumnOrderArray(hListview, dwCount, nOrderArray);

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
            Columns[i].dwColumnOrder = nOrderArray[dwIndex];
            dwIndex++;
        }
    }
}

static inline void WA_UI_Listview_RebuildWitdh(HWND hListview)
{
    DWORD dwIndex = 0U;
    DWORD dwCount = WA_UI_Listview_CountVisibleColumns();
    DWORD dwWidth;

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
            dwWidth = ListView_GetColumnWidth(hListview, dwIndex);

            if(dwWidth > 0)
                Columns[i].dwColumnWidth = dwWidth;

            dwIndex++;
        }
    }
}

static inline DWORD WA_UI_Listview_FindNearLeftIndex(DWORD dwDesideredIndex)
{
    DWORD dwIndex = 0U;

    if (dwDesideredIndex == 0U)
        return 0U;

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
    
            if (dwDesideredIndex < Columns[i].dwColumnDefaultIndex)
                return dwIndex;

            dwIndex++;
        }
    }

    return dwIndex;
}

static void WA_UI_Listview_HideColumn(HWND hListview, DWORD dwColumnID)
{
    WA_Listview_Column* pColumn;
    bool bFound = false;
    DWORD dwIndex = 0U;

    pColumn = WA_UI_Listview_GetColumnFromID(dwColumnID);

    // Do Nothing if Column is Already hidden
    if (!pColumn->bIsVisible)
        return;

    //Store Witdh info before remove the column
    WA_UI_Listview_RebuildWitdh(hListview);

    WA_UI_Listview_RemoveColumn(hListview, pColumn->dwColumnIndex);
    pColumn->bIsVisible = false;

    WA_UI_Listview_RebuildIdexes();
    WA_UI_Listview_RebuildOrder(hListview);
    
}

static void WA_UI_Listview_ShowColumn(HWND hListview, DWORD dwColumnID)
{
    WA_Listview_Column* pColumn;
    DWORD dwMapIndex;

    pColumn = WA_UI_Listview_GetColumnFromID(dwColumnID);

    // Do Nothing if Column is Already hidden
    if (pColumn->bIsVisible)
        return;

    dwMapIndex = WA_UI_Listview_FindNearLeftIndex(pColumn->dwColumnDefaultIndex);

    WA_UI_Listview_AddColumn(hListview,
        WA_Listview_GetStringFromID(pColumn->dwColumn),
        pColumn->dwColumnWidth,
        dwMapIndex,
        pColumn->uFlags);

    pColumn->bIsVisible = true;

    WA_UI_Listview_RebuildIdexes();
    WA_UI_Listview_RebuildOrder(hListview);
    WA_UI_Listview_RebuildWitdh(hListview);
}




static void WA_UI_Listview_ShowHeaderContextMenu(HWND hHeader, int32_t x, int32_t y)
{
    HMENU hMenu = CreatePopupMenu();
    WA_Listview_Column* pColumn;
    DWORD dwMenuID;
    
    if (!hMenu)
        return;

    // Handle SHIFT+F10 command to show context menu
    if ((x < 0) || (y < 0))
    {
        RECT HeaderRect;

        GetWindowRect(hHeader, &HeaderRect);

        x = HeaderRect.left;
        y = HeaderRect.top;
    }

    for (uint32_t i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (Columns[i].bIsVisible)
        {
            AppendMenuW(hMenu,
                MF_CHECKED | MF_STRING, 
                Columns[i].dwColumn, 
                WA_Listview_GetStringFromID(Columns[i].dwColumn));
        }
        else
        {
            AppendMenuW(hMenu,
                MF_UNCHECKED | MF_STRING,
                Columns[i].dwColumn,
                WA_Listview_GetStringFromID(Columns[i].dwColumn));
        }

    }

    dwMenuID = TrackPopupMenu(hMenu, 
        TPM_NONOTIFY | TPM_RETURNCMD, 
        x, y, 0, 
        hHeader, NULL);


    if (dwMenuID > 0)
    {
        pColumn = WA_UI_Listview_GetColumnFromID(dwMenuID);

        if (pColumn->bIsVisible)
            WA_UI_Listview_HideColumn(Globals2.hListView, pColumn->dwColumn);
        else
            WA_UI_Listview_ShowColumn(Globals2.hListView, pColumn->dwColumn);
    }

    DestroyMenu(hMenu);
}

static void WA_UI_Listview_DeleteSelected(HWND hListview)
{
    INT nIndex, nSelectedCount, nCounter;
    INT* IntArray;

    if (!Globals2.pPlaylist)
        return;

    nSelectedCount = ListView_GetSelectedCount(hListview);

    if (nSelectedCount <= 0)
        return;

    IntArray = (INT*) calloc(nSelectedCount, sizeof(INT));

    if (!IntArray)
        return;  

    nIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);
    nCounter = 0;

    while (nIndex != -1)
    {
        ListView_SetItemState(hListview, nIndex, 0, LVNI_SELECTED);

        IntArray[nCounter] = nIndex;

        nCounter++;

        _RPT1(_CRT_WARN, "Index: %i \n", nIndex);

        nIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);
    }

    // 0-Based Index
    nCounter--;

    while (nCounter > -1)
    {
        WA_Playlist_Remove(Globals2.pPlaylist, IntArray[nCounter]);
        nCounter--;
    }

    free(IntArray);

    WA_Playlist_UpdateView(Globals2.pPlaylist, false);
    
}

static void WA_UI_Listview_ShowItemContextMenu(HWND hListview, int32_t x, int32_t y)
{
    HMENU hMenu, hSubMenu;
    DWORD dwMenuID;

    hMenu = LoadMenu(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDR_PLAYLIST_MENU));


    if (!hMenu)
        return;

    hSubMenu = GetSubMenu(hMenu, 0);

    // Handle SHIFT+F10 command to show context menu
    if ((x < 0) || (y < 0))
    {
        RECT HeaderRect;

        GetWindowRect(hListview, &HeaderRect);

        x = HeaderRect.left;
        y = HeaderRect.top;
    }

    dwMenuID = TrackPopupMenu(hSubMenu,
        TPM_NONOTIFY | TPM_RETURNCMD,
        x, y, 0,
        hListview, NULL);

    switch (dwMenuID)
    {
    case ID_FILE_PLAY:
    {
        INT nIndex;
        nIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);

        if (nIndex != -1)      
            MainWindow_Open_Playlist_Index((DWORD) nIndex);

        break;
    }
    case ID_FILE_DELETE:
        WA_UI_Listview_DeleteSelected(hListview);
        break;
    case ID_FILE_SELECTALL:
        ListView_SetItemState(hListview, -1, LVIS_SELECTED, LVIS_SELECTED);
        break;
    case ID_FILE_SELECTNONE:
        ListView_SetItemState(hListview, -1, 0, LVIS_SELECTED);
        break;
    case ID_FILE_CLEARPLAYLIST:

        if (Globals2.pPlaylist)
        {
            WA_Playlist_RemoveAll(Globals2.pPlaylist);
            WA_Playlist_UpdateView(Globals2.pPlaylist, false);
        }
    }


    DestroyMenu(hMenu);
}

// TODO: Spostare la funzione in un Thread per migliorare le prestazioni dell'UI
static bool WA_UI_Listview_ReadCallback(WA_Playlist_Metadata* pMetadata)
{    
    HANDLE hFile;
    WA_Input* pIn;
    WA_AudioFormat Format;
    uint64_t uDuration;

    // Read File size
    hFile = CreateFile(pMetadata->lpwFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER liSize;

        if (GetFileSizeEx(hFile, &liSize))
            pMetadata->dwFileSizeBytes = liSize.QuadPart;

        CloseHandle(hFile);       
    }
    else
    {
        pMetadata->dwFileSizeBytes = 0LL;
    }

    pIn = WA_Playback_Engine_Find_Decoder(pMetadata->lpwFilePath);

    if (!pIn)
    {   
        pMetadata->uFileDurationMs = 0U;
        ZeroMemory(&pMetadata->Metadata, sizeof(WA_AudioMetadata));
        return true;
    }

    pIn->WA_Input_GetFileInfo(pIn, pMetadata->lpwFilePath, &Format, &pMetadata->Metadata, &uDuration);
    pMetadata->uFileDurationMs = uDuration;

    // Use File Name if Tags are empty
    if ((wcslen(pMetadata->Metadata.Title) == 0) && (wcslen(pMetadata->Metadata.Artist) == 0))
    {
        wcsncpy_s(pMetadata->Metadata.Artist,
            WA_METADATA_MAX_LEN,
            PathFindFileName(pMetadata->lpwFilePath),
            WA_METADATA_MAX_LEN - 1);

        PathRemoveExtension(pMetadata->Metadata.Artist);
    }

    return true;   
}

static void WA_UI_Listview_UpdateCallback(bool bRedrawItems)
{

    if (bRedrawItems)
    {
        RedrawWindow(Globals2.hListView, NULL, NULL, RDW_INVALIDATE);
    }
    else
    {
        DWORD dwCount;

        dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);

        // See https://learn.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-listview_setitemcountex
        ListView_SetItemCountEx(Globals2.hListView, dwCount, LVSICF_NOSCROLL);
    }
}


static void WA_Listview_GetItem(NMLVDISPINFO* pInfo)
{
    WA_Playlist_Metadata* pMetadata;
    DWORD dwColumnID;

    pMetadata = WA_Playlist_Get_Item(Globals2.pPlaylist, pInfo->item.iItem);

    _ASSERT(pMetadata);

    if (!pMetadata)
        return;

    dwColumnID = WA_UI_Listview_GetIDFromIndex(pInfo->item.iSubItem);    

    if (pInfo->item.mask & LVIF_TEXT)
    {        
        switch (dwColumnID)
        {
        case WA_LISTVIEW_COLUMN_STATUS:
            if(pMetadata->bFileSelected)
                wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, L"->\0");
            else
                wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, L"\0");
            break;
        case WA_LISTVIEW_COLUMN_INDEX:
        {
            wchar_t Buffer[WA_LISTVIEW_PRINTF_MAX];
            swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%d\0", pInfo->item.iItem);
            
            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, Buffer);
            break;
        }
        case WA_LISTVIEW_COLUMN_TITLE_ARTIST:
        {
            wchar_t Buffer[WA_LISTVIEW_PRINTF_MAX];

            if(wcslen(pMetadata->Metadata.Title) != 0U)
                if (wcslen(pMetadata->Metadata.Artist) != 0U)
                    swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%s - %s\0", pMetadata->Metadata.Artist, pMetadata->Metadata.Title);
                else
                    swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%s\0", pMetadata->Metadata.Title);
            else
                swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%s\0", pMetadata->Metadata.Artist);

            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, Buffer);
            break;
        }
        case WA_LISTVIEW_COLUMN_ALBUM:
            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, pMetadata->Metadata.Album);
            break;
        case WA_LISTVIEW_COLUMN_DURATION:
        {
            wchar_t Buffer[WA_LISTVIEW_PRINTF_MAX];
            uint32_t uHour, uMinute, uSeconds;

            uSeconds = (uint32_t)(pMetadata->uFileDurationMs / 1000U);
            uMinute = uSeconds / 60;
            uHour = uMinute / 60;

            uSeconds = uSeconds % 60;
            uMinute = uMinute % 60;

            if (uHour > 0)
            {
                swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%02u:%02u:%02u\0", uHour, uMinute, uSeconds);
            }
            else
            {
                swprintf_s(Buffer, WA_LISTVIEW_PRINTF_MAX, L"%02u:%02u\0", uMinute, uSeconds);
            }      


            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, Buffer);
            break;
        }

        case WA_LISTVIEW_COLUMN_GENRE:
            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, pMetadata->Metadata.Genre);
            break;
        case WA_LISTVIEW_COLUMN_SIZE:
        {  
            wchar_t Buffer[WA_LISTVIEW_PRINTF_MAX];

            // see https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-strformatbytesizew
            StrFormatByteSize(pMetadata->dwFileSizeBytes, Buffer, WA_LISTVIEW_PRINTF_MAX);

            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, Buffer);
            break;
        }

        case WA_LISTVIEW_COLUMN_PATH:
            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, pMetadata->lpwFilePath);
            break;
        default:
            wcscpy_s(pInfo->item.pszText, pInfo->item.cchTextMax, L"\0");
            break;

        }
    }
}


static void WA_Listview_PrepCache(NMLVCACHEHINT *pCache)
{   
    WA_Playlist_UpdateCache(Globals2.pPlaylist, (DWORD) pCache->iFrom,(DWORD) pCache->iTo);
}

static LRESULT WA_Listview_FindItem(LPNMLVFINDITEM pFindItem)
{
    LVFINDINFOW* pFindInfo = &pFindItem->lvfi;
    DWORD dwStartIndex = (DWORD)(pFindItem->iStart) % WA_Playlist_Get_Count(Globals2.pPlaylist);
    DWORD dwFoundIndex;

    /*

    See: https://learn.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-lvfindinfoa
    and https://www.codeproject.com/Articles/7891/Using-virtual-lists

    LVFI_STRING
        Searches based on the item text. Unless additional values are specified, the item text
        of the matching item must exactly match the string pointed to by the psz member.
        However, the search is case-insensitive.
    */

    if ((pFindInfo->flags & LVFI_STRING) == 0)
        return -1;

    if (!Globals2.pPlaylist)
        return -1;

    if(!WA_Playlist_FindByFirstChar(Globals2.pPlaylist, 
        dwStartIndex, 
        pFindInfo->psz, 
        &dwFoundIndex))   
        return -1;

    return (LRESULT)dwFoundIndex;
}

/// <summary>
/// Reorder Listiew Selected Items
/// </summary>
static void WA_Listview_ReorderSelectedItems(HWND hListview, INT uTargetIndex)
{
    DWORD dwCount;
    INT uIndex, uSelectedCount, uOffset;
    DWORD* ItemsArray;

    if (uTargetIndex == -1)
        return;
    
    if (!Globals2.pPlaylist)
        return;

    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);

    if (dwCount < (DWORD)uTargetIndex)
        return;

    uSelectedCount = ListView_GetSelectedCount(hListview);

    if (uSelectedCount <= 0)
        return;

    uIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);
 
    if (uIndex == uTargetIndex)
        return;

    ItemsArray = (DWORD*)calloc(sizeof(DWORD), uSelectedCount);

    if (!ItemsArray)
        return;

    uOffset = 0;
    while (uIndex != -1)
    {
        ItemsArray[uOffset] = (DWORD) uIndex;
        uOffset++;
        ListView_SetItemState(hListview, uIndex, 0, LVNI_SELECTED);

        uIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);
        
    }

    WA_Playlist_ReorderIndexes(Globals2.pPlaylist, ItemsArray, uSelectedCount, (DWORD)uTargetIndex);

    free(ItemsArray);

    ListView_SetItemState(hListview, uTargetIndex, LVIS_SELECTED, LVNI_SELECTED);

}

static void WA_Listview_DBLCKL(HWND hListview, LPNMITEMACTIVATE pItemActivate)
{

    if (pItemActivate->iItem == -1)
        return;
 
    MainWindow_Open_Playlist_Index((DWORD)pItemActivate->iItem);  
}

static void WA_Listview_OpenSelectedItem(HWND hListview)
{
    INT nIndex;

    nIndex = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);

    if (nIndex != -1)
    {
        MainWindow_Open_Playlist_Index(nIndex);
    }
}


LRESULT CALLBACK WA_UI_Listview_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

    switch (uMsg)
    {
    case WM_NOTIFY: // Header custom draw
    {
        // Use Standard Listview if Light Theme is Enabled
        if (!DarkMode_IsEnabled())
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);


        LPNMHDR lpHdr = (LPNMHDR)lParam;

 // Disable warning Arithmetic overflow: '-' operation produces a negative 
 // unsigned result at compile time
#pragma warning( push )
#pragma warning( disable : 26454 )

        if (lpHdr->code & NM_CUSTOMDRAW)
        {
            LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;

            switch (nmcd->dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
            {               
                SetTextColor(nmcd->hdc, ColorPolicy_Get_TextOnBackground_Color());
                return CDRF_DODEFAULT;
            }
            }
        }
#pragma warning( pop )
    }
    case WM_THEMECHANGED:
    {
        HWND hHeader = ListView_GetHeader(hWnd);

        DarkMode_AllowDarkModeForWindow(hWnd, DarkMode_IsEnabled());
        DarkMode_AllowDarkModeForWindow(hHeader, DarkMode_IsEnabled());

        // Update Colors
        ListView_SetBkColor(hWnd, ColorPolicy_Get_Background_Color());
        ListView_SetTextColor(hWnd, ColorPolicy_Get_TextOnBackground_Color());

        SendMessage(hHeader, WM_THEMECHANGED, wParam, lParam);

        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

        break;
    }
    case WM_CONTEXTMENU:
    {
        HWND hHeader = ListView_GetHeader(hWnd);


        if (hHeader == (HWND)wParam)
        {
            WA_UI_Listview_ShowHeaderContextMenu((HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return TRUE;
        }
        else
        {     
        
            WA_UI_Listview_ShowItemContextMenu((HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return TRUE;
        }
    }
    case WM_MOUSEMOVE:
    {
        LVHITTESTINFO lvHitTest;
        int32_t nItem;
        RECT rcItem;

        if (!Globals2.bListviewDragging)
            break;

        Globals2.ptListviewDraggingCursor.x = GET_X_LPARAM(lParam);
        Globals2.ptListviewDraggingCursor.y = GET_Y_LPARAM(lParam);             

        lvHitTest.pt.x = Globals2.ptListviewDraggingCursor.x;
        lvHitTest.pt.y = Globals2.ptListviewDraggingCursor.y;

        /* Returns the index of the item at the specified position, if any, or -1 otherwise. */
        nItem = ListView_HitTest(hWnd, &lvHitTest);

        if (nItem == -1)
            break;

        if(!(lvHitTest.flags & LVHT_ONITEMLABEL))
            break;

        
        if (!ListView_GetItemRect(hWnd, nItem, &rcItem, LVIR_BOUNDS))
            break;
        

        Globals2.nListviewItem = nItem;
                                
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);    

        break;
    }
    case WM_LBUTTONUP:

        if (!Globals2.bListviewDragging)
            break;

        WA_Listview_ReorderSelectedItems(hWnd, Globals2.nListviewItem);
        Globals2.bListviewDragging = false;
        Globals2.nListviewItem = -1;
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE); 

        break;
    case WM_MOUSELEAVE:
    
        if (!Globals2.bListviewDragging)
            break;
        
        Globals2.bListviewDragging = false;
        Globals2.nListviewItem = -1;
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);        

        break;
    
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_RETURN:
            WA_Listview_OpenSelectedItem(hWnd);
            return 0;
        case VK_DELETE:
        case VK_BACK:
            WA_UI_Listview_DeleteSelected(hWnd);
            return 0;
        }           

        break;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}



LRESULT WA_UI_Listview_CustomDraw(HWND hWnd, LPNMLVCUSTOMDRAW lplvcd)
{
    switch (lplvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT :
    {     
      

        if (WA_UI_Listview_IsRowSelected(hWnd, (int) lplvcd->nmcd.dwItemSpec))
        {
            lplvcd->clrTextBk = ColorPolicy_Get_Secondary_Color();
            lplvcd->clrText = ColorPolicy_Get_TextOnSecondary_Color();

            /*
                TODO: Not sure if this is the correct method to change the color of a selected item.
                On windows 11 it seems to work fine.To be tested further...
            */
            lplvcd->nmcd.uItemState = CDIS_DEFAULT;         
        }
        else
        {
            if ((lplvcd->nmcd.dwItemSpec % 2) == 0)
            {
                lplvcd->clrTextBk = ColorPolicy_Get_Background_Color();
                lplvcd->clrText = ColorPolicy_Get_TextOnBackground_Color();
            }
                
            else
            {
                lplvcd->clrTextBk = ColorPolicy_Get_Primary_Color();
                lplvcd->clrText = ColorPolicy_Get_TextOnPrimary_Color();
            }
               
        }  


      

        return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
    }
    case CDDS_ITEMPOSTPAINT:
    {
        if(!Globals2.bListviewDragging)
            return CDRF_DODEFAULT;

        if (lplvcd->nmcd.dwItemSpec == Globals2.nListviewItem)          
            FrameRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, ColorPolicy_Get_TextOnBackground_Brush()); 
       
        return CDRF_SKIPDEFAULT;
    }
    default:
        return CDRF_DODEFAULT;
    }
}



LRESULT WA_UI_Listview_OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpHdr = (LPNMHDR)lParam;

    switch (lpHdr->code)
    {
    case NM_CUSTOMDRAW:
        return WA_UI_Listview_CustomDraw(lpHdr->hwndFrom, (LPNMLVCUSTOMDRAW)lpHdr);
    case LVN_GETDISPINFO:
        WA_Listview_GetItem((NMLVDISPINFO*)lParam);
        break;
    case LVN_ODCACHEHINT:
        WA_Listview_PrepCache((LPNMLVCACHEHINT)lParam);
        break;
    case LVN_ODFINDITEM:        
        return WA_Listview_FindItem((LPNMLVFINDITEM) lParam);
    case NM_DBLCLK:
        WA_Listview_DBLCKL(hWnd, (LPNMITEMACTIVATE)lParam);
        return TRUE;
    case LVN_BEGINDRAG:
    {
        // See: https://www.codeproject.com/Articles/1298/Rearrange-rows-in-a-ListView-control-by-drag-and-d
        LPNMLISTVIEW lpListItem = (LPNMLISTVIEW)lParam;

        Globals2.bListviewDragging = true;
        Globals2.ptListviewDraggingCursor = lpListItem->ptAction;    

        break;
    }
    case LVN_HOTTRACK:
        return TRUE;        
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);

}

static void WA_UI_Listview_InitColumns(HWND hListview)
{

    Columns[0].dwColumn = WA_LISTVIEW_COLUMN_STATUS;
    Columns[0].dwColumnWidth = 45;
    Columns[0].bIsVisible = true;
    Columns[0].dwColumnOrder = 0;
    Columns[0].dwColumnIndex = 0;
    Columns[0].dwColumnDefaultIndex = 0;
    Columns[0].uFlags = LVCFMT_LEFT;

    Columns[1].dwColumn = WA_LISTVIEW_COLUMN_INDEX;
    Columns[1].dwColumnWidth = 45;
    Columns[1].bIsVisible = true;
    Columns[1].dwColumnOrder = 1;
    Columns[1].dwColumnIndex = 1;
    Columns[1].dwColumnDefaultIndex = 1;
    Columns[1].uFlags = LVCFMT_RIGHT;

    Columns[2].dwColumn = WA_LISTVIEW_COLUMN_TITLE_ARTIST;
    Columns[2].dwColumnWidth = 200;
    Columns[2].bIsVisible = true;
    Columns[2].dwColumnOrder = 2;
    Columns[2].dwColumnIndex = 2;
    Columns[2].dwColumnDefaultIndex = 2;
    Columns[2].uFlags = LVCFMT_LEFT;

    Columns[3].dwColumn = WA_LISTVIEW_COLUMN_ALBUM;
    Columns[3].dwColumnWidth = 100;
    Columns[3].bIsVisible = true;
    Columns[3].dwColumnOrder = 3;
    Columns[3].dwColumnIndex = 3;
    Columns[3].dwColumnDefaultIndex = 3;
    Columns[3].uFlags = LVCFMT_LEFT;

    Columns[4].dwColumn = WA_LISTVIEW_COLUMN_DURATION;
    Columns[4].dwColumnWidth = 90;
    Columns[4].bIsVisible = true;
    Columns[4].dwColumnOrder = 4;
    Columns[4].dwColumnIndex = 4;
    Columns[4].dwColumnDefaultIndex = 4;
    Columns[4].uFlags = LVCFMT_RIGHT;


    Columns[5].dwColumn = WA_LISTVIEW_COLUMN_GENRE;
    Columns[5].dwColumnWidth = 100;
    Columns[5].bIsVisible = true;
    Columns[5].dwColumnOrder = 5;
    Columns[5].dwColumnIndex = 5;
    Columns[5].dwColumnDefaultIndex = 5;
    Columns[5].uFlags = LVCFMT_LEFT;

    Columns[6].dwColumn = WA_LISTVIEW_COLUMN_SIZE;
    Columns[6].dwColumnWidth = 90;
    Columns[6].bIsVisible = true;
    Columns[6].dwColumnOrder = 6;
    Columns[6].dwColumnIndex = 6;
    Columns[6].dwColumnDefaultIndex = 6;
    Columns[6].uFlags = LVCFMT_RIGHT;

    Columns[7].dwColumn = WA_LISTVIEW_COLUMN_PATH;
    Columns[7].dwColumnWidth = 200;
    Columns[7].bIsVisible = true;
    Columns[7].dwColumnOrder = 7;
    Columns[7].dwColumnIndex = 7;
    Columns[7].dwColumnDefaultIndex = 7;
    Columns[7].uFlags = LVCFMT_LEFT;

    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        WA_UI_Listview_AddColumn(hListview,
            WA_Listview_GetStringFromID(Columns[i].dwColumn),
            Columns[i].dwColumnWidth,
            Columns[i].dwColumnIndex,
            Columns[i].uFlags);
    }

}

HWND WA_UI_Listview_Create(HWND hOwner, PRECT pRect)
{
    HWND hListview;
    DWORD dwStyle;


    dwStyle = WS_CHILD | WS_VISIBLE |
        WS_VSCROLL | WS_HSCROLL |
        LVS_REPORT | LVS_SHOWSELALWAYS |
        LVS_OWNERDATA;

    hListview = CreateWindowEx(
        0,										//	dwExStyle
        WC_LISTVIEW, 							//	lpClassName
        L"WA_Playlist",							//	lpWindowName
        dwStyle,	                            //	dwStyle
        pRect->left,							//	x
        pRect->top,                             //	y
        pRect->right,							//	nWidth
        pRect->bottom,							//	nHeight
        hOwner,									//	hWndParent
        (HMENU)MW_ID_LISTVIEW,					//	hMenu
        GetModuleHandle(NULL),					//	hInstance
        NULL								    //	lpParam
    );


    // Set ListView style
    ListView_SetExtendedListViewStyle(hListview, LVS_EX_FULLROWSELECT |
        LVS_EX_LABELTIP |
        LVS_EX_DOUBLEBUFFER | 
        LVS_EX_HEADERDRAGDROP);

    // Hide Focus Dots
    SendMessage(hListview, WM_CHANGEUISTATE, (WPARAM)MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    // Set ListView Background color
    ListView_SetBkColor(hListview, ColorPolicy_Get_Background_Color());
    ListView_SetTextColor(hListview, ColorPolicy_Get_TextOnBackground_Color());

    // Create new Empty Playlist
    Globals2.pPlaylist = WA_Playlist_New(WA_PLAYLIST_INITIAL_MAX_SIZE,
        WA_UI_Listview_ReadCallback,
        WA_UI_Listview_UpdateCallback);
     

    // Create Columns
    WA_UI_Listview_InitColumns(hListview);

    return hListview;
}

VOID WA_UI_Listview_Destroy(HWND hListview)
{
    if (Globals2.pPlaylist)
        WA_Playlist_Delete(Globals2.pPlaylist);
}

VOID WA_UI_Listview_SaveSettings(HWND hListview)
{

    WA_Ini *pIni;
    wchar_t pBuffer[30];

    pIni = WA_Ini_New();

    // Skip Save Settings on Fail(Fatal Error)
    if (!pIni)
        return;

    // Reload View
    WA_UI_Listview_RebuildIdexes();
    WA_UI_Listview_RebuildOrder(hListview);
    WA_UI_Listview_RebuildWitdh(hListview);

    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        swprintf_s(pBuffer, 30, L"Listview_Column_%u\0", i);
        WA_Ini_Write_UInt32(pIni, Columns[i].dwColumn, pBuffer, L"ColumnID");
        WA_Ini_Write_UInt32(pIni, Columns[i].dwColumnOrder, pBuffer, L"ColumnOrder");
        WA_Ini_Write_UInt32(pIni, Columns[i].dwColumnWidth, pBuffer, L"ColumnWidth");
        WA_Ini_Write_Bool(pIni, Columns[i].bIsVisible, pBuffer, L"ColumnVisible");
    }

    WA_Ini_Delete(pIni);
}

VOID WA_UI_Listview_LoadSettings(HWND hListview)
{
    WA_Ini* pIni;
    wchar_t pBuffer[30];
    WA_Listview_Column TempColumns[WA_LISTVIEW_COLUMNS_COUNT];
    int nOrderArray[WA_LISTVIEW_COLUMNS_COUNT];
    DWORD dwIndex;

    pIni = WA_Ini_New();

    // Skip Load Settings on Fail(Fatal Error)
    if (!pIni)
        return;

    // Load Settings
    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        swprintf_s(pBuffer, 30U, L"Listview_Column_%u\0", i);

        TempColumns[i].bIsVisible = WA_Ini_Read_Bool(pIni, true, pBuffer, L"ColumnVisible");
        TempColumns[i].dwColumn = WA_Ini_Read_UInt32(pIni, 0, pBuffer, L"ColumnID");
        TempColumns[i].dwColumnOrder = WA_Ini_Read_UInt32(pIni, UINT32_MAX, pBuffer, L"ColumnOrder");
        TempColumns[i].dwColumnWidth = WA_Ini_Read_UInt32(pIni, 0, pBuffer, L"ColumnWidth");
    }

    // Set Saved Witdh
    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if ((TempColumns[i].dwColumnWidth > 0U) && 
            (TempColumns[i].dwColumnWidth != Columns[i].dwColumnWidth))
        {
            Columns[i].dwColumnWidth = TempColumns[i].dwColumnWidth;
            ListView_SetColumnWidth(hListview, i, Columns[i].dwColumnWidth);
        }

    }

    // Show Only Visible Columns
    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if (TempColumns[i].bIsVisible != Columns[i].bIsVisible)
        {
            WA_UI_Listview_HideColumn(hListview, Columns[i].dwColumn);
        }
    }

    // Reorder Columns
    dwIndex = 0U;
    for (DWORD i = 0U; i < WA_LISTVIEW_COLUMNS_COUNT; i++)
    {
        if ((Columns[i].bIsVisible) && (TempColumns[i].dwColumnOrder != UINT32_MAX))
        {
            nOrderArray[dwIndex] = TempColumns[i].dwColumnOrder;
            Columns[i].dwColumnOrder = TempColumns[i].dwColumnOrder;
            dwIndex++;
        }
    }

    if (dwIndex > 0U)
    {
        ListView_SetColumnOrderArray(hListview, dwIndex, nOrderArray);
    }


    WA_Ini_Delete(pIni);
}