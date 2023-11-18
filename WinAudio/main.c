
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
#include "WA_GEN_Playback_Engine.h"
#include "WA_GEN_Playlist.h"
#include "..\WInAudio_SharedFunc\WA_GEN_INI.h"
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
HWND MainWindow_CreatePositionTooltip(HWND hOwnerHandle, TOOLINFO *pInfo);
HWND MainWindow_CreateVolumeTrackbar(HWND hOwnerHandle);
HWND MainWindow_CreateSpectrumBar(HWND hOwnerHandle);

HWND MainWindow_CreateStatus(HWND hOwnerHandle);
void MainWindow_Status_SetText(HWND hStatusHandle, wchar_t *pText, bool bResetMessage);
void MainWindow_UpdateStatusText(HWND hStatusHandle,
    uint32_t uSamplerate,
    uint32_t uChannels,
    uint16_t uBitsPerSample,
    uint64_t uPositionInMs,
    uint64_t uDurationMs);

// Listview Helpers
HWND MainWindow_CreateListView(HWND hOwnerHandle);
void MainWindow_DestroyListView();

void MainWindow_CreateUI(HWND hwndOwner);
void MainWindow_DestroyUI();
HWND MainWindow_CreateRebar(HWND hwndOwner);
void MainWindow_DestroyRebar();

void MainWindow_Resize(LPARAM lParam, BOOL blParamValid);
void MainWindow_HandleCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
bool MainWindow_OpenFileDialog(HWND hOwnerHandle);
bool MainWindow_AddFilesDialog(HWND hOwnerHandle);
bool MainWindow_AddFolderDialog(HWND hOwnerHandle);
bool MainWindow_OpenPlaylistDialog(HWND hOwnerHandle);
bool MainWindow_SaveAsPlaylistDialog(HWND hOwnerHandle);

DWORD MainWindow_HandleEndOfStreamMsg();
void MainWindow_UpdatePositionTrackbar(uint64_t uNewValue);
void MainWindow_DrawSpectrum();
void MainWindow_UpdateVolumeFromTrackbarValue();

void MainWindow_UpdateWindowTitle(const wchar_t* pString, bool bClearTitle);

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


HRESULT STDMETHODCALLTYPE QueryInterface(IDropTarget* This, REFIID riid, void** ppvObject);
ULONG STDMETHODCALLTYPE AddRef(IDropTarget* This);
ULONG STDMETHODCALLTYPE Release(IDropTarget* This);
HRESULT STDMETHODCALLTYPE DragEnter(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
HRESULT STDMETHODCALLTYPE DragOver(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
HRESULT STDMETHODCALLTYPE DragLeave(IDropTarget* This);
HRESULT STDMETHODCALLTYPE Drop(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

static IDropTargetVtbl DropTargetVtbl = {QueryInterface, AddRef, Release, DragEnter, DragOver, DragLeave, Drop};


static bool MainWindow_GetPlaylistSaveFolder(wchar_t* pFolderPath)
{
    if (!pFolderPath)
        return false;

    HRESULT hr;
    PWSTR lpwUserPath;

    // Get User Roadming Folder
    hr = SHGetKnownFolderPath(&FOLDERID_RoamingAppData,
        KF_FLAG_DEFAULT,
        NULL,
        &lpwUserPath);

    if FAILED(hr)
        return false;    

    // Create Path to our settings folder
    PathAppend(pFolderPath, lpwUserPath);
    PathAppend(pFolderPath, L"\\WinAudio");


    // Check if Directory Exist
    if (!PathFileExists(pFolderPath))
        CreateDirectory(pFolderPath, NULL);


    PathAppend(pFolderPath, L"\\Playlist.m3u8");

    // See doc: https://learn.microsoft.com/it-it/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath?redirectedfrom=MSDN
    if (lpwUserPath)
        CoTaskMemFree(lpwUserPath);

    return true;
}

/// <summary>
/// Restore Main window Position and Location after creation and after is shown
/// </summary>
static void MainWindow_RestoreWindowRectFromSettings()
{
    // Restore window position and size (if enabled in settings)
    if (Settings2.bSaveWndSizePos)
    {
        bool bResize = ((Settings2.MainWindowRect.right != 0) || (Settings2.MainWindowRect.left != 0) ||
            (Settings2.MainWindowRect.top != 0) || (Settings2.MainWindowRect.bottom != 0));
        if (bResize)
        {
            HDWP hDefer = BeginDeferWindowPos(10);

            if (hDefer)
            {
                int32_t nWidth = Settings2.MainWindowRect.right - Settings2.MainWindowRect.left;
                int32_t nHeight = Settings2.MainWindowRect.bottom - Settings2.MainWindowRect.top;
                int32_t nX = Settings2.MainWindowRect.left;
                int32_t nY = Settings2.MainWindowRect.top;

                hDefer = DeferWindowPos(hDefer, Globals2.hMainWindow, HWND_TOP, nX, nY, nWidth, nHeight, SWP_SHOWWINDOW);

                if (hDefer)
                    EndDeferWindowPos(hDefer);

            }

        }
    }
}


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

        // Show Existing Window
        SetForegroundWindow(hExistingWindow);
        ShowWindow(hExistingWindow, SW_RESTORE);

        _RPTFW0(_CRT_WARN, L"Found an already opened instance\n");

        return EXIT_SUCCESS;
    }

    // How it works:
    // true  = Skip loading playlist and process in params in MainWindow_ProcessStartupFiles(..)
    // false = load playlist only if Settings.bSavePlaylistOnExit == true
    Globals2.bPendingInParams = (__argc > 1) ? true : false;


    // Init COM and start application only if we don't found and 
    // opened instance
    //HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    HRESULT hr = OleInitialize(NULL);

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
        DestroyWindow(Globals2.hMainWindow);
        MainWindow_DestroyDarkMode();
        CoUninitialize();  

        if (hMutex)
         ReleaseMutex(hMutex);

        return EXIT_FAILURE;
    }
    
    // Process Pending in Arguments(Open and Play First file)
    if (Globals2.bPendingInParams)
        MainWindow_ProcessStartupFiles(__argc, __wargv);
       
    
    // Show Main Window
    ShowWindow(Globals2.hMainWindow, nShowCmd);
    UpdateWindow(Globals2.hMainWindow);

    // Run Playlist Caching Thread
    WA_ListView_RunCacheThread();


    // Load Keyboard accelerators
    Globals2.hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

    // Restore Saved Size and Position 
    // (must be called here, after window creation and after is shown)
    MainWindow_RestoreWindowRectFromSettings();

    // Run the message loop.
    MainWindow_StartMainLoop();   
 

    // Clean Memory
    MainWindow_SaveSettings();
    MainWindow_DestroyDarkMode();
    WA_Playback_Engine_Delete();

    OleUninitialize();

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
        {
            EnterCriticalSection(&Globals2.CacheThreadSection);
            WA_Playlist_Add(Globals2.pPlaylist, pArgs[i]);
            LeaveCriticalSection(&Globals2.CacheThreadSection);
        }
                   
               
    }

    // Update Listview Count
    EnterCriticalSection(&Globals2.CacheThreadSection);
    WA_Playlist_UpdateView(Globals2.pPlaylist, false);
    LeaveCriticalSection(&Globals2.CacheThreadSection);

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

        data.cbData = (DWORD)((wcslen(pArgs[i]) + 1) * sizeof(wchar_t));
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
        if (!TranslateAccelerator(Globals2.hMainWindow, Globals2.hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }

}

/// <summary>
/// Window Message Procedure
/// </summary>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;

    if(DarkMode_IsEnabled()) // TODO: Decidere se toglierlo, il menù risulta molto chiaro
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
		FillRect(hdc, &ps.rcPaint, ColorPolicy_Get_Background_Brush());

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
                uPositionValue = (DWORD) SendMessage(Globals2.hPositionTrackbar, TBM_GETPOS, 0, 0);
                
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
    
        if (!MainWindow_OpenFileDialog(Globals2.hMainWindow))
            MessageBox(Globals2.hMainWindow, L"Unable to Show File Dialog", L"WinAudio Error", MB_ICONERROR | MB_OK);

        break;
    }
    case ID_FILE_ADDFILES:
    {
        if (!MainWindow_AddFilesDialog(Globals2.hMainWindow))
            MessageBox(Globals2.hMainWindow, L"Unable to Show File Dialog", L"WinAudio Error", MB_ICONERROR | MB_OK);

        break;
    }
    case ID_FILE_ADDFOLDER:
    {
        if (!MainWindow_AddFolderDialog(Globals2.hMainWindow))
            MessageBox(Globals2.hMainWindow, L"Unable to Show Folder Dialog", L"WinAudio Error", MB_ICONERROR | MB_OK);

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
    case ID_ABOUT_INFO:
        DialogBox(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDD_ABOUT), Globals2.hMainWindow, AboutProc);
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
    case WM_TOOLBAR_OPEN:

        if (!MainWindow_AddFilesDialog(Globals2.hMainWindow))
            MessageBox(Globals2.hMainWindow, L"Unable to Show File Dialog", L"WinAudio Error", MB_ICONERROR | MB_OK);

        break;
    case ID_PLAYLIST_DELETEALL:
        if (Globals2.pPlaylist)
        {
            EnterCriticalSection(&Globals2.CacheThreadSection);
            WA_Playlist_RemoveAll(Globals2.pPlaylist);
            WA_Playlist_UpdateView(Globals2.pPlaylist, false);
            LeaveCriticalSection(&Globals2.CacheThreadSection);
        }

        break;
    case ID_PLAYLIST_OPEN:
        MainWindow_OpenPlaylistDialog(Globals2.hMainWindow);
        break;
    case ID_PLAYLIST_SAVEAS:
        MainWindow_SaveAsPlaylistDialog(Globals2.hMainWindow);
        break;
    case ID_PLAYLIST_SEARCH:
        DialogBox(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDD_SEARCH_DLG), Globals2.hMainWindow, SearchDlgProc);
        break;
    case ID_PLAYBACK_PREV5SECONDS:
        MainWindow_Back5Sec();
        break;
    case ID_PLAYBACK_FWD5SECONDS:
        MainWindow_Fwd5Sec();
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
    tbButtons[3].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
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

    /*
    In this function we load Listview settings, and create Playlist object
    used in main.c. We create also Caching thread and
    */
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
/// Destroy ListView Control
/// </summary>
void MainWindow_DestroyListView()
{
    WA_UI_Listview_SaveSettings(Globals2.hListView);
    RemoveWindowSubclass(Globals2.hListView, WA_UI_Listview_Proc, MW_ID_LISTVIEW);
    WA_UI_Listview_Destroy(Globals2.hListView);    
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
    Globals2.DropTargetReferenceCounter = 0L;

    // Load Playlist
    if ((Settings2.bSavePlaylistOnExit) && (Globals2.pPlaylist))
    {
        // Skip loading playlist if there are some In Params 
        if (!Globals2.bPendingInParams)
        {
            wchar_t PlaylistPath[MAX_PATH];

            if (MainWindow_GetPlaylistSaveFolder(PlaylistPath))
            {
                EnterCriticalSection(&Globals2.CacheThreadSection);
                WA_Playlist_LoadM3U(Globals2.pPlaylist, PlaylistPath);
                WA_Playlist_UpdateView(Globals2.pPlaylist, false);
                LeaveCriticalSection(&Globals2.CacheThreadSection);
            }           


        }

    }

    // Add Drag and Drop Capabilities
    Globals2.bUseTargetHelper = false;
    Globals2.DropTarget.lpVtbl = &DropTargetVtbl;

    RegisterDragDrop(hMainWindow, &Globals2.DropTarget);

    if SUCCEEDED(CoCreateInstance(&CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, &IID_IDropTargetHelper, (LPVOID*)&Globals2.pDropTargetHelper))
        Globals2.bUseTargetHelper = true;

    // Show Welcome Message in status bar
    MainWindow_Status_SetText(Globals2.hStatus, NULL, true);  

}

/// <summary>
/// Destroy Loaded UI
/// </summary>
void MainWindow_DestroyUI()
{
    // Store MainWindow RECT used in SaveSettings
    if (!GetWindowRect(Globals2.hMainWindow, &Settings2.MainWindowRect))
    {
        ZeroMemory(&Settings2.MainWindowRect, sizeof(RECT));
    }

    MainWindow_Close();

    if (Globals2.pDropTargetHelper)
        IDropTargetHelper_Release(Globals2.pDropTargetHelper);

    Globals2.pDropTargetHelper = NULL;

    RevokeDragDrop(Globals2.hMainWindow);

    // Save Playlist
    if ((Settings2.bSavePlaylistOnExit) && (Globals2.pPlaylist))
    {
        wchar_t PlaylistPath[MAX_PATH];

        if (MainWindow_GetPlaylistSaveFolder(PlaylistPath))
        {
            EnterCriticalSection(&Globals2.CacheThreadSection);
            WA_Playlist_SaveAsM3U(Globals2.pPlaylist, PlaylistPath);
            LeaveCriticalSection(&Globals2.CacheThreadSection);
        }
            
        
    }

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
    DWORD dwToolbarButtonSize = (DWORD) SendMessage(Globals2.hToolbar, TB_GETBUTTONSIZE, 0, 0);

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

    // Create Tracking Tooltip
    Globals2.hPositionTool = MainWindow_CreatePositionTooltip(hTrackbarHandle, &Globals2.PositionToolInfo);


    return hTrackbarHandle;

}

/// <summary>
/// Create Tracking Tooltip used in Position Trackbar
/// </summary>
HWND MainWindow_CreatePositionTooltip(HWND hOwnerHandle, TOOLINFO *pInfo)
{  

    // Create the tooltip. 
    HWND hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        hOwnerHandle, NULL,
        GetModuleHandle(NULL), NULL);


    ZeroMemory(pInfo, sizeof(TOOLINFO));
    pInfo->cbSize = sizeof(TOOLINFO);
    pInfo->hwnd = hOwnerHandle;
    pInfo->hinst = GetModuleHandle(NULL);
    pInfo->uId = (UINT_PTR)hOwnerHandle;
    pInfo->uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;;
    pInfo->lpszText = L"0:00 / 3.42";    

    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)pInfo);

    return hwndTip;
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

    // Set Value to Current Settings value (MW_MAX_VOLUME on Defaults)
    SendMessage(hTrackbarHandle, TBM_SETPOS, TRUE, Settings2.uCurrentVolume);

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
    wchar_t pTempStr[MAX_PATH];
    HRESULT hr;


    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Get Open File Dialog Filter
    uArrayCount = WA_Playback_Engine_GetExtFilter(&pFilter, pTempStr, MAX_PATH);

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
                    EnterCriticalSection(&Globals2.CacheThreadSection);
                    WA_Playlist_RemoveAll(Globals2.pPlaylist);
                    LeaveCriticalSection(&Globals2.CacheThreadSection);

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
                                if (WA_Playback_Engine_IsFileSupported(pszFilePath))
                                {
                                    EnterCriticalSection(&Globals2.CacheThreadSection);
                                    WA_Playlist_Add(Globals2.pPlaylist, pszFilePath);
                                    LeaveCriticalSection(&Globals2.CacheThreadSection);
                                }
                                    

                                CoTaskMemFree(pszFilePath);
                            }

                            IShellItem_Release(pItem);

                        }

                    }

                    // Update Listview Count
                    EnterCriticalSection(&Globals2.CacheThreadSection);
                    WA_Playlist_UpdateView(Globals2.pPlaylist, false);
                    LeaveCriticalSection(&Globals2.CacheThreadSection);

                    // Open First Index in the playlist
                    MainWindow_Open_Playlist_Index(0);

                }


                IShellItemArray_Release(pItemsArray);
            }
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    // Free resources from WA_Playback_Engine_GetExtFilter function
    free(pFilter);

    return true;
}

/// <summary>
/// Show a File Dialog
/// </summary>
/// <param name="hOwnerHandle">Main Window Handle</param>
bool MainWindow_AddFilesDialog(HWND hOwnerHandle)
{
    IFileOpenDialog* pFileOpen;
    uint32_t uArrayCount;
    COMDLG_FILTERSPEC* pFilter = NULL;
    wchar_t pTempStr[MAX_PATH];
    HRESULT hr;    

    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Get Open File Dialog Filter
    uArrayCount = WA_Playback_Engine_GetExtFilter(&pFilter, pTempStr, MAX_PATH);

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

        IFileOpenDialog_SetTitle(pFileOpen, L"Add audio file(s)...");

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
                                
                                if (WA_Playback_Engine_IsFileSupported(pszFilePath))
                                {
                                    EnterCriticalSection(&Globals2.CacheThreadSection);
                                    WA_Playlist_Add(Globals2.pPlaylist, pszFilePath);
                                    LeaveCriticalSection(&Globals2.CacheThreadSection);
                                }
                                   

                                CoTaskMemFree(pszFilePath);
                            }

                            IShellItem_Release(pItem);

                        }

                    }

                    // Update Listview Count
                    EnterCriticalSection(&Globals2.CacheThreadSection);
                    WA_Playlist_UpdateView(Globals2.pPlaylist, false);
                    LeaveCriticalSection(&Globals2.CacheThreadSection);

                }


                IShellItemArray_Release(pItemsArray);
            }
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    // Free resources from WA_Playback_Engine_GetExtFilter function
    free(pFilter);

    return true;
}

/// <summary>
/// Add supported files of a folder into the playlist
/// </summary>
/// <param name="pszFilePath">Folder Path</param>
static void MainWindow_Folder2Playlist(const LPWSTR pszFilePath)
{
    WIN32_FIND_DATA data;
    HANDLE hFind;
    wchar_t lpwFindPath[MAX_PATH];
    wchar_t lpwFilePath[MAX_PATH];

    wcscpy_s(lpwFindPath, MAX_PATH, pszFilePath);
    wcscat_s(lpwFindPath, MAX_PATH, L"\\*");

    hFind = FindFirstFile(lpwFindPath, &data);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        // Skip Folders
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        wcscpy_s(lpwFilePath, MAX_PATH, pszFilePath);
        wcscat_s(lpwFilePath, MAX_PATH, L"\\");
        wcscat_s(lpwFilePath, MAX_PATH, data.cFileName);

        if (WA_Playback_Engine_IsFileSupported(lpwFilePath))
        {
            EnterCriticalSection(&Globals2.CacheThreadSection);
            WA_Playlist_Add(Globals2.pPlaylist, lpwFilePath);
            LeaveCriticalSection(&Globals2.CacheThreadSection);
        }
           

    } while (FindNextFile(hFind, &data) != 0);


    FindClose(hFind);

    // Update Listview Count
    EnterCriticalSection(&Globals2.CacheThreadSection);
    WA_Playlist_UpdateView(Globals2.pPlaylist, false);
    LeaveCriticalSection(&Globals2.CacheThreadSection);
   

}

/// <summary>
/// Show a File Dialog
/// </summary>
/// <param name="hOwnerHandle">Main Window Handle</param>
bool MainWindow_AddFolderDialog(HWND hOwnerHandle)
{
    IFileOpenDialog* pFileOpen;
    HRESULT hr;

    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (LPVOID*)(&pFileOpen));


    if SUCCEEDED(hr)
    {

        IFileOpenDialog_SetTitle(pFileOpen, L"Select a folder to open...");

        // Allow Multiple Selections
        IFileOpenDialog_SetOptions(pFileOpen, FOS_PICKFOLDERS);

        // Show the Open dialog box.
        hr = IFileOpenDialog_Show(pFileOpen, hOwnerHandle);


        if SUCCEEDED(hr)
        {
            IShellItem* pFolder;
            

            hr = IFileOpenDialog_GetFolder(pFileOpen, &pFolder);

            if SUCCEEDED(hr)
            {

                LPWSTR pszFilePath;

                hr = IShellItem_GetDisplayName(pFolder, SIGDN_FILESYSPATH, &pszFilePath);

                if SUCCEEDED(hr)
                {
                    MainWindow_Folder2Playlist(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }

                IShellItem_Release(pFolder);
            }
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    return true;
}

/// <summary>
/// Show Playlist Open File Dialog
/// On success, tihs function open playlist from specified path
/// </summary>
bool MainWindow_OpenPlaylistDialog(HWND hOwnerHandle)
{
    IFileOpenDialog* pFileOpen;
    COMDLG_FILTERSPEC Filter;
    HRESULT hr;

    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (LPVOID*)(&pFileOpen));


    if SUCCEEDED(hr)
    {
        Filter.pszName = L"M3U8 Playlist";
        Filter.pszSpec = L"*.m3u8";

        IFileOpenDialog_SetFileTypes(pFileOpen, 1U, &Filter);
        IFileOpenDialog_SetFileTypeIndex(pFileOpen, 1U);

        IFileOpenDialog_SetTitle(pFileOpen, L"Open M3U8 Playlist...");

        // Allow Multiple Selections
        IFileOpenDialog_SetOptions(pFileOpen, FOS_FILEMUSTEXIST);

        // Show the Open dialog box.
        hr = IFileOpenDialog_Show(pFileOpen, hOwnerHandle);

        if SUCCEEDED(hr)
        {
            IShellItemArray* pItemsArray;       

            hr = IFileOpenDialog_GetResults(pFileOpen, &pItemsArray);

            if SUCCEEDED(hr)
            {           
                IShellItem* pItem;

                hr = IShellItemArray_GetItemAt(pItemsArray, 0, &pItem);

                if SUCCEEDED(hr)
                {
                    LPWSTR pszFilePath;
                    hr = IShellItem_GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                    if SUCCEEDED(hr)
                    {
                        // Clear Playlist and load new one
                        EnterCriticalSection(&Globals2.CacheThreadSection);
                        WA_Playlist_RemoveAll(Globals2.pPlaylist);                
                        WA_Playlist_LoadM3U(Globals2.pPlaylist, pszFilePath);
                        LeaveCriticalSection(&Globals2.CacheThreadSection);

                        CoTaskMemFree(pszFilePath);
                    }

                    IShellItem_Release(pItem);
                }              

                // Update Listview Count
                EnterCriticalSection(&Globals2.CacheThreadSection);
                WA_Playlist_UpdateView(Globals2.pPlaylist, false);         
                LeaveCriticalSection(&Globals2.CacheThreadSection);

                IShellItemArray_Release(pItemsArray);
            }
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    return true;
}

/// <summary>
/// Show Playlist File Save Dialog.
/// On success, this function save the playlist at specified path
/// </summary>
bool MainWindow_SaveAsPlaylistDialog(HWND hOwnerHandle)
{
    IFileSaveDialog* pFileSave;
    COMDLG_FILTERSPEC Filter;
    HRESULT hr;
    IShellItem* pItem;

    // Check if we have a working Playlist Object
    // Should be always valid
    if (!Globals2.pPlaylist)
        return false;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
        &IID_IFileSaveDialog, (LPVOID*)(&pFileSave));


    if SUCCEEDED(hr)
    {
        Filter.pszName = L"M3U8 Playlist";
        Filter.pszSpec = L"*.m3u8";
     
        IFileSaveDialog_SetFileTypes(pFileSave, 1U, &Filter);
        IFileSaveDialog_SetFileTypeIndex(pFileSave, 1U);

        IFileSaveDialog_SetTitle(pFileSave, L"Save M3U8 Playlist...");

        // Show the Save dialog box.
        hr = IFileSaveDialog_Show(pFileSave, hOwnerHandle);     
   
        if SUCCEEDED(hr)
        {
            hr = IFileSaveDialog_GetResult(pFileSave, &pItem);

            if SUCCEEDED(hr)
            {
                LPWSTR pszFilePath;
                hr = IShellItem_GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                if SUCCEEDED(hr)
                {
                   
                    EnterCriticalSection(&Globals2.CacheThreadSection);
                    WA_Playlist_SaveAsM3U(Globals2.pPlaylist, pszFilePath);
                    LeaveCriticalSection(&Globals2.CacheThreadSection);
                    CoTaskMemFree(pszFilePath);
                }

                IShellItem_Release(pItem);
            }
        }     

        IFileSaveDialog_Release(pFileSave);
        pFileSave = NULL;
    }

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

    EnterCriticalSection(&Globals2.CacheThreadSection);
    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);
    LeaveCriticalSection(&Globals2.CacheThreadSection);

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

    EnterCriticalSection(&Globals2.CacheThreadSection);
    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);
    LeaveCriticalSection(&Globals2.CacheThreadSection);

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
/// Move playback position back of 5 seconds
/// </summary>
bool MainWindow_Back5Sec()
{
    uint64_t uDuration, uPosition;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    if (!WA_Playback_Engine_Get_Duration(&uDuration))
        return false;

    if (!WA_Playback_Engine_Get_Position(&uPosition))
        return false;

    if (((int64_t)uPosition - 5000) < 0)
        uPosition = 0;
    else
        uPosition -= 5000;

    Sleep(50);

    MainWindow_UpdatePositionTrackbar(uPosition);

    return WA_Playback_Engine_Seek(uPosition);
}

/// <summary>
/// Move playback position forward of 5 seconds
/// </summary>
bool MainWindow_Fwd5Sec()
{
    uint64_t uDuration, uPosition;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    if (!WA_Playback_Engine_Get_Duration(&uDuration))
        return false;

    if (!WA_Playback_Engine_Get_Position(&uPosition))
        return false;

    if ((uPosition + 5000) > uDuration)
        return MainWindow_NextItem();
    else
        uPosition += 5000;

    Sleep(50);

    MainWindow_UpdatePositionTrackbar(uPosition);

    return WA_Playback_Engine_Seek(uPosition);
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

    EnterCriticalSection(&Globals2.CacheThreadSection);
    pMetadata = WA_Playlist_Get_Item(Globals2.pPlaylist, dwIndex);
    LeaveCriticalSection(&Globals2.CacheThreadSection);

    if (!pMetadata)
        return false;

    if (!MainWindow_Open(pMetadata->lpwFilePath))
        return false;


    // Update Listview
    if (Globals2.pPlaylist)
    {
        EnterCriticalSection(&Globals2.CacheThreadSection);
        WA_Playlist_SelectIndex(Globals2.pPlaylist, dwIndex);
        WA_Playlist_UpdateView(Globals2.pPlaylist, true);
        LeaveCriticalSection(&Globals2.CacheThreadSection);
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

        EnterCriticalSection(&Globals2.CacheThreadSection);
        if (WA_Playlist_Get_SelectedIndex(Globals2.pPlaylist, &dwIndex))
        {
            WA_Playlist_DeselectIndex(Globals2.pPlaylist, dwIndex);
            WA_Playlist_UpdateView(Globals2.pPlaylist, true);
        }
        LeaveCriticalSection(&Globals2.CacheThreadSection);

    }

    // Clear Window Title
    MainWindow_UpdateWindowTitle(NULL, true);

    return WA_Playback_Engine_CloseFile();
}


/// <summary>
/// Update The Position Trackbar 
/// Timer Driven
/// </summary>
void MainWindow_UpdatePositionTrackbar(uint64_t uNewValue)
{
    WA_AudioFormat Format;
    uint64_t uDuration;

    // Update only if there is no mouse operations in progress
    if (!Globals2.bMouseDownOnPosition)
        SendMessage(Globals2.hPositionTrackbar, TBM_SETPOS, TRUE, (LPARAM)uNewValue);


    if (WA_Playback_Engine_Get_Current_Format(&Format))
    {

        WA_Playback_Engine_Get_Duration(&uDuration);

        MainWindow_UpdateStatusText(Globals2.hStatus,
            Format.uSamplerate,
            Format.uChannels,
            Format.uBitsPerSample,
            uNewValue, uDuration);
    }

}

/// <summary>
/// Update Window on End of Stream Event
/// </summary>
DWORD MainWindow_HandleEndOfStreamMsg()
{    
    MainWindow_Stop();    

   
    // Play next file(if allowed in UI)
    if (Settings2.bPlayNextItem)
        MainWindow_NextItem(); 
    else
        MainWindow_Close();

    return WA_OK;
}

/// <summary>
/// Update the position of Tracking Tooltip that follows 
/// the Position Trackbar Thumb. The tooltip displays the
/// time of current position and the duration of the audio track
/// in this format: hh:mm:ss - hh:mm:ss
/// 
/// see also: https://learn.microsoft.com/en-us/windows/win32/controls/implement-tracking-tooltips
/// </summary>
static void MainWindow_UpdatePositionTooltipValue(HWND hPositionTrackbar, HWND hPositionTooltip, TOOLINFO* pInfo)
{
    RECT rcThumb;
    POINT ptToolPosition;
    DWORD dwTooltipSize, dwTrackbarValue, dwTrackbarMax;
    WORD TooltipWidth, TooltipHeight;
    DWORD PositionHour, PositionMinute, PositionSeconds;
    DWORD DurationHour, DurationMinute, DurationSeconds;
    wchar_t TimeStr[50];

    SendMessage(hPositionTrackbar, TBM_GETTHUMBRECT, 0, (LPARAM)&rcThumb);


    dwTooltipSize = (DWORD) SendMessage(Globals2.hPositionTool, TTM_GETBUBBLESIZE, 0, (LPARAM)pInfo);
    TooltipWidth = LOWORD(dwTooltipSize);
    TooltipHeight = HIWORD(dwTooltipSize);

    ptToolPosition.x = rcThumb.right;
    ptToolPosition.y = rcThumb.bottom;

    ClientToScreen(hPositionTrackbar, &ptToolPosition);

    ptToolPosition.x = ptToolPosition.x - (TooltipWidth / 2);
    ptToolPosition.y = ptToolPosition.y - (TooltipHeight * 2);

    dwTrackbarMax = (DWORD)SendMessage(hPositionTrackbar, TBM_GETRANGEMAX, 0, 0);
    dwTrackbarValue = (DWORD)SendMessage(hPositionTrackbar, TBM_GETPOS, 0, 0);

    // Curent Position in Ms
    PositionSeconds = dwTrackbarValue / 1000;
    PositionMinute = PositionSeconds / 60;
    PositionHour = PositionMinute / 60;

    PositionSeconds = PositionSeconds % 60;
    PositionMinute = PositionMinute % 60;

    // Current Duration in Ms
    DurationSeconds = dwTrackbarMax / 1000;
    DurationMinute = DurationSeconds / 60;
    DurationHour = DurationMinute / 60;

    DurationSeconds = DurationSeconds % 60;
    DurationMinute = DurationMinute % 60;

    ZeroMemory(TimeStr, sizeof(TimeStr));

    if(DurationHour > 0)
        swprintf_s(TimeStr, 50, L"%02u:%02u:%02u-%02u:%02u:%02u", PositionHour, PositionMinute, PositionSeconds, DurationHour, DurationMinute, DurationSeconds);
    else
        swprintf_s(TimeStr, 50, L"%02u:%02u-%02u:%02u", PositionMinute, PositionSeconds, DurationMinute, DurationSeconds);

    Globals2.PositionToolInfo.lpszText = TimeStr;

    SendMessage(hPositionTooltip, TTM_SETTOOLINFO, 0, (LPARAM)pInfo);

    SendMessage(hPositionTooltip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(ptToolPosition.x, ptToolPosition.y));

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
    {
        POINT ptMouse;
        RECT rcThumb, rcChannel;
        DWORD dwMin, dwMax, dwLogicPos, dwDelta, dwRange, dwMouseX;

        SendMessage(hWnd, TBM_GETTHUMBRECT, 0, (LPARAM)&rcThumb);

        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        // If mouse is on channel, convert mouse coordinates 
        // to a trackbar logic value
        if (!PtInRect(&rcThumb, ptMouse))
        {

            SendMessage(hWnd, TBM_GETCHANNELRECT, 0, (LPARAM)&rcChannel);

            dwMin = (DWORD)SendMessage(hWnd, TBM_GETRANGEMIN, 0, 0);
            dwMax = (DWORD)SendMessage(hWnd, TBM_GETRANGEMAX, 0, 0);

            dwRange = rcChannel.right - rcChannel.left;

            if (ptMouse.x < rcChannel.left)
            {
                dwMouseX = 0;
            }
            else
            {
                dwMouseX = ptMouse.x - rcChannel.left;
            }

            dwMouseX = min(dwMouseX, dwRange);

            dwDelta = dwRange - (dwRange - dwMouseX);

            dwLogicPos = ((dwDelta * (dwMax - dwMin)) / dwRange) + dwMin;

            SendMessage(hWnd, TBM_SETPOS, (WPARAM) TRUE, (LPARAM)dwLogicPos);

            // Seek only if playing
            if (Globals2.dwCurrentStatus == WA_STATUS_PLAY)
                WA_Playback_Engine_Seek((uint64_t)dwLogicPos);

            return 0;
        }
        else
        {
            Globals2.bMouseDownOnPosition = true;

            // Activate the tooltip.
            SendMessage(Globals2.hPositionTool, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&Globals2.PositionToolInfo);

            MainWindow_UpdatePositionTooltipValue(hWnd, Globals2.hPositionTool, &Globals2.PositionToolInfo);
        }

        break;
    }
    case WM_LBUTTONUP:
        Globals2.bMouseDownOnPosition = false;

        // Deactivate the tooltip.
        SendMessage(Globals2.hPositionTool, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&Globals2.PositionToolInfo);

        break;
    case WM_MOUSEMOVE:
    {
        // If Mouse is Down Update Tooltip value and position
        if (Globals2.bMouseDownOnPosition)
        {
            MainWindow_UpdatePositionTooltipValue(hWnd, Globals2.hPositionTool, &Globals2.PositionToolInfo);          
        }

        break;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/// <summary>
/// Subclass Volume Trackbar to Handle Mouse Events
/// </summary>
LRESULT CALLBACK VolumeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT ptMouse;
        RECT rcThumb, rcChannel;
        DWORD dwMin, dwMax, dwLogicPos, dwDelta, dwRange, dwMouseX;       

        SendMessage(hWnd, TBM_GETTHUMBRECT, 0, (LPARAM)&rcThumb);

        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        // If mouse is on channel, convert mouse coordinates 
        // to a trackbar logic value
        if (!PtInRect(&rcThumb, ptMouse))
        {
            
            SendMessage(hWnd, TBM_GETCHANNELRECT, 0, (LPARAM)&rcChannel);

            dwMin = (DWORD)SendMessage(hWnd, TBM_GETRANGEMIN, 0, 0);
            dwMax = (DWORD)SendMessage(hWnd, TBM_GETRANGEMAX, 0, 0);

            dwRange = rcChannel.right - rcChannel.left;

            if (ptMouse.x < rcChannel.left)
            {
                dwMouseX = 0;
            }
            else
            {
              dwMouseX = ptMouse.x - rcChannel.left;     
            }            

            dwMouseX = min(dwMouseX, dwRange);

            dwDelta = dwRange - (dwRange - dwMouseX);

            dwLogicPos = ((dwDelta * (dwMax - dwMin)) / dwRange) + dwMin;

            SendMessage(hWnd, TBM_SETPOS, (WPARAM) TRUE, (LPARAM)dwLogicPos);   

            MainWindow_UpdateVolumeFromTrackbarValue();

            return 0;
        }
        else
        {
            Globals2.bMouseDownOnVolume = true;
        }
       
        break;
    }
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

    dwVolumeValue = (DWORD) SendMessage(Globals2.hVolumeTrackbar, TBM_GETPOS, 0, 0);
    dwVolumeValue = min(dwVolumeValue, UINT8_MAX);  

    WA_Playback_Engine_Set_Volume((uint8_t)dwVolumeValue);

    Settings2.uCurrentVolume = dwVolumeValue;
}

/// <summary>
/// Update Main Window Text
/// </summary>
/// <param name="pString">Input String</param>
/// <param name="bClear">If true reset the title</param>
void MainWindow_UpdateWindowTitle(const wchar_t* pString, bool bClearTitle)
{
    wchar_t pResultString[MAX_PATH];

    if (bClearTitle)
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
    uint64_t uPositionInMs,
    uint64_t uDurationMs)
{
    wchar_t StatusText[MAX_PATH];
    uint32_t uHour, uMinute, uSeconds;
    uint32_t uHourD, uMinuteD, uSecondsD;

    // Position
    uSeconds = (uint32_t) (uPositionInMs / 1000U);
    uMinute = uSeconds / 60U;
    uHour = uMinute / 60U;

    uSeconds = uSeconds % 60U;
    uMinute = uMinute % 60U;

    // Duration
    uSecondsD = (uint32_t)(uDurationMs / 1000U);
    uMinuteD = uSecondsD / 60U;
    uHourD = uMinuteD / 60U;

    uSecondsD = uSecondsD % 60U;
    uMinuteD = uMinuteD % 60U;

    StatusText[0] = L'\0'; 

    if (uHourD > 0)
    {
        if (swprintf_s(StatusText,
            MAX_PATH,
            L"%u Hz | %u Channels | %u Bits | Playback: %02u:%02u:%02u - %02u:%02u:%02u",
            uSamplerate,
            uChannels,
            uBitsPerSample,
            uHour,
            uMinute,
            uSeconds,
            uHourD,
            uMinuteD,
            uSecondsD))
        {
            MainWindow_Status_SetText(hStatusHandle, StatusText, false);
        }
    }
    else
    {
        if (swprintf_s(StatusText,
            MAX_PATH,
            L"%u Hz | %u Channels | %u Bits | Playback: %02u:%02u - %02u:%02u",
            uSamplerate,
            uChannels,
            uBitsPerSample,            
            uMinute,
            uSeconds,
            uMinuteD,
            uSecondsD))
        {
            MainWindow_Status_SetText(hStatusHandle, StatusText, false);
        }
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

        MainWindow_UpdateVolumeFromTrackbarValue();

        return WA_OK;
    }
    case MSG_GETVOLUME:
    {
        DWORD dwVolumeValue;

        dwVolumeValue = (DWORD) SendMessage(Globals2.hVolumeTrackbar, TBM_GETPOS, 0, 0);

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

        EnterCriticalSection(&Globals2.CacheThreadSection);
        WA_Playlist_RemoveAll(Globals2.pPlaylist);
        WA_Playlist_Add(Globals2.pPlaylist, (wchar_t*) lpCopy->lpData);

        // Update Listview Count
        WA_Playlist_UpdateView(Globals2.pPlaylist, false);
        LeaveCriticalSection(&Globals2.CacheThreadSection);

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
       
        EnterCriticalSection(&Globals2.CacheThreadSection);
        WA_Playlist_Add(Globals2.pPlaylist, (wchar_t*)lpCopy->lpData);

        // Update Listview Count
        WA_Playlist_UpdateView(Globals2.pPlaylist, false);
        LeaveCriticalSection(&Globals2.CacheThreadSection);

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
    Settings2.uCurrentVolume = WA_Ini_Read_UInt32(pIni, 255, L"Globals", L"CurrentVolume");
    Settings2.bSavePlaylistOnExit = WA_Ini_Read_Bool(pIni, true, L"Globals", L"SavePlaylistOnExit");
    Settings2.bSaveWndSizePos = WA_Ini_Read_Bool(pIni, true, L"Globals", L"SaveWndPosSize");

    if (!WA_Ini_Read_Struct(pIni, &Settings2.MainWindowRect, sizeof(RECT), L"Globals", L"WindowRECT"))
    {
        ZeroMemory(&Settings2.MainWindowRect, sizeof(RECT));
    }

    WA_Ini_Read_String(pIni, Settings2.pActiveOutputDescr, MW_MAX_PLUGIN_DESC, L"", L"Globals", L"OutputPluginDescr");
    WA_Ini_Read_String(pIni, Settings2.pActiveEffectDescr, MW_MAX_PLUGIN_DESC, L"", L"Globals", L"OutputEffectDescr");

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
    WA_Ini_Write_UInt32(pIni, Settings2.uCurrentVolume, L"Globals", L"CurrentVolume");
    WA_Ini_Write_Bool(pIni, Settings2.bSavePlaylistOnExit, L"Globals", L"SavePlaylistOnExit");

    // Save MainWindow Position and Size
    WA_Ini_Write_Struct(pIni, &Settings2.MainWindowRect, sizeof(RECT), L"Globals", L"WindowRECT");
    WA_Ini_Write_Bool(pIni, Settings2.bSaveWndSizePos, L"Globals", L"SaveWndPosSize");

    WA_Ini_Write_String(pIni, Settings2.pActiveOutputDescr, MW_MAX_PLUGIN_DESC, L"Globals", L"OutputPluginDescr");
    WA_Ini_Write_String(pIni, Settings2.pActiveEffectDescr, MW_MAX_PLUGIN_DESC, L"Globals", L"OutputEffectDescr");

    WA_Ini_Delete(pIni);

}



HRESULT STDMETHODCALLTYPE QueryInterface(IDropTarget* This, REFIID riid, void** ppvObject)
{
    return S_OK;
}

ULONG STDMETHODCALLTYPE AddRef(IDropTarget* This)
{
    return InterlockedIncrement(&Globals2.DropTargetReferenceCounter);
}

ULONG STDMETHODCALLTYPE Release(IDropTarget* This)
{
    return InterlockedDecrement(&Globals2.DropTargetReferenceCounter);
}

HRESULT STDMETHODCALLTYPE DragEnter(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    IEnumFORMATETC* pEnumFormat;
    HRESULT hr; 
    FORMATETC Format;

    (*pdwEffect) = DROPEFFECT_NONE;

    hr = IDataObject_EnumFormatEtc(pDataObj, DATADIR_GET, &pEnumFormat);

    if FAILED(hr)
    {
        IDropTargetHelper_DragEnter(Globals2.pDropTargetHelper, Globals2.hMainWindow, pDataObj, (LPPOINT)&pt, (*pdwEffect));
        return E_UNEXPECTED;
    }
   

    Globals2.bAllowFileDrop = false;
    ZeroMemory(&Globals2.DropFormat, sizeof(FORMATETC));

    while (IEnumFORMATETC_Next(pEnumFormat, 1, &Format, NULL) == S_OK)
    {
        if (Format.cfFormat == CF_HDROP)
        {
            Globals2.bAllowFileDrop = true;
            memcpy_s(&Globals2.DropFormat, sizeof(FORMATETC), &Format, sizeof(FORMATETC));
            break; // Exit LOOP
        }
    }

    

    IEnumFORMATETC_Release(pEnumFormat);
 
    if (!Globals2.bAllowFileDrop)
    {
        IDropTargetHelper_DragEnter(Globals2.pDropTargetHelper, Globals2.hMainWindow, pDataObj, (LPPOINT)&pt, (*pdwEffect));
        return S_OK;
    }
        

    (*pdwEffect) = DROPEFFECT_LINK;
    IDropTargetHelper_DragEnter(Globals2.pDropTargetHelper, Globals2.hMainWindow, pDataObj, (LPPOINT)&pt, (*pdwEffect));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DragOver(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    (*pdwEffect) = DROPEFFECT_NONE;

    if (!Globals2.bAllowFileDrop)
    {
        IDropTargetHelper_DragOver(Globals2.pDropTargetHelper, (LPPOINT)&pt, (*pdwEffect));
        return S_OK;
    }
        

    (*pdwEffect) = DROPEFFECT_LINK;
    IDropTargetHelper_DragOver(Globals2.pDropTargetHelper, (LPPOINT)&pt, (*pdwEffect));

    return S_OK;   
}

HRESULT STDMETHODCALLTYPE DragLeave(IDropTarget* This)
{
    Globals2.bAllowFileDrop = false;
    ZeroMemory(&Globals2.DropFormat, sizeof(FORMATETC));

    IDropTargetHelper_DragLeave(Globals2.pDropTargetHelper);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE Drop(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    HRESULT hr;
    STGMEDIUM Stg;
    HDROP pDropFiles;
    UINT uDroppedFiles;
    DWORD dwLastIndex;
    DWORD dwAddedFiles;

    (*pdwEffect) = DROPEFFECT_NONE;    

    if (!Globals2.bAllowFileDrop)
    {
        IDropTargetHelper_Drop(Globals2.pDropTargetHelper, pDataObj, (LPPOINT)&pt, (*pdwEffect));
        return S_OK;
    }
       

    if (!Globals2.pPlaylist)
    {
        IDropTargetHelper_Drop(Globals2.pDropTargetHelper, pDataObj, (LPPOINT)&pt, (*pdwEffect));
        return E_UNEXPECTED;
    }
        

    hr = IDataObject_GetData(pDataObj, &Globals2.DropFormat, &Stg);

    if FAILED(hr)
    {
        IDropTargetHelper_Drop(Globals2.pDropTargetHelper, pDataObj, (LPPOINT)&pt, (*pdwEffect));
        return E_UNEXPECTED;
    }

    dwAddedFiles = 0U;

    if (Stg.tymed == TYMED_HGLOBAL)
    {
        (*pdwEffect) = DROPEFFECT_LINK;

        pDropFiles = (HDROP)Stg.hGlobal;

        // Get the number of dropped files
        // see https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-dragqueryfilea
        uDroppedFiles = DragQueryFile(pDropFiles, 0xFFFFFFFF, NULL, 0);
        

        for (UINT i = 0; i < uDroppedFiles; i++)
        {
            wchar_t FilePath[MAX_PATH];

            if (DragQueryFile(pDropFiles, i, FilePath, MAX_PATH))
            {         
                if (WA_Playback_Engine_IsFileSupported(FilePath))
                {
                    EnterCriticalSection(&Globals2.CacheThreadSection);
                    WA_Playlist_Add(Globals2.pPlaylist, FilePath);
                    LeaveCriticalSection(&Globals2.CacheThreadSection);
                    dwAddedFiles++;
                }                      
            }
        }

    }

    if (dwAddedFiles > 0)
    {
        EnterCriticalSection(&Globals2.CacheThreadSection);
        WA_Playlist_UpdateView(Globals2.pPlaylist, false);
        dwLastIndex = WA_Playlist_Get_Count(Globals2.pPlaylist) - 1U;
        LeaveCriticalSection(&Globals2.CacheThreadSection);

        ListView_EnsureVisible(Globals2.hListView, dwLastIndex, FALSE);
    }
 

    ReleaseStgMedium(&Stg);

    IDropTargetHelper_Drop(Globals2.pDropTargetHelper, pDataObj, (LPPOINT) & pt, (*pdwEffect));

    return S_OK;
}

