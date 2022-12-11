
#ifndef GLOBALS_H
#define GLOBALS_H

#include "DecoderManager.h"
#include "Visualization.h"
#include "GUI_OpenFileDialog.h"


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
	HWND hMainWindowHandle;
	HMENU hMainMenuHandle;
	HINSTANCE hMainWindowInstance;
	HIMAGELIST hToolbarImagelist;
	HWND hRebarHandle;
	HWND hToolbarHandle;
	HWND hStatusHandle;
	HWND hListViewHandle;
	HWND hStaticHandle;
	HWND hPositionTrackbarHandle;
	HWND hVolumeTrackbarHandle;
	DecoderManager* pDecoderManager;
	uint8_t PlaybackStatus;
	bool bFileIsOpen;
	bool bMouseDownOnPosition;
	bool bMouseDownOnVolume;
	Visualization* pVisual;
	bool bVisualEnabled;
	int32_t nLastPlayedIndex;
	int32_t nCurrentPlayingIndex;
} Globals;


struct
{
	bool bEnhancerEnable;
	bool bBoostEnable;
	bool bReverbEnable;
	float fBoostValue;
	float fBass;
	float fBassRange;
	float fTreble;
	float fTrebleRange;		

} Settings;


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EnhancerProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
void Enhancer_DspCallback(float* pLeftSamples, float* pRightSamples, uint32_t uSamplesSize);
void Enhancer_Update(uint32_t uSamplerate);
void Enhancer_Init();
void Enhancer_Destroy();

#endif