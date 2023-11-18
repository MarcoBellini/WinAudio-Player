#ifndef GLOBALS2_H
#define GLOBALS2_H

// Increase at every commit
#define MW_ID_BUILD_NR					7

#define MAINWINDOW_WIDTH				640
#define MAINWINDOW_HEIGHT				480

#define MAINWINDOW_MIN_WIDTH			320
#define MAINWINDOW_MIN_HEIGHT			240

#define MW_ID_REBAR						1000
#define MW_ID_TOOLBAR					1001
#define MW_ID_MENU_TOOLBAR				1002
#define MW_ID_POSITION_TRACKBAR			1003
#define MW_ID_VOLUME_TRACKBAR			1004
#define MW_ID_SPECTRUM_STATIC			1005
#define MW_ID_STATUS					1006
#define MW_ID_LISTVIEW					1007
#define MW_ID_POS_TIMER					1008
#define MW_ID_POS_SUBCLASS				1009
#define MW_ID_SPECTRUM_TIMER			1010
#define MW_ID_VOL_SUBCLASS				1011
#define MW_ID_TOOLBAR_IMAGELIST_NR		0x0

#define WM_TOOLBAR_PLAY					(WM_APP + 0x010)
#define WM_TOOLBAR_PAUSE				(WM_APP + 0x011)
#define WM_TOOLBAR_STOP					(WM_APP + 0x012)
#define WM_TOOLBAR_NEXT					(WM_APP + 0x013)
#define WM_TOOLBAR_PREV					(WM_APP + 0x014)
#define WM_TOOLBAR_OPEN					(WM_APP + 0x015)

#define MW_TOOLBAR_BITMAP_SIZE			16
#define MW_TOOLBAR_BUTTONS				7


#define MW_REBAR_HEIGHT					24
#define MW_TRACKBAR_MIN_WIDTH			150
#define MW_STATIC_MIN_WIDTH				100

#define MW_VOLUME_MIN					0
#define MW_VOLUME_MAX					255

// Define Current Playback Status
#define MW_PLAYING						WA_STATUS_PLAY
#define MW_PAUSING						WA_STATUS_PAUSE
#define MW_STOPPED						WA_STATUS_STOP

#define MW_MAX_PLUGIN_DESC				127


// Main Window Globals Vars
struct
{
	HWND hMainWindow;				// Main Window Handle
	HMENU hMainMenu;				// Main Menu
	HINSTANCE hMainWindowInstance;
	HIMAGELIST hToolbarImagelist;
	HWND hRebar;
	HWND hToolbar;
	HWND hStatus;
	HWND hListView;
	HWND hStatic;
	HWND hPositionTrackbar;
	HWND hVolumeTrackbar;
	DWORD dwCurrentStatus;			// MW_PLAYING, MW_PAUSING, MW_STOPPED
	bool bFileIsOpen;
	bool bStreamIsSeekable;
	uint32_t uOutputLatency;		// Store Current Latency of Opened Output Plugin

	bool bMouseDownOnPosition;
	bool bMouseDownOnVolume;

	WA_Input* pInput;	// Selected When a file is Processed and Open
	WA_Output* pOutput; // Selected in Settings
	WA_Effect* pEffect; // Selected in Settings	

	WA_Playlist* pPlaylist; // Store Playlist Informations
	WA_Visualizations* pVisualizations; // Store Visualizations Handle

	bool bListviewDragging;
	POINT ptListviewDraggingCursor;
	int32_t nListviewItem;

	bool bPendingInParams; // Used to determine if load a playlist or process in Params

	LONG DropTargetReferenceCounter;
	IDropTarget DropTarget;
	bool bAllowFileDrop;
	FORMATETC DropFormat;
	IDropTargetHelper* pDropTargetHelper;
	bool bUseTargetHelper;

	TOOLINFO PositionToolInfo;
	HWND hPositionTool;


	HANDLE hCacheAbort;
	HANDLE hCacheSemaphore;
	HANDLE hCacheThread;
	CRITICAL_SECTION CacheThreadSection;
	bool bCacheThreadFail;


	HACCEL hAccel;
} Globals2;


struct
{
	ColorMode Mode;
	ColorThemes Theme;

	wchar_t pActiveOutputDescr[MW_MAX_PLUGIN_DESC];
	wchar_t pActiveEffectDescr[MW_MAX_PLUGIN_DESC];
	bool bEffectIsActive;

	ColorThemes CurrentTheme;
	ColorMode CurrentMode;

	bool bPlayNextItem;
	bool bSavePlaylistOnExit;
	bool bSaveWndSizePos;

	uint32_t uCurrentVolume;

	RECT MainWindowRect;

} Settings2;


// Share some window functions
bool MainWindow_Play();
bool MainWindow_Pause();
bool MainWindow_Stop();
bool MainWindow_Open(const wchar_t* wPath);
bool MainWindow_Open_Playlist_Index(DWORD dwIndex);
bool MainWindow_Close();
bool MainWindow_PreviousItem();
bool MainWindow_NextItem();
bool MainWindow_Back5Sec();
bool MainWindow_Fwd5Sec();

// Dialogs
INT_PTR CALLBACK SettingsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SearchDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif
