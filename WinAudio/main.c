
#include "stdafx.h"
#include "resource.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_UAHMenubar.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_UI_StatusBar.h"
#include "WA_UI_Trackbar.h"
#include "WA_UI_Rebar.h"
#include "WA_UI_ListView.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_PluginLoader.h"
#include "WA_Playback_Engine.h"
#include "WA_GEN_Playlist.h"
#include "WA_GEN_INI.h"
#include "WA_UI_Visualizations.h"
#include "Globals2.h"


const wchar_t *WA_CLASS_NAME = L"WinAudio_Player";
const wchar_t *WA_APP_NAME = L"WinAudio Player";
const wchar_t *WA_MUTEX_NAME = L"WAMUTEX-A8708208-1B22-484C-BD41-955BFDDA95BF";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND MainWindow_CreateMainWindow(HINSTANCE hInstance);
void MainWindow_StartMainLoop();
void MainWindow_ProcessStartupFiles(int32_t nParams, wchar_t **pArgs);
void MainWindow_CopyData(HWND hExistingWindow, int32_t nParams, wchar_t** pArgs);
HWND MainWindow_CreateToolbar(HWND hOwnerHandle);
HMENU MainWindow_CreateMenu(HWND hOwnerHandle);
void MainWindow_DestroyMenu();
HWND MainWindow_CreatePositionTrackbar(HWND hOwnerHandle);
HWND MainWindow_CreateVolumeTrackbar(HWND hOwnerHandle);
HWND MainWindow_CreateSpectrumBar(HWND hOwnerHandle);

HWND MainWindow_CreateStatus(HWND hOwnerHandle);
void MainWindow_Status_SetText(HWND hStatusHandle, wchar_t *pText, bool bResetMessage);
void MainWindow_UpdateStatusText(HWND hStatusHandle,
    uint32_t uSamplerate,
    uint32_t uChannels,
    uint16_t uBitsPerSample,
    uint64_t uPositionInMs);

// Listview Helpers
HWND MainWindow_CreateListView(HWND hOwnerHandle);
void MainWindow_DestroyListView();
void MainWindow_ListView_AddColumn(HWND hListboxHandle, wchar_t* pColumnText, int32_t ColumnWidth, int32_t ColumnIndex);
void MainWindow_ListView_DeleteColumn(HWND hListboxHandle, int32_t ColumnIndex);
void MainWindow_ListView_InitColumns(HWND hListboxHandle);
void MainWindow_ListView_InsertRow(HWND hListboxHandle, wchar_t* col1, wchar_t* col2);
void MainWindow_ListView_DeleteRow(HWND hListboxHandle, int32_t nIndex);
void MainWindow_ListView_DeleteAllRows(HWND hListboxHandle);
void MainWindow_ListView_SetPlayIndex(HWND hListboxHandle, int32_t nIndex);


void MainWindow_CreateUI(HWND hwndOwner);
void MainWindow_DestroyUI();
HWND MainWindow_CreateRebar(HWND hwndOwner);
void MainWindow_DestroyRebar();

void MainWindow_Resize(LPARAM lParam, BOOL blParamValid);
void MainWindow_HandleCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
bool MainWindow_OpenFileDialog(HWND hOwnerHandle);


void MainWindow_SplitFilePath(const wchar_t* pInPath, wchar_t* pFolder, wchar_t* pFileName);
DWORD MainWindow_HandleEndOfStreamMsg();
void MainWindow_UpdatePositionTrackbar(uint64_t uNewValue);
void MainWindow_DrawSpectrum();
void MainWindow_UpdateVolumeFromTrackbarValue();

void MainWindow_UpdateWindowTitle(const wchar_t* pString, bool bClear);

void MainWindow_LoadSettings();
void MainWindow_SaveSettings();

void MainWindow_InitDarkMode();
void MainWindow_DestroyDarkMode();

LRESULT CALLBACK PositionSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

LRESULT CALLBACK VolumeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


LRESULT MainWindow_HandleMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT MainWindow_HandleCopyData(HWND hWnd, WPARAM wParam, LPARAM lParam);



/// <summary>
/// Application Entry Point
/// </summary>
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{    
    HWND hExistingWindow = NULL;
    HANDLE hMutex = NULL;

    // Perform Memory Dump on Exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // Before Create new application, find if exist a previous opened instance
    hMutex = CreateMutex(NULL, TRUE, WA_MUTEX_NAME);

    if ((hMutex == NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
        hExistingWindow = FindWindow(WA_CLASS_NAME, NULL);
        

    // If we Found and opened instance, send
    // to them data and close current instance
    if (hExistingWindow != NULL)
    {
        // Send Data to Already open instance
        MainWindow_CopyData(hExistingWindow, __argc, __wargv);

        // Show Already Opened Window
        SetForegroundWindow(hExistingWindow);
        ShowWindow(hExistingWindow, SW_RESTORE);

        _RPTFW0(_CRT_WARN, L"Found an already opened instance\n");

        return EXIT_SUCCESS;
    }


    // Init COM and start application only if we don't found and 
    // opened instance
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if FAILED(hr)
    {
        MessageBox(NULL, L"Cannot Initialize COM", L"Fatal Error", MB_OK | MB_ICONSTOP);
        return EXIT_FAILURE;
    }
        

    // Init Common Controls
    INITCOMMONCONTROLSEX commonCtrl;
    commonCtrl.dwSize = sizeof(INITCOMMONCONTROLSEX);
    commonCtrl.dwICC =  ICC_BAR_CLASSES | 
                        ICC_STANDARD_CLASSES | 
                        ICC_COOL_CLASSES | 
                        ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&commonCtrl);
    
    MainWindow_LoadSettings();
    MainWindow_InitDarkMode(); 
        
    // Store Main Window Handle and Instance
    Globals2.hMainWindow = MainWindow_CreateMainWindow(hInstance);
    Globals2.hMainWindowInstance = hInstance;

    // Check if Window has a valid handle
    if (Globals2.hMainWindow == NULL)
    {
        MainWindow_DestroyDarkMode();
        CoUninitialize();

        if (hMutex)
            ReleaseMutex(hMutex);

        return ERROR_INVALID_HANDLE; 
    }     


    // Load Plugins
    if (!WA_Playback_Engine_New())
    {
        MainWindow_DestroyDarkMode();
        CoUninitialize();  

        if (hMutex)
         ReleaseMutex(hMutex);

        return EXIT_FAILURE;
    }
    
    // Process Pending in Arguments(Open and Play First file)
    if (__argc > 1)        
        MainWindow_ProcessStartupFiles(__argc, __wargv);
       
    
    // Show Main Window
    ShowWindow(Globals2.hMainWindow, nShowCmd);
    UpdateWindow(Globals2.hMainWindow);

    _RPTW1(_CRT_WARN, L"DPI: %u\n", GetDpiForWindow(Globals2.hMainWindow));

    // Run the message loop.
    MainWindow_StartMainLoop();   
 

    // Clean Memory
    MainWindow_SaveSettings();
    MainWindow_DestroyDarkMode();
    WA_Playback_Engine_Delete();
    CoUninitialize();

    if(hMutex)
        ReleaseMutex(hMutex);


    return EXIT_SUCCESS;
}



/// <summary>
/// Process Params from WinMain or WM_COPYDATA message
/// https://docs.microsoft.com/en-us/cpp/cpp/main-function-command-line-args?view=msvc-160
/// </summary>
void MainWindow_ProcessStartupFiles(int32_t nParams, wchar_t** pArgs)
{
    int32_t i;

    if (!Globals2.pPlaylist)
        return;

#if _DEBUG
    for (i = 0; i < nParams; i++)
    {
        _RPTFW1(_CRT_WARN, L"File: %s \n", pArgs[i]);
    }

#endif

    // i = 0 is the executable path
    for (i = 1; i < nParams; i++)
    {
        // Check if file exist
        if (PathFileExists(pArgs[i]))        
            WA_Playlist_Add(Globals2.pPlaylist, pArgs[i]);         
               
    }

    // Update Listview Count
    WA_Playlist_UpdateView(Globals2.pPlaylist, false);

    // Open and Play file at Index [0]
    if (MainWindow_Open_Playlist_Index(0)) 
        MainWindow_Play();
  
  
        


}

/// <summary>
/// Prepare a WM_COPYDATA message to send Files to an Existing Instance
/// </summary>
/// <param name="hExistingWindow">Handle to a Found Window</param>
/// <param name="nParams">__argc Param</param>
/// <param name="pArgs">__wargv Param</param>
void MainWindow_CopyData(HWND hExistingWindow, int32_t nParams, wchar_t** pArgs)
{
    
    COPYDATASTRUCT data;
  
    // Copy data only if we have params
    if (nParams < 2)
        return;
    

    for (int32_t i = 1; i < nParams; i++)
    {   

        data.cbData = (wcslen(pArgs[i]) + 1) * sizeof(wchar_t);
        data.dwData = (i == 1) ? MSG_OPENFILE : MSG_ENQUEUEFILE;
        data.lpData = pArgs[i];

        SendMessage(hExistingWindow, WM_COPYDATA, 0, (LPARAM)&data);
    } 
}



/// <summary>
/// Create and Register Main Window
/// </summary>
/// <param name="hInstance">hInstance from WinMain</param>
/// <returns>Handle of the window</returns>
HWND MainWindow_CreateMainWindow(HINSTANCE hInstance)
{
    WNDCLASS wc; 
    HWND hWindowHandle;  

    // Register the Window Class.   
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WA_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);


    RegisterClass(&wc);

    // Create the window.
    hWindowHandle = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        WA_CLASS_NAME,  
        WA_APP_NAME,    
        WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, MAINWINDOW_WIDTH, MAINWINDOW_HEIGHT,
        NULL,        
        NULL,       
        hInstance,  
        NULL        
    );

    return hWindowHandle;
}

/// <summary>
/// Start Win32 Window Message Loop
/// </summary>
void MainWindow_StartMainLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

}

/// <summary>
/// Window Message Procedure
/// </summary>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;

    if(DarkMode_IsEnabled()) // TODO: Decidere se toglierlo, il men� risulta molto chiaro
        if (UAHWndProc(hwnd, uMsg, wParam, lParam, &lr))
        {
            return lr;
        }

	switch (uMsg)
	{
	case WM_DESTROY:

        // Destroy Main Window UI
		MainWindow_DestroyUI();          

        // Close Message Loop
		PostQuitMessage(EXIT_SUCCESS);

		return 0;

	case WM_CREATE:
	{    
		// Create Main Window UI
		MainWindow_CreateUI(hwnd);  

		return 0;
	}    
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// Fill Background with window color
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);

		return 0;
	}
    case WM_COMMAND:
    {
        // Handle Commands
        MainWindow_HandleCommand(uMsg, wParam, lParam);
        return 0;
    }
    case WM_DRAWITEM:
    {
        if (wParam == MW_ID_SPECTRUM_STATIC)
        {
            if (lParam)
            {
                if (Globals2.pVisualizations)
                    WA_Visualizations_Clear(Globals2.pVisualizations);

                return true;
            }
            
        }
        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR lpHdr = (LPNMHDR)lParam;

        // Split for Control ID
        switch (lpHdr->idFrom)
        {
        case MW_ID_LISTVIEW:      
            return WA_UI_Listview_OnNotify(hwnd, uMsg, wParam, lParam);
        case MW_ID_REBAR:
        {
            switch (lpHdr->code)
            {
             // Change Listbox size when rebar size(height) change
            case RBN_HEIGHTCHANGE:
                MainWindow_Resize(lParam, false);
            }
            break;
        }
        case MW_ID_POSITION_TRACKBAR:
        {
            switch (lpHdr->code)
            {
            case NM_RELEASEDCAPTURE:
            {
                DWORD uPositionValue;
                uPositionValue = SendMessage(Globals2.hPositionTrackbar, TBM_GETPOS, 0, 0);
                
                // Seek only if playing
                if (Globals2.dwCurrentStatus == WA_STATUS_PLAY)
                    WA_Playback_Engine_Seek((uint64_t)uPositionValue);
               
                break;
            } 
            case NM_CUSTOMDRAW:
            {
                return WA_UI_Trackbar_CustomDraw(lpHdr->hwndFrom, (LPNMCUSTOMDRAW)lParam);
            }

            }

            break;
        }   
        case MW_ID_VOLUME_TRACKBAR:
        {       

            switch (lpHdr->code)
            {
            case NM_CUSTOMDRAW:                
                return WA_UI_Trackbar_CustomDraw(lpHdr->hwndFrom, (LPNMCUSTOMDRAW)lParam);
            }
        }
       
        }
        break;

    }
    case WM_SIZE:
        // Handle Resize Message
        MainWindow_Resize(lParam, true);
        return 0;
    case WM_TIMER:
    {
        UINT_PTR TimerID = (UINT_PTR)wParam;

        switch (TimerID)
        {
        case MW_ID_POS_TIMER:   
        {
            uint64_t uPositionMs = 0;

            if(WA_Playback_Engine_Get_Position(&uPositionMs))
                MainWindow_UpdatePositionTrackbar(uPositionMs);
        }
                   
        case MW_ID_SPECTRUM_TIMER:
            MainWindow_DrawSpectrum();
        }


        return 0;
    }
    case WM_COPYDATA:
    {
        return MainWindow_HandleCopyData(hwnd, wParam, lParam);
    } 
    case WM_SETTINGCHANGE:
    {
        
        if (DarkMode_IsColorSchemeChangeMessage(uMsg, lParam))
        {
            DarkMode_HandleThemeChange();
            DarkMode_RefreshTitleBarThemeColor(hwnd);

            ColorPolicy_Close();

            if (DarkMode_IsEnabled())
            {
                ColorPolicy_Init(Dark, Settings2.CurrentTheme);
                Settings2.CurrentMode = Dark;
            }
            else
            {
                ColorPolicy_Init(Light, Settings2.CurrentTheme);
                Settings2.CurrentMode = Light;
            }
            
            SendMessage(Globals2.hListView, WM_THEMECHANGED, 0, 0);

            ListView_SetBkColor(Globals2.hListView, ColorPolicy_Get_Background_Color());
            ListView_SetTextColor(Globals2.hListView, ColorPolicy_Get_TextOnBackground_Color());

            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
        }
        
    }
    case WM_WA_MSG:
        return MainWindow_HandleMessage(hwnd, wParam, lParam);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/// <summary>
/// Handle WM_COMMAND for MainWindow Message Loop
/// </summary>
void MainWindow_HandleCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*
        Message Source	WPARAM(HIWORD)		WPARAM(LOWORD)			LPARAM
        --------------------------------------------------------------------------
        Menu			0					Menu Identifier 		0
        Accelerator		1					Acelerator Identifier	0
        Control			Control - defined 	Control identifier		Handle to the
                        notification code							control window
    */

    WORD wControlId = LOWORD(wParam);
    WORD wMessageId = HIWORD(wParam); 
    HWND hControlHandle = (HWND)lParam;



    switch (wControlId)
    {
    case ID_FILE_OPENFILE:
    {
        // TODO: Verifica se va bene
        if (!MainWindow_OpenFileDialog(Globals2.hMainWindow))
            MessageBox(Globals2.hMainWindow, L"Unable to Show Open File Dialog", L"Open File Error", MB_ICONERROR | MB_OK);

        break;
    }
    case ID_FILE_ADDFILES:
    {
        // TODO: Add Files Command
        /*
        ShellFilesArray* pArray = NULL;
        DWORD dwFilesCount, i;   
        wchar_t Folder[MAX_PATH];
        wchar_t FileName[MAX_PATH];

        if (Shell_MultipleFilesOpenDialog(Globals.hMainWindowHandle, &pArray, &dwFilesCount))
        {

            if (pArray)
            {
                _ASSERT(dwFilesCount > 0);

                // Add Files to Playlist
                for (i = 0; i < dwFilesCount; i++)
                {
                    MainWindow_SplitFilePath(pArray[i].lpwsPath, Folder, FileName);
                    MainWindow_ListView_InsertRow(Globals.hListViewHandle, FileName, Folder);                       
                }
       

                free(pArray);
            }
        }
        */
        break;
    }
    case WM_TOOLBAR_PLAY:
        MainWindow_Play();       
        break;
    case WM_TOOLBAR_PAUSE:
        MainWindow_Pause();
        break;
    case WM_TOOLBAR_STOP:
        MainWindow_Stop();
        break;
    case ID_FILE_CLOSE:
        DestroyWindow(Globals2.hMainWindow);
        break;
    case ID_FILE_SETTINGS:        
        DialogBox(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDD_SETTINGS), Globals2.hMainWindow, SettingsProc);
        break;    
    case ID_PLAYBACK_PLAY:
        MainWindow_Play();
        break;
    case ID_PLAYBACK_PAUSE:
        MainWindow_Pause();
        break;
    case ID_PLAYBACK_STOP:
        MainWindow_Stop();
        break;
    case ID_PLAYBACK_PREVIOUS:
        MainWindow_PreviousItem();
        break;
    case ID_PLAYBACK_NEXT:
        MainWindow_NextItem();
        break;
    case WM_TOOLBAR_PREV:
        MainWindow_PreviousItem();
        break;
    case WM_TOOLBAR_NEXT:
        MainWindow_NextItem();
        break;

    }
}


/// <summary>
/// Create Application Toolbar, used in Create Rebar Function
/// </summary>
/// <param name="hOwnerHandle">Rebar Handle</param>
/// <returns>Toolbar Handle</returns>
HWND MainWindow_CreateToolbar(HWND hOwnerHandle)
{

    HICON hicon;  // handle to icon 

    // Create a masked image list large enough to hold the icons. 
    Globals2.hToolbarImagelist = ImageList_Create(MW_TOOLBAR_BITMAP_SIZE, MW_TOOLBAR_BITMAP_SIZE, ILC_COLOR32 | ILC_MASK, MW_TOOLBAR_BUTTONS, 0);

    // Load the icon resources, and add the icons to the image list
    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PREVIOUS_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PAUSE_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_STOP_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NEXT_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_OPEN_ICON));
    ImageList_AddIcon(Globals2.hToolbarImagelist, hicon);
    DestroyIcon(hicon);

    // Create toolbar
   Globals2.hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, 0,
            TBSTYLE_FLAT | CCS_NODIVIDER | WS_CHILD | 
            WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE,           
            0, 0, 0, 0,
       hOwnerHandle, (HMENU)MW_ID_TOOLBAR, GetModuleHandle(NULL), 0);


    // If an application uses the CreateWindowEx function to create the 
   // toolbar, the application must send this message to the toolbar before 
   // sending the TB_ADDBITMAP or TB_ADDBUTTONS message. The CreateToolbarEx 
   // function automatically sends TB_BUTTONSTRUCTSIZE, and the size of the 
   // TBBUTTON structure is a parameter of the function.
    SendMessage(Globals2.hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    

    // Set the image list.
    SendMessage(Globals2.hToolbar, TB_SETIMAGELIST,
        (WPARAM)MW_ID_TOOLBAR_IMAGELIST_NR,
        (LPARAM)Globals2.hToolbarImagelist);

    // Add buttons
    TBBUTTON tbButtons[MW_TOOLBAR_BUTTONS];

    ZeroMemory(&tbButtons, sizeof(tbButtons));

    tbButtons[0].iBitmap = MAKELONG(0, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[0].fsState = TBSTATE_ENABLED;
    tbButtons[0].fsStyle = BTNS_BUTTON;
    tbButtons[0].idCommand = WM_TOOLBAR_PREV;
    tbButtons[0].iString = (INT_PTR) L"Previous";

    tbButtons[1].iBitmap = MAKELONG(1, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[1].fsState = TBSTATE_ENABLED;
    tbButtons[1].fsStyle = BTNS_BUTTON;
    tbButtons[1].idCommand = WM_TOOLBAR_PLAY;
    tbButtons[1].iString = (INT_PTR) L"Play";

    tbButtons[2].iBitmap = MAKELONG(2, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[2].fsState = TBSTATE_ENABLED;
    tbButtons[2].fsStyle = BTNS_BUTTON;
    tbButtons[2].idCommand = WM_TOOLBAR_PAUSE;
    tbButtons[2].iString = (INT_PTR) L"Pause";

    tbButtons[3].iBitmap = MAKELONG(3, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[3].fsState = TBSTATE_ENABLED;
    tbButtons[3].fsStyle = BTNS_BUTTON;
    tbButtons[3].idCommand = WM_TOOLBAR_STOP;
    tbButtons[3].iString = (INT_PTR) L"Stop";

    tbButtons[4].iBitmap = MAKELONG(4, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[4].fsState = TBSTATE_ENABLED;
    tbButtons[4].fsStyle = BTNS_BUTTON;
    tbButtons[4].idCommand = WM_TOOLBAR_NEXT;
    tbButtons[4].iString = (INT_PTR) L"Next";

    tbButtons[5].iBitmap = 5;
    tbButtons[5].fsState = 0;
    tbButtons[5].fsStyle = BTNS_SEP;
    tbButtons[5].idCommand = 0;
    tbButtons[5].iString = (INT_PTR)L"";

    tbButtons[6].iBitmap = MAKELONG(5, MW_ID_TOOLBAR_IMAGELIST_NR);
    tbButtons[6].fsState = TBSTATE_ENABLED;
    tbButtons[6].fsStyle = BTNS_BUTTON;
    tbButtons[6].idCommand = WM_TOOLBAR_OPEN;
    tbButtons[6].iString = (INT_PTR)L"Add Files...";


    // Add buttons
    SendMessage(Globals2.hToolbar, TB_ADDBUTTONS, (WPARAM)MW_TOOLBAR_BUTTONS, (LPARAM)&tbButtons);

    // Show tooltips (https://docs.microsoft.com/en-us/windows/win32/controls/display-tooltips-for-buttons)
    SendMessage(Globals2.hToolbar, TB_SETMAXTEXTROWS, 0, 0);

    // Resize the toolbar, and then show it.
    SendMessage(Globals2.hToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(MW_TOOLBAR_BITMAP_SIZE, MW_TOOLBAR_BITMAP_SIZE));

    // Automatic size toolbar
    SendMessage(Globals2.hToolbar, TB_AUTOSIZE, 0, 0);

     
    return Globals2.hToolbar;
}

/// <summary>
/// Destroy Toolbar resources
/// </summary>
void MainWindow_DestroyToolbar()
{
    RemoveWindowSubclass(Globals2.hPositionTrackbar, PositionSubclassProc, MW_ID_POS_SUBCLASS);
    RemoveWindowSubclass(Globals2.hVolumeTrackbar, VolumeSubclassProc, MW_ID_VOL_SUBCLASS);

    ImageList_Destroy(Globals2.hToolbarImagelist);
}

/// <summary>
/// Create Main Window Statusbar
/// </summary>
/// <param name="hOwnerHandle">Main window Handle</param>
/// <returns>Statusbar Handle</returns>
HWND MainWindow_CreateStatus(HWND hOwnerHandle)
{    
    HWND hStatusHandle;

    // Create the status bar.
    hStatusHandle = CreateWindowEx(
        0,                       // no extended styles
        STATUSCLASSNAME,         // name of status bar class
        (PCTSTR)NULL,           // no text when first created
        SBARS_SIZEGRIP |         // includes a sizing grip
        WS_CHILD | WS_VISIBLE,   // creates a visible child window
        0, 0, 0, 0,              // ignores size and position
        hOwnerHandle,              // handle to parent window
        (HMENU)MW_ID_STATUS,          // child window identifier
        GetModuleHandle(NULL),     // handle to application instance
        NULL);                   // no window creation data

   // Create Simple toolbar
   SendMessage(hStatusHandle, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0); 

    /*
    INT Parts[3] = { 100, 340, -1 };

    SendMessage(hStatusHandle, SB_SETPARTS, 3, Parts);

    SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(0, SBT_NOBORDERS), (LPARAM)L"Testo 1\0");
    SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(1, SBT_NOBORDERS), (LPARAM)L"Testo 2 Lorem Ipsum\0");
    SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(2, SBT_NOBORDERS), (LPARAM)L"Testo 3 In Fondo 0:00\0");
    */

   //Init Theme
   WA_UI_Status_Init(hStatusHandle);

   // Subclass for Darkmode     
   SetWindowSubclass(hStatusHandle, WA_UI_Status_Proc, MW_ID_STATUS, 0);

   // Check here for resizing documentation 
   // https://docs.microsoft.com/en-us/windows/win32/controls/status-bars
   return hStatusHandle;
}

/// <summary>
/// Destroy Statusbar resources
/// </summary>
void MainWindow_DestroyStatus()
{
    RemoveWindowSubclass(Globals2.hStatus, WA_UI_Status_Proc, MW_ID_STATUS);
    WA_UI_Status_Close(Globals2.hStatus);
}

/// <summary>
/// Set Main Window Statusbar Message
/// </summary>
/// <param name="hStatusHandle">Handle to Main Window Statusbar</param>
/// <param name="pText">Pointer to Text value</param>
void MainWindow_Status_SetText(HWND hStatusHandle, wchar_t* pText, bool bResetMessage)
{
    if (bResetMessage)
    {
        wchar_t pWelcomeString[MAX_PATH];
        LoadString(GetModuleHandle(NULL), IDS_STATUS_WELCOME, pWelcomeString, MAX_PATH);

        SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(SB_SIMPLEID, 0), (LPARAM)pWelcomeString);
    }
    else
    {
        SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(SB_SIMPLEID, 0), (LPARAM)pText);   
    }

}

/// <summary>
/// Create ListView control (it fits the space between rebar and status)
/// </summary>
HWND MainWindow_CreateListView(HWND hOwnerHandle)
{
    HWND hListview;
    HWND hHeader;
    RECT WindowRect,RebarRect, ListRect, StatusRect;


    // Get Window Size
    GetClientRect(hOwnerHandle, &WindowRect);

    // Get Rebar size
    GetClientRect(Globals2.hRebar, &RebarRect);

    // Get Status size
    GetClientRect(Globals2.hStatus, &StatusRect);
    

    // Assign Listview Size
    ListRect.left = 0;
    ListRect.top = RebarRect.bottom;
    ListRect.right = WindowRect.right;
    ListRect.bottom = WindowRect.bottom - RebarRect.bottom - StatusRect.bottom;


    hListview = WA_UI_Listview_Create(hOwnerHandle, &ListRect);

    SetWindowSubclass(hListview, WA_UI_Listview_Proc, MW_ID_LISTVIEW, 0);

    hHeader = ListView_GetHeader(hListview);

    SetWindowTheme(hHeader, L"ItemsView", NULL); // DarkMode
    SetWindowTheme(hListview, L"ItemsView", NULL); // DarkMode

    // Load Listview Settings
    WA_UI_Listview_LoadSettings(hListview);

    return hListview;

}

/// <summary>
/// Insert a Row in the ListView
/// </summary>
void MainWindow_ListView_InsertRow(HWND hWnd, wchar_t* col1, wchar_t* col2)
{
    /*
    LV_ITEM		lvItem;
    int32_t nLastItem;

    // Insert Item after the last row
    nLastItem = ListView_GetItemCount(hWnd);

    lvItem.mask = 0;
    lvItem.iItem = nLastItem;
    lvItem.iSubItem = 0;
    lvItem.iItem = ListView_InsertItem(hWnd, &lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = L"";
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    ListView_SetItem(hWnd, &lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = 3;
    lvItem.pszText = col1;
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    ListView_SetItem(hWnd, &lvItem);

    lvItem.iSubItem = 5;
    lvItem.pszText = col2;
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    ListView_SetItem(hWnd, &lvItem);
    */
}

/// <summary>
/// Destroy ListView Control
/// </summary>
void MainWindow_DestroyListView()
{
    WA_UI_Listview_SaveSettings(Globals2.hListView);
    WA_UI_Listview_Destroy(Globals2.hListView);
    RemoveWindowSubclass(Globals2.hListView, WA_UI_Listview_Proc, MW_ID_LISTVIEW);
}



/// <summary>
/// Add Column to ListView
/// </summary>
void MainWindow_ListView_AddColumn(HWND hListboxHandle, wchar_t *pColumnText, int32_t ColumnWidth, int32_t ColumnIndex)
{
    /*
    LV_COLUMN columns;

    columns.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    columns.cx = ColumnWidth;
    columns.pszText = pColumnText;
    columns.cchTextMax = wcslen(columns.pszText);
    columns.iSubItem = ColumnIndex;

    

    ListView_InsertColumn(hListboxHandle, ColumnIndex, &columns);
    */

    
}

/// <summary>
/// Delete a Column from ListView
/// </summary>
void MainWindow_ListView_DeleteColumn(HWND hListboxHandle, int32_t ColumnIndex)
{
    // ListView_DeleteColumn(hListboxHandle, ColumnIndex);
}

/// <summary>
/// Create Listview Column Layouts
/// </summary>
void MainWindow_ListView_InitColumns(HWND hListboxHandle)
{
    /*
    MainWindow_ListView_AddColumn(Globals2.hListView, L"#", 20, 0);
    MainWindow_ListView_AddColumn(Globals2.hListView, L"File Name", 350, 3);
    MainWindow_ListView_AddColumn(Globals2.hListView, L"File Path", 250, 5);
    */
}

// TODO: Implement Function
void MainWindow_ListView_DeleteRow(HWND hListboxHandle, int32_t nIndex) 
{

}

/// <summary>
/// Delete All Items in Playlist
/// </summary>
void MainWindow_ListView_DeleteAllRows(HWND hListboxHandle)
{
    //ListView_DeleteAllItems(hListboxHandle);
}

/// <summary>
/// Highlight Item in Playing
/// </summary>
void MainWindow_ListView_SetPlayIndex(HWND hListboxHandle, int32_t nIndex)
{
    // Remove Current Item Selection
    /*
    if (Globals2.nCurrentPlayingIndex != MW_LW_INVALID_INDEX)
    {
        ListView_SetItemText(Globals2.hListView, Globals2.nCurrentPlayingIndex, 0, L"");
    }

    if (nIndex != MW_LW_INVALID_INDEX)
        ListView_SetItemText(Globals2.hListView, nIndex, 0, L">");    
  
   
    Globals2.nLastPlayedIndex = Globals2.nCurrentPlayingIndex;
    Globals2.nCurrentPlayingIndex = nIndex;
    */
}

/// <summary>
/// Create Main Window UI
/// </summary>
/// <param name="hMainWindow">= Parent Window Handle</param>
void MainWindow_CreateUI(HWND hMainWindow)
{

    // Switch to Dark Mode (If Enabled)
    if (DarkMode_IsSupported() && DarkMode_IsEnabled())
    {
        DarkMode_AllowDarkModeForWindow(hMainWindow, true);
        DarkMode_RefreshTitleBarThemeColor(hMainWindow);
        DarkMode_ApplyMica(hMainWindow);

    }

    // Create main controls
    Globals2.hMainMenu = MainWindow_CreateMenu(hMainWindow);
    Globals2.hStatus = MainWindow_CreateStatus(hMainWindow);
    Globals2.hRebar = MainWindow_CreateRebar(hMainWindow);
    Globals2.hListView = MainWindow_CreateListView(hMainWindow);


    // Create Visualizations resources for Our Static
    Globals2.pVisualizations = WA_Visualizations_New(Globals2.hStatic);

    if (Globals2.pVisualizations)
        WA_Visualizations_Clear(Globals2.pVisualizations);

    // Assign Initial Status to Stopped
    Globals2.dwCurrentStatus = MW_STOPPED;
    Globals2.bFileIsOpen = false;
    Globals2.bMouseDownOnPosition = false;
    Globals2.bMouseDownOnVolume = false;
    Globals2.bListviewDragging = false;



    // Show Welcome Message in status bar
    MainWindow_Status_SetText(Globals2.hStatus, NULL, true);  
}

/// <summary>
/// Destroy Loaded UI
/// </summary>
void MainWindow_DestroyUI()
{
    // Destroy Visualizations
    if(Globals2.pVisualizations)
        WA_Visualizations_Delete(Globals2.pVisualizations);
   
    MainWindow_DestroyRebar();
    MainWindow_DestroyStatus();
    MainWindow_DestroyListView();
    MainWindow_DestroyMenu();  
}

/// <summary>
/// Create a Rebar Control with Volume, Position, Toolbar and Static
/// </summary>
HWND MainWindow_CreateRebar(HWND hwndOwner)
{
    REBARINFO     rbi;
    REBARBANDINFO rbBand;
    HWND hRebarHandle;
    RECT WindowRect;
   
    // https://stackoverflow.com/questions/70939363/winapi-rebar-control-not-showing-up

    // Create Rebar
    hRebarHandle = CreateWindowEx(WS_EX_TOOLWINDOW,
                    REBARCLASSNAME,
                    NULL,
                    WS_VISIBLE |  WS_CHILD | WS_CLIPCHILDREN |
                    WS_CLIPSIBLINGS | RBS_BANDBORDERS |
                    CCS_NODIVIDER, 0, 0, 0, 0,
                    hwndOwner, 
                    (HMENU)MW_ID_REBAR, 
                    GetModuleHandle(NULL), NULL);


    // Check Rebar Handle
    if (!hRebarHandle)
        return NULL;

    // Initialize and send the REBARINFO structure.
    rbi.cbSize = sizeof(REBARINFO);  
    rbi.fMask = 0;
    rbi.himl = (HIMAGELIST)NULL;

    if (!SendMessage(hRebarHandle, RB_SETBARINFO, 0, (LPARAM)&rbi))
        return NULL;
 
    // Create Objects for Rebar Bands
    Globals2.hToolbar = MainWindow_CreateToolbar(hRebarHandle);
    Globals2.hPositionTrackbar = MainWindow_CreatePositionTrackbar(hRebarHandle);
    Globals2.hVolumeTrackbar = MainWindow_CreateVolumeTrackbar(hRebarHandle);      
    Globals2.hStatic = MainWindow_CreateSpectrumBar(hRebarHandle);    

    // Get current window size and position
    GetClientRect(hwndOwner, &WindowRect);   
   
    
    // Create common struct info
    ZeroMemory(&rbBand, sizeof(REBARBANDINFO));
    rbBand.cbSize = sizeof(REBARBANDINFO);  
    rbBand.fMask = RBBIM_TEXT | RBBIM_BACKGROUND |
        RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
        RBBIM_SIZE | RBBIM_ID;
    rbBand.fStyle = RBBS_CHILDEDGE | RBBS_NOGRIPPER;
    rbBand.hbmBack = NULL;     
  

    // Get the size of Toolbar button (loword = width - hiword = height)
    DWORD dwToolbarButtonSize = SendMessage(Globals2.hToolbar, TB_GETBUTTONSIZE, 0, 0);

    // Create Rebar bands
    rbBand.hwndChild = Globals2.hToolbar;
    rbBand.wID = MW_ID_TOOLBAR;
    rbBand.cxMinChild = MW_TOOLBAR_BUTTONS * LOWORD(dwToolbarButtonSize);
    rbBand.cyMinChild = MW_REBAR_HEIGHT;
    rbBand.cx = (UINT)((WindowRect.right - WindowRect.left) / 3);

    // Add the band that has the Toolbar
    SendMessage(hRebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);


    rbBand.hwndChild = Globals2.hVolumeTrackbar;
    rbBand.wID = MW_ID_VOLUME_TRACKBAR;
    rbBand.cxMinChild = MW_TRACKBAR_MIN_WIDTH;
    rbBand.cyMinChild = MW_REBAR_HEIGHT;  
    rbBand.cx = (UINT)((WindowRect.right - WindowRect.left ) / 3);
    

    // Add the band that has the Volume Trackbar
    SendMessage(hRebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);


    rbBand.hwndChild = Globals2.hStatic;
    rbBand.wID = MW_ID_SPECTRUM_STATIC;
    rbBand.cxMinChild = MW_STATIC_MIN_WIDTH + 50;
    rbBand.cyMinChild = MW_REBAR_HEIGHT;
    rbBand.cx = (UINT)((WindowRect.right - WindowRect.left) / 3);
    rbBand.fStyle = rbBand.fStyle | RBBS_FIXEDSIZE;

    // Add the band that has the Static Control
    SendMessage(hRebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);


    rbBand.hwndChild = Globals2.hPositionTrackbar;
    rbBand.wID = MW_ID_POSITION_TRACKBAR;
    rbBand.cxMinChild = (UINT)(WindowRect.right - WindowRect.left);
    rbBand.cyMinChild = MW_REBAR_HEIGHT;
    rbBand.cx = (UINT)(WindowRect.right - WindowRect.left);
    rbBand.fStyle =  RBBS_CHILDEDGE | RBBS_NOGRIPPER | RBBS_BREAK;

    // Add the band that has the Volume Trackbar
    SendMessage(hRebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand); 

    WA_UI_Rebar_Init(hRebarHandle);

    SetWindowSubclass(hRebarHandle, WA_UI_Rebar_Proc, MW_ID_REBAR, 0);
   

    return hRebarHandle;
}

/// <summary>
/// Destroy Rebar Resources
/// </summary>
void MainWindow_DestroyRebar()
{
    MainWindow_DestroyToolbar(); 
    RemoveWindowSubclass(Globals2.hRebar, WA_UI_Rebar_Proc, MW_ID_REBAR);
    WA_UI_Rebar_Close(Globals2.hRebar);
}

/// <summary>
/// Create Main Window Menu
/// </summary>
HMENU MainWindow_CreateMenu(HWND hOwnerHandle)
{
    HMENU hMenuHandle;

    hMenuHandle = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAIN_MENU));

    SetMenu(hOwnerHandle, hMenuHandle);   

    return hMenuHandle;
}

/// <summary>
/// Create Position Trackbar (Used in Rebar creation)
/// </summary>
HWND MainWindow_CreatePositionTrackbar(HWND hOwnerHandle)
{
    HWND hTrackbarHandle;


    hTrackbarHandle = CreateWindowEx(
                            0,                               // no extended styles 
                            TRACKBAR_CLASS,                  // class name 
                            L"Position Control",              // title (caption) 
                            WS_CHILD |
                            WS_VISIBLE |
                            TBS_NOTICKS |
                            TBS_TRANSPARENTBKGND | 
                            WS_DISABLED,              // style 
                            0, 0,                          // position 
                            0, 0,                         // size 
                            hOwnerHandle,                    // parent window 
                            (HMENU)MW_ID_POSITION_TRACKBAR,       // control identifier 
                            GetModuleHandle(NULL),            // instance 
                            NULL                             // no WM_CREATE parameter 
    );

    // Hide Focus Dots
    SendMessage(hTrackbarHandle, WM_CHANGEUISTATE, (WPARAM)MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    // Install a Subclass to process custom messages
    SetWindowSubclass(hTrackbarHandle, PositionSubclassProc, MW_ID_POS_SUBCLASS, 0);


    return hTrackbarHandle;

}

/// <summary>
/// Create Volume Trackbar (Used in Rebar creation)
/// </summary>
HWND MainWindow_CreateVolumeTrackbar(HWND hOwnerHandle)
{
    HWND hTrackbarHandle;


    hTrackbarHandle = CreateWindowEx(
        0,                               // no extended styles 
        TRACKBAR_CLASS,                  // class name 
        L"Volume Control",              // title (caption) 
        WS_CHILD |
        WS_VISIBLE |
        TBS_NOTICKS |
        TBS_TRANSPARENTBKGND |
        TBS_TOOLTIPS,              // style 
        0, 0,                          // position 
        0, 0,                         // size 
        hOwnerHandle,                    // parent window 
        (HMENU)MW_ID_VOLUME_TRACKBAR,       // control identifier 
        GetModuleHandle(NULL),            // instance 
        NULL                             // no WM_CREATE parameter 
    );

    // Set Min and Max Range
    SendMessage(hTrackbarHandle, TBM_SETRANGE, TRUE, MAKELPARAM(MW_VOLUME_MIN, MW_VOLUME_MAX));

    // Set Value to Default MAX VOLUME
    SendMessage(hTrackbarHandle, TBM_SETPOS, TRUE, MW_VOLUME_MAX);

    // Hide Focus Dots
    SendMessage(hTrackbarHandle, WM_CHANGEUISTATE, (WPARAM)MAKELONG(UIS_SET, UISF_HIDEFOCUS),0);

    // Install a Subclass to process custom messages
    SetWindowSubclass(hTrackbarHandle, VolumeSubclassProc, MW_ID_VOL_SUBCLASS, 0);

    return hTrackbarHandle;
}

/// <summary>
/// Create Static Control for Spectrum (Used in Rebar creation)
/// </summary>
HWND MainWindow_CreateSpectrumBar(HWND hOwnerHandle)
{
    HWND hStaticControl;

    hStaticControl = CreateWindowEx(
        0,                               // no extended styles 
        L"STATIC",                  // class name 
        L"",              // title (caption) 
        WS_CHILD |
        WS_VISIBLE |
        SS_OWNERDRAW     
        ,              // style 
        0, 0,                          // position 
        0, 0,                         // size 
        hOwnerHandle,                    // parent window 
        (HMENU)MW_ID_SPECTRUM_STATIC,       // control identifier 
        GetModuleHandle(NULL),            // instance 
        NULL                             // no WM_CREATE parameter 
    );

    return hStaticControl;

}

/// <summary>
/// Destroy Menu Resource
/// </summary>
void MainWindow_DestroyMenu()
{
    DestroyMenu(Globals2.hMainMenu);
}

/// <summary>
/// Resize Main Window Elements (lParam = New MainWindow Size in WM_SIZE)
/// </summary>
void MainWindow_Resize(LPARAM lParam, BOOL blParamValid)
{
    RECT RebarRect;
    RECT StatusRect;
    RECT ListRect;
    RECT WindowRect;

    // Resize Statusbar
    if (Globals2.hStatus && blParamValid)
        SendMessage(Globals2.hStatus, WM_SIZE, 0, lParam);

    // Resize Rebar
    if (Globals2.hRebar && blParamValid)
        SendMessage(Globals2.hRebar, WM_SIZE, 0, lParam);

    // Resize Listbox
    if ((Globals2.hListView) && (Globals2.hStatus) && (Globals2.hRebar))
    {
        // Get RWindow size
        GetClientRect(Globals2.hMainWindow, &WindowRect);

        // Get Rebar size
        GetClientRect(Globals2.hRebar, &RebarRect);

        // Get Status size
        GetClientRect(Globals2.hStatus, &StatusRect);

        // Calculate new size
        ListRect.left = 0;
        ListRect.top = RebarRect.bottom;
        ListRect.right = WindowRect.right;
        ListRect.bottom = WindowRect.bottom - StatusRect.bottom - RebarRect.bottom;

        // Resize ListVIew
        MoveWindow(Globals2.hListView, ListRect.left, ListRect.top, ListRect.right, ListRect.bottom, TRUE);
    }
        

}



/// <summary>
/// Show a File Dialog
/// </summary>
/// <param name="hOwnerHandle">Main Window Handle</param>
bool MainWindow_OpenFileDialog(HWND hOwnerHandle)
{  
    IFileOpenDialog* pFileOpen;     
    uint32_t uArrayCount;
    COMDLG_FILTERSPEC* pFilter = NULL;
    HRESULT hr;

    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Get Open File Dialog Filter
    uArrayCount = WA_Playback_Engine_GetExtFilter(&pFilter);

    // The function return 0 on fail
    if (uArrayCount == 0)
        return false;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (LPVOID*)(&pFileOpen));


    if SUCCEEDED(hr)
    {
        IFileOpenDialog_SetFileTypes(pFileOpen, uArrayCount, pFilter);
        IFileOpenDialog_SetFileTypeIndex(pFileOpen, 1U);

        IFileOpenDialog_SetTitle(pFileOpen, L"Open audio file(s)...");

        // Allow Multiple Selections
        IFileOpenDialog_SetOptions(pFileOpen, FOS_ALLOWMULTISELECT);

        // Show the Open dialog box.
        hr = IFileOpenDialog_Show(pFileOpen, hOwnerHandle);


        if SUCCEEDED(hr)
        {
            IShellItemArray* pItemsArray;
            DWORD dwFilesCount;

            hr = IFileOpenDialog_GetResults(pFileOpen, &pItemsArray);

            if SUCCEEDED(hr)
            {

                // Get The number of Selected Files
                hr = IShellItemArray_GetCount(pItemsArray, &dwFilesCount);


                if (SUCCEEDED(hr) && (dwFilesCount > 0))
                {
                    IShellItem* pItem;

                    // Clear Playlist
                    WA_Playlist_RemoveAll(Globals2.pPlaylist);

                    // Get The Path of Each Selected File
                    for (DWORD i = 0U; i < dwFilesCount; i++)
                    {

                        hr = IShellItemArray_GetItemAt(pItemsArray, i, &pItem);

                        if SUCCEEDED(hr)
                        {
                            LPWSTR pszFilePath;
                            hr = IShellItem_GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                            if SUCCEEDED(hr)
                            {

                                // Add item to Playlist
                                WA_Playlist_Add(Globals2.pPlaylist, pszFilePath);

                                CoTaskMemFree(pszFilePath);
                            }

                            IShellItem_Release(pItem);

                        }

                    }

                    // Update Listview Count
                    WA_Playlist_UpdateView(Globals2.pPlaylist, false);

                }


                IShellItemArray_Release(pItemsArray);
            }
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    // Free resources from WA_Playback_Engine_GetExtFilter function
    free(pFilter[0].pszSpec);
    free(pFilter);

    return true;
}

/// <summary>
/// Handle Play Command
/// </summary>
bool MainWindow_Play()
{

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus == MW_PLAYING)
        return false;

    if (Globals2.dwCurrentStatus == WA_STATUS_STOP)
    {
        if (!WA_Playback_Engine_Play())
            return false;
       
    }
    else
    {
        if (!WA_Playback_Engine_Resume())
            return false;
    }

    Globals2.dwCurrentStatus = MW_PLAYING;

    // Highlight Play button
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(TRUE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(FALSE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(FALSE, 0));

    // Update Menu
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PLAY, MF_CHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PAUSE, MF_UNCHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_STOP, MF_UNCHECKED);

    // Create Timer for position updates
    SetTimer(Globals2.hMainWindow, MW_ID_POS_TIMER, 1000, NULL);

    // Create Timer for Spectrum Analylis
    SetTimer(Globals2.hMainWindow, MW_ID_SPECTRUM_TIMER, 30, NULL);

    // Enable Position Trackbar
    EnableWindow(Globals2.hPositionTrackbar, true);

        
    return true;
}


/// <summary>
/// Handle Pause Command
/// </summary>
bool MainWindow_Pause()
{
    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus == MW_PAUSING)
        return false;

    if (!WA_Playback_Engine_Pause())
        return false;

    Globals2.dwCurrentStatus = MW_PAUSING;

    // Check Pause Button
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(TRUE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(FALSE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(FALSE, 0));

    // Update Menu
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PLAY, MF_UNCHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PAUSE, MF_CHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_STOP, MF_UNCHECKED);

    KillTimer(Globals2.hMainWindow, MW_ID_POS_TIMER);
    KillTimer(Globals2.hMainWindow, MW_ID_SPECTRUM_TIMER);

    // Disable Position Trackbar
    EnableWindow(Globals2.hPositionTrackbar, false);

    return true;
}

/// <summary>
/// Handle Stop Command
/// </summary>
bool MainWindow_Stop()
{
    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus == MW_STOPPED)
        return false;

    if (!WA_Playback_Engine_Stop())
        return false;

    Globals2.dwCurrentStatus = MW_STOPPED;

    // Check Stop Button
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(FALSE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(FALSE, 0));
    SendMessage(Globals2.hToolbar, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(TRUE, 0));

    // Update Menu
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PLAY, MF_UNCHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_PAUSE, MF_UNCHECKED);
    CheckMenuItem(Globals2.hMainMenu, ID_PLAYBACK_STOP, MF_CHECKED);

    KillTimer(Globals2.hMainWindow, MW_ID_POS_TIMER);
    KillTimer(Globals2.hMainWindow, MW_ID_SPECTRUM_TIMER);

    // Reset Position Trackbar to 0
    MainWindow_UpdatePositionTrackbar(0);

    // Clear Static Background
    if (Globals2.pVisualizations)
        WA_Visualizations_Clear(Globals2.pVisualizations);


    // Disable Position Trackbar
    EnableWindow(Globals2.hPositionTrackbar, false);


    // Reset status
    MainWindow_Status_SetText(Globals2.hStatus, NULL, true);
  
    return true;
}

/// <summary>
/// Go to next file into playlist
/// </summary>
/// <returns>True on Success</returns>
bool MainWindow_NextItem()
{
    DWORD dwIndex = 0;
    DWORD dwCount;

    if (!Globals2.pPlaylist)
        return false;

    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);

    if (dwCount < 1)
        return false;

    if (!WA_Playlist_Get_SelectedIndex(Globals2.pPlaylist, &dwIndex))
        return false;

    MainWindow_Close();

    dwIndex++;
    dwIndex = dwIndex % dwCount;
    
    return MainWindow_Open_Playlist_Index(dwIndex);
}

/// <summary>
/// Go to previous file into playlist
/// </summary>
/// <returns>True on Success</returns>
bool MainWindow_PreviousItem()
{
    DWORD dwIndex = 0;
    DWORD dwCount;

    if (!Globals2.pPlaylist)
        return false;

    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);

    if (dwCount < 1)
        return false;

    if (!WA_Playlist_Get_SelectedIndex(Globals2.pPlaylist, &dwIndex))
        return false;

    MainWindow_Close();

    if (dwIndex == 0)
        dwIndex = dwCount - 1;
    else
        dwIndex--;

  
    return MainWindow_Open_Playlist_Index(dwIndex);
}

/// <summary>
/// Open a file using an Index of Playlist
/// </summary>
/// <param name="dwIndex">0-BAsed Index</param>
/// <returns>True on Success</returns>
bool MainWindow_Open_Playlist_Index(DWORD dwIndex)
{
    WA_Playlist_Metadata* pMetadata;

    if (!Globals2.pPlaylist)
        return false;

    pMetadata = WA_Playlist_Get_Item(Globals2.pPlaylist, dwIndex);

    if (!pMetadata)
        return false;

    if (!MainWindow_Open(pMetadata->lpwFilePath))
        return false;


    // Update Listview
    if (Globals2.pPlaylist)
    {
        WA_Playlist_SelectIndex(Globals2.pPlaylist, dwIndex);
        WA_Playlist_UpdateView(Globals2.pPlaylist, true);
    }

    MainWindow_Play();

    return true;
}

/// <summary>
/// Open a file to play from a specified Path (not a dialog) 
/// </summary>
/// <param name="lpwPath">Path of file to open</param>
/// <returns>true on success, false otherwise</returns>
bool MainWindow_Open(const wchar_t* lpwPath)
{
    uint64_t uDuration = 0;
    WA_AudioFormat Format;

    if (Globals2.dwCurrentStatus != MW_STOPPED)
        MainWindow_Stop();

    if (Globals2.bFileIsOpen)
        MainWindow_Close();

    if (!WA_Playback_Engine_OpenFile(lpwPath))
        return false;

    // If Fail to Get Duration Disable Trackback for this audio stream
    Globals2.bStreamIsSeekable = WA_Playback_Engine_Get_Duration(&uDuration);

    // Set Trackbar Range (Only if stream is seekable)
    if (Globals2.bStreamIsSeekable)
    {
        SendMessage(Globals2.hPositionTrackbar, TBM_SETRANGEMIN, false, 0);
        SendMessage(Globals2.hPositionTrackbar, TBM_SETRANGEMAX, true, (LPARAM)uDuration);
    }

    // Update Visualizations
    if (WA_Playback_Engine_Get_Current_Format(&Format))
    {
        if(Globals2.pVisualizations)
            WA_Visualizations_UpdateFormat(Globals2.pVisualizations, 
                Format.uSamplerate, 
                Format.uChannels, 
                Format.uBitsPerSample);
    }

    // Update Main Window Title with file name    
    MainWindow_UpdateWindowTitle(PathFindFileName(lpwPath), false);

    return true;
}

bool MainWindow_Close()
{
    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_STOPPED)
        MainWindow_Stop();

    // Update Listview
    if (Globals2.pPlaylist)
    {
        DWORD dwIndex = 0;

        if (WA_Playlist_Get_SelectedIndex(Globals2.pPlaylist, &dwIndex))
        {
            WA_Playlist_DeselectIndex(Globals2.pPlaylist, dwIndex);
            WA_Playlist_UpdateView(Globals2.pPlaylist, true);
        }


    }

    // Clear Window Title
    MainWindow_UpdateWindowTitle(NULL, true);

    return WA_Playback_Engine_CloseFile();
}

/// <summary>
/// Split Input full path into file name and folder
/// </summary>
/// <param name="pInPath">Full path of a file</param>
/// <param name="pFolder">Folder of file</param>
/// <param name="pFileName">File name + Extension</param>
void MainWindow_SplitFilePath(const wchar_t* pInPath, wchar_t* pFolder, wchar_t* pFileName)
{    
    size_t nLastBackslash = wcsrchr(pInPath, L'\\') - pInPath;
    size_t nLen = wcsnlen(pInPath, MAX_PATH);

    wcsncpy_s(pFolder, MAX_PATH, pInPath, nLastBackslash);
    wcsncpy_s(pFileName, MAX_PATH, pInPath + nLastBackslash + 1, nLen - nLastBackslash);
}

/// <summary>
/// Update The Position Trackbar 
/// Timer Driven
/// </summary>
void MainWindow_UpdatePositionTrackbar(uint64_t uNewValue)
{
    WA_AudioFormat Format;

    // Update only if there is no mouse operations in progress
    if (!Globals2.bMouseDownOnPosition)
        SendMessage(Globals2.hPositionTrackbar, TBM_SETPOS, TRUE, (LPARAM)uNewValue);


    if (WA_Playback_Engine_Get_Current_Format(&Format))
    {
        MainWindow_UpdateStatusText(Globals2.hStatus,
            Format.uSamplerate,
            Format.uChannels,
            Format.uBitsPerSample,
            uNewValue);
    }

}

/// <summary>
/// Update Window on End of Stream Event
/// </summary>
DWORD MainWindow_HandleEndOfStreamMsg()
{
    _RPTW0(_CRT_WARN, L"Main Window End Of Stream\n");
    MainWindow_Stop();
    MainWindow_Close();

   
    // TODO: Play next file(if allowed in UI)

    return WA_OK;
}

/// <summary>
/// Subclass Position Trackbar to Handle Mouse Events
/// </summary>
LRESULT CALLBACK PositionSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        Globals2.bMouseDownOnPosition = true;
        break;
    case WM_LBUTTONUP:
        Globals2.bMouseDownOnPosition = false;
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK VolumeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        Globals2.bMouseDownOnVolume = true;  
        break;
    case WM_LBUTTONUP:
        Globals2.bMouseDownOnVolume = false;
        break;
    case WM_CAPTURECHANGED:
    { 
        MainWindow_UpdateVolumeFromTrackbarValue();

        break;
    }
    case WM_MOUSEMOVE:
    {
        // If Mouse is Down Update Volume
        if (Globals2.bMouseDownOnVolume)
        {
            MainWindow_UpdateVolumeFromTrackbarValue();            
        }

        break;
    }        
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}



/// <summary>
/// Draw Visualizations on Static Control
/// </summary>
void MainWindow_DrawSpectrum()
{
    int8_t* pBuffer;
    uint32_t uBufferSize;

    if (!Globals2.pVisualizations)
        return;

    pBuffer = WA_Visualizations_AllocBuffer(Globals2.pVisualizations, &uBufferSize);

    if (!pBuffer)
        return;


    if(WA_Playback_Engine_Get_Buffer(pBuffer, uBufferSize))
    {
        WA_Visualizations_Draw(Globals2.pVisualizations, pBuffer);
    }    


    WA_Visualizations_FreeBuffer(Globals2.pVisualizations, pBuffer);
  
}


/// <summary>
/// Update Volume From the value in the volume trackbar
/// </summary>
void MainWindow_UpdateVolumeFromTrackbarValue()
{
    DWORD dwVolumeValue;

    dwVolumeValue = SendMessage(Globals2.hVolumeTrackbar, TBM_GETPOS, 0, 0);
    dwVolumeValue = min(dwVolumeValue, UINT8_MAX);  

    WA_Playback_Engine_Set_Volume((uint8_t)dwVolumeValue);

}

/// <summary>
/// Update Main Window Text
/// </summary>
/// <param name="pString">Input String</param>
/// <param name="bClear">If true reset the title</param>
void MainWindow_UpdateWindowTitle(const wchar_t* pString, bool bClear)
{
    wchar_t pResultString[MAX_PATH];

    if (bClear)
    {
        SetWindowText(Globals2.hMainWindow, L"WinAudio\0");
    }
    else
    {
        if (swprintf_s(pResultString, MAX_PATH, L"%s - WinAudio\0", pString))
        {
            SetWindowText(Globals2.hMainWindow, pResultString);
        }
    }
  
}

/// <summary>
/// Update Status Bar Info Text
/// </summary>
void MainWindow_UpdateStatusText(HWND hStatusHandle, 
    uint32_t uSamplerate, 
    uint32_t uChannels, 
    uint16_t uBitsPerSample,
    uint64_t uPositionInMs)
{
    wchar_t StatusText[MAX_PATH];
    uint32_t uHour, uMinute, uSeconds;

    uSeconds = (uint32_t) (uPositionInMs / 1000U);
    uMinute = uSeconds / 60U;
    uHour = uMinute / 60U;

    uSeconds = uSeconds % 60U;
    uMinute = uMinute % 60U;

    StatusText[0] = L'\0'; 


    if (swprintf_s(StatusText, 
        MAX_PATH, 
        L"%u Hz | %u Channels | %u Bits | Playback: %02u:%02u:%02u", 
        uSamplerate, 
        uChannels,
        uBitsPerSample,
        uHour,
        uMinute,
        uSeconds))
    {
        MainWindow_Status_SetText(hStatusHandle, StatusText, false);
    }
}




LRESULT MainWindow_HandleMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwMessageID = (DWORD)wParam;

    switch (dwMessageID)
    {
    
    // Get or Set Current Playback Status
    case MSG_STATUS: 
    {
        WORD wStatusId = (WORD)lParam;

        switch (wStatusId)
        {
        case MSG_STATUS_PLAY:
            MainWindow_Play();
            return Globals2.dwCurrentStatus;
        case MSG_STATUS_PAUSE:
            MainWindow_Pause();
            return Globals2.dwCurrentStatus;
        case MSG_STATUS_STOP:
            MainWindow_Stop();
            return Globals2.dwCurrentStatus;
        case MSG_STATUS_GET:
            return Globals2.dwCurrentStatus;
        }
    }

    // Output Send This Message When It Stop to Play and Terminate
    // write thread and it is ready to be safe closed.
    // Main Window Must Close Input and Output and
    // if requied, play Next file on Playlist
    case MSG_NOTIFYENDOFSTREAM:
        return MainWindow_HandleEndOfStreamMsg();

    case MSG_SETVOLUME:
    {
        uint8_t dwVolumeValue;

        // Skip Invalid Values
        if (lParam > MW_VOLUME_MAX)
            return WA_ERROR_BADPARAM;

        dwVolumeValue = (uint8_t) lParam;     

        SendMessage(Globals2.hVolumeTrackbar, TBM_SETPOS, TRUE, dwVolumeValue);
        Globals2.dwCurrentVolume = dwVolumeValue;

        // Try to Update Output
        if (Globals2.pOutput)
            Globals2.pOutput->WA_Output_Set_Volume(Globals2.pOutput, dwVolumeValue);

        return WA_OK;
    }
    case MSG_GETVOLUME:
    {
        DWORD dwVolumeValue;

        dwVolumeValue = SendMessage(Globals2.hVolumeTrackbar, TBM_GETPOS, 0, 0);        

        return dwVolumeValue;
    }
    default:
        return WA_ERROR_BADMSG;
    }
}

LRESULT MainWindow_HandleCopyData(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    
    PCOPYDATASTRUCT lpCopy = (PCOPYDATASTRUCT)lParam;


    if (!Globals2.pPlaylist)
        return FALSE;

  
    switch (lpCopy->dwData)
    {
    case MSG_OPENFILE:
    {
        if (!PathFileExists((wchar_t*)lpCopy->lpData))
            return FALSE;

        if (!WA_Playback_Engine_IsFileSupported((wchar_t*)lpCopy->lpData))
            return FALSE;

        WA_Playlist_RemoveAll(Globals2.pPlaylist);

        WA_Playlist_Add(Globals2.pPlaylist, (wchar_t*) lpCopy->lpData);

        // Update Listview Count
        WA_Playlist_UpdateView(Globals2.pPlaylist, false);

        // Open and Play file at Index [0]
        if (MainWindow_Open_Playlist_Index(0)) 
            MainWindow_Play();
       
          

        break;
    }
    case MSG_ENQUEUEFILE:
    {
        if (!PathFileExists((wchar_t*)lpCopy->lpData))
            return FALSE;

        if (!WA_Playback_Engine_IsFileSupported((wchar_t*)lpCopy->lpData))
            return FALSE;
       
        WA_Playlist_Add(Globals2.pPlaylist, (wchar_t*)lpCopy->lpData);

        // Update Listview Count
        WA_Playlist_UpdateView(Globals2.pPlaylist, false);

        break;
    }
    }
   
    return TRUE;
}

void MainWindow_InitDarkMode()
{
    // Initialize Dark Mode
    DarkMode_Init(true);

    // Initialize ColorPolicy
    if (DarkMode_IsSupported() && DarkMode_IsEnabled())
    {
        ColorPolicy_Init(Dark, Settings2.CurrentTheme);
        Settings2.CurrentMode = Dark;
    }
    else
    {
        ColorPolicy_Init(Light, Settings2.CurrentTheme);
        Settings2.CurrentMode = Light;
    }
        
}


void MainWindow_DestroyDarkMode()
{
    DarkMode_Close();
    ColorPolicy_Close();
}

void MainWindow_LoadSettings()
{
    WA_Ini* pIni;

    pIni = WA_Ini_New();

    if (!pIni)
        return;

    Settings2.CurrentTheme = WA_Ini_Read_UInt8(pIni, (uint8_t) Red, L"Globals", L"ColorMode");
    Settings2.bPlayNextItem = WA_Ini_Read_Bool(pIni, false, L"Globals", L"PlayNextItem");

    WA_Ini_Delete(pIni);  
}

void MainWindow_SaveSettings()
{
    WA_Ini* pIni;

    pIni = WA_Ini_New();

    if (!pIni)
        return;

    WA_Ini_Write_UInt8(pIni, (uint8_t)Settings2.CurrentTheme, L"Globals", L"ColorMode");
    WA_Ini_Write_Bool(pIni, Settings2.bPlayNextItem, L"Globals", L"PlayNextItem");

    WA_Ini_Delete(pIni);

}

