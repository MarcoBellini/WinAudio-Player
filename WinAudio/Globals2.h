#ifndef GLOBALS2_H
#define GLOBALS2_H


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

#define MW_LW_INVALID_INDEX				-1

// Define Current Playback Statis
#define MW_PLAYING						0x01
#define MW_PAUSING						0x02
#define MW_STOPPED						0x03

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
	uint16_t dwCurrentVolume;
	bool bFileIsOpen;
	int32_t nLastPlayedIndex;
	int32_t nCurrentPlayingIndex;
	uint32_t uOutputLatency;		// Store Current Latency of Opened Output Plugin

	// TODO: Is This Userfull??
	bool bMouseDownOnPosition;
	bool bMouseDownOnVolume;

	WA_Input* pInput;	// Selected When a file is Processed and Open
	WA_Output* pOutput; // Selected in Settings
	WA_Effect* pEffect; // Selected in Settings	
} Globals2;


struct
{
	ColorMode Mode;
	ColorThemes Theme;

	wchar_t* lpwActiveOutputDescr;
	wchar_t* lpwActiveEffectDescr;
	bool bEffectIsActive;

} Settings2;

#endif
