#ifndef WA_UI_LISTVIEW_H
#define WA_UI_LISTVIEW_H

// Bit Mask of Columns
#define WA_LISTVIEW_COLUMN_STATUS           0x0001
#define WA_LISTVIEW_COLUMN_INDEX            0x0002
#define WA_LISTVIEW_COLUMN_TITLE_ARTIST     0x0004
#define WA_LISTVIEW_COLUMN_ALBUM            0x0008
#define WA_LISTVIEW_COLUMN_DURATION         0x0010
#define WA_LISTVIEW_COLUMN_GENRE            0x0020
#define WA_LISTVIEW_COLUMN_SIZE             0x0040
#define WA_LISTVIEW_COLUMN_PATH             0x0080

#define WA_LISTVIEW_COLUMNS_COUNT           8
#define WA_LISTVIEW_PRINTF_MAX              100

#define WA_LISTVIEW_CACHING_TIMEOUT         200 // In Ms

// Store Columns Order and Visibility
typedef struct TagWA_Listview_Column
{
    DWORD dwColumn;
    DWORD dwColumnIndex;
    DWORD dwColumnDefaultIndex;
    DWORD dwColumnOrder;
    DWORD dwColumnWidth;  
    INT uFlags;
    bool bIsVisible;
} WA_Listview_Column;



// Subclass Listview Header
LRESULT CALLBACK WA_UI_Listview_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Process WM_NOTIFY Message
LRESULT WA_UI_Listview_OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Create Main listview
HWND WA_UI_Listview_Create(HWND hOwner, PRECT pRect);
VOID WA_UI_Listview_Destroy(HWND hListview);

// Load or Save current layout
VOID WA_UI_Listview_SaveSettings(HWND hListview);
VOID WA_UI_Listview_LoadSettings(HWND hListview);

// Signal to Cache Thread that main window is ready
VOID WA_ListView_RunCacheThread();


#endif
