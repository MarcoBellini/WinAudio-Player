
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

#include "Globals2.h"


const wchar_t *WA_CLASS_NAME = L"WinAudio_Player";
const wchar_t *WA_APP_NAME = L"WinAudio Player";
const wchar_t *WA_MUTEX_NAME = L"WAMUTEX-A8708208-1B22-484C-BD41-955BFDDA95BF";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND MainWindow_CreateMainWindow(HINSTANCE hInstance);
void MainWindow_StartMainLoop();
void MainWindow_ProcessInParams(int32_t nParams, wchar_t **pArgs);
HWND MainWindow_CreateToolbar(HWND hOwnerHandle);
HMENU MainWindow_CreateMenu(HWND hOwnerHandle);
void MainWindow_DestroyMenu();
HWND MainWindow_CreatePositionTrackbar(HWND hOwnerHandle);
HWND MainWindow_CreateVolumeTrackbar(HWND hOwnerHandle);
HWND MainWindow_CreateSpectrumBar(HWND hOwnerHandle);

HWND MainWindow_CreateStatus(HWND hOwnerHandle);
void MainWindow_Status_SetText(HWND hStatusHandle, wchar_t *pText);
void MainWindow_UpdateStatusText(HWND hStatusHandle,
    uint32_t uSamplerate,
    uint32_t uChannels,
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
bool MainWindow_OpenFileDialog(HWND hOwnerHandle, LPWSTR lpwsPath);

void MainWindow_Play();
void MainWindow_Pause();
void MainWindow_Stop();
bool MainWindow_Open(const wchar_t* wPath);
bool MainWindow_CloseFile();
void MainWindow_SplitFilePath(const wchar_t* pInPath, wchar_t* pFolder, wchar_t* pFileName);
DWORD MainWindow_HandleEndOfStreamMsg();
void MainWindow_UpdatePositionTrackbar(uint64_t uNewValue);
void MainWindow_DrawSpectrum();
void MainWindow_UpdateVolumeFromTrackbarValue();

void MainWindow_UpdateWindowTitle(const wchar_t* pString, bool bClear);


void MainWindow_InitDarkMode(HWND hMainWindow);
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
    HWND hFoundInstanceHandle = NULL;
    HANDLE hMutex = NULL;

    // Before Create new application, find previous opened instance
    hMutex = CreateMutex(NULL, TRUE, WA_MUTEX_NAME);

    if ((hMutex == NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
        hFoundInstanceHandle = FindWindow(WA_CLASS_NAME, NULL);
        

    // If we Found and opened instance, send
    // to them data and close current instance
    if (hFoundInstanceHandle != NULL)
    {
        //TODO: Send Data to Already open instance

        /*
        COPYDATASTRUCT data;
        DWORD dwSize = 0;       

        // Copy data only if we have params
        if (__argc > 1)
        {
            for (int32_t i = 0; i < __argc; i++)
            {
                dwSize += wcslen(__wargv[i]) + 1;
            }

            data.cbData = dwSize * sizeof(wchar_t);
            data.dwData = __argc;
            data.lpData = __wargv[0];

            SendMessage(hFoundInstanceHandle, WM_COPYDATA, 0, (LPARAM)&data);

           
        }        
         */

        // Show Already Opened Window
        SetForegroundWindow(hFoundInstanceHandle);
        ShowWindow(hFoundInstanceHandle, SW_RESTORE);

        _RPTFW0(_CRT_WARN, L"Found and already opened instance\n");

        return EXIT_SUCCESS;
    }


    // Init COM and start application only if we don't found and 
    // opened instance
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if FAILED(hr)
    {
        MessageBox(NULL, L"Cannot Initialize COM", L"Fatal Error", MB_OK);
        return -1;
    }
        

    // Init Common Controls
    INITCOMMONCONTROLSEX commonCtrl;
    commonCtrl.dwSize = sizeof(INITCOMMONCONTROLSEX);
    commonCtrl.dwICC =  ICC_BAR_CLASSES | 
                        ICC_STANDARD_CLASSES | 
                        ICC_COOL_CLASSES | 
                        ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&commonCtrl);

    // TODO: Load Settings


    // Initialize Dark Mode
    DarkMode_Init(true);

    // Initialize ColorPolicy
    if (DarkMode_IsSupported() && DarkMode_IsEnabled())
        ColorPolicy_Init(Dark, Red);
    else
        ColorPolicy_Init(Light, Red);    

        
    // Store Main Window Handle and Instance
    Globals2.hMainWindow = MainWindow_CreateMainWindow(hInstance);
    Globals2.hMainWindowInstance = hInstance;

    // Check if Window has a valid handle
    if (Globals2.hMainWindow == NULL)
        return ERROR_INVALID_HANDLE; // TODO: Clean Resource Before Exit for Fail


    // Load Plugins
    if (!WA_Playback_Engine_New())
    {
        // TODO: Clean Resource on Fail and Exit
        CoUninitialize();
        return EXIT_FAILURE;
    }

    
    
    if (__argc > 1)
    {
        // TODO: Process Pending in Arguments(Open and Play First file)

        /*
        MainWindow_ProcessInParams(__argc, __wargv);

        if (MainWindow_Open(__wargv[1]))
        {
            int32_t uIndex = ListView_GetItemCount(Globals2.hListView) - __argc + 1;

            MainWindow_Play();

            // Select Last Item
            MainWindow_ListView_SetPlayIndex(Globals2.hListView, uIndex);            
       
        }  

         */
    }    
    
    // Show Main Window
    ShowWindow(Globals2.hMainWindow, nShowCmd);
    UpdateWindow(Globals2.hMainWindow);

    // Run the message loop.
    MainWindow_StartMainLoop();      

    // Clean Memory
    DarkMode_Close();
    ColorPolicy_Close();   
    WA_Playback_Engine_Delete();
    CoUninitialize();

    if(hMutex)
        ReleaseMutex(hMutex);

    _CrtDumpMemoryLeaks();

    return EXIT_SUCCESS;
}



/// <summary>
/// Process Params from WinMain or WM_COPYDATA message
/// https://docs.microsoft.com/en-us/cpp/cpp/main-function-command-line-args?view=msvc-160
/// </summary>
void MainWindow_ProcessInParams(int32_t nParams, wchar_t** pArgs)
{
    int32_t i;
    wchar_t FileName[MAX_PATH];
    wchar_t Folder[MAX_PATH];

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
        if (PathFileExists(pArgs[i]) == TRUE)
        {
            MainWindow_SplitFilePath(pArgs[i], Folder, FileName);           

            // Add files to the Playlist
            MainWindow_ListView_InsertRow(Globals2.hListView, FileName, Folder);
          
        }       
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

    if(DarkMode_IsEnabled())
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
		PostQuitMessage(0);

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
                MainWindow_DrawSpectrum();
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
        {
            LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;

            switch (lplvcd->nmcd.hdr.code)
            {
            // Draw a custom Row background of ListView
            case NM_CUSTOMDRAW:
                return WA_UI_Listview_CustomDraw(lpHdr->hwndFrom, lplvcd);
            case NM_DBLCLK:
            {
                LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
                wchar_t wsFolder[MAX_PATH];
                wchar_t wsFileName[MAX_PATH];
                wchar_t wsFilePath[MAX_PATH];

                // Check if we have a valid Item index
                if (lpnmitem->iItem > -1)
                {

                    // Get File Coordinates (File Name + Folder)
                    ListView_GetItemText(lpnmitem->hdr.hwndFrom,
                        lpnmitem->iItem, 1, wsFileName, MAX_PATH);

                    ListView_GetItemText(lpnmitem->hdr.hwndFrom,
                        lpnmitem->iItem, 2, wsFolder, MAX_PATH);

                    // Make a File Path and try to open it
                    if (swprintf_s(wsFilePath, MAX_PATH, L"%s\\%s\0", wsFolder, wsFileName))
                    {
                        if (MainWindow_Open(wsFilePath))
                        {
                            MainWindow_Play();  
                            MainWindow_ListView_SetPlayIndex(Globals2.hListView, lpnmitem->iItem);
                        }
                        else
                        {
                            MessageBox(Globals2.hMainWindow, L"Unable to Open this file", L"Open File Error", MB_ICONERROR | MB_OK);
                        }                       

                }

                }               

                
                break;
            }
               
            }
        }
            break;
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
                    // TODO: Seek
                    // DecoderManager_Seek(Globals2.pDecoderManager, (uint64_t)uPositionValue);

               
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
            // TODO: Update Position
            //MainWindow_UpdatePositionTrackbar(Globals.pDecoderManager->uCurrentPositionMs);            
        case MW_ID_SPECTRUM_TIMER:
            MainWindow_DrawSpectrum();
        }


        return 0;
    }
    case WM_COPYDATA:
    {
        PCOPYDATASTRUCT lpCopy = (PCOPYDATASTRUCT) lParam;

        if (lpCopy->dwData > 1)
        {           
            wchar_t *lpString = lpCopy->lpData;
            wchar_t* Path[MAX_PATH] = { 0 };
            uint32_t uOffset = 0;

            for (uint32_t i = 0; i < lpCopy->dwData; i++)
            {
                Path[i] = lpString + uOffset;

                // Find Null Terminating strings
                while (lpString[uOffset] != L'\0')
                {
                    uOffset++;
                }

                // Skip Null Terminating Character
                uOffset++;
            } 


            // Delete all rows
            MainWindow_ListView_DeleteAllRows(Globals2.hListView);

            // Add new files to listview
            MainWindow_ProcessInParams(lpCopy->dwData, Path);

            // Open and play the first file 
            if (MainWindow_Open(Path[1]))
            {
                // Highlight Played Item (in this case the first item)
                MainWindow_ListView_SetPlayIndex(Globals2.hListView, 0);

                // Play file
                MainWindow_Play();
            }
        }


        return true;
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
                ColorPolicy_Init(Dark, Red); // TODO: Use Color saved in settings
            }
            else
            {
                ColorPolicy_Init(Light, Red); // TODO: Use Color saved in settings
            }
            
            SendMessage(Globals2.hListView, WM_THEMECHANGED, 0, 0);

            ListView_SetBkColor(Globals2.hListView, ColorPolicy_Get_Background_Color());
            ListView_SetTextColor(Globals2.hListView, ColorPolicy_Get_TextOnBackground_Color());
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
        // TODO: Open File Command
        /*
        wchar_t pFilePath[MAX_PATH] = { 0 };
        wchar_t Folder[MAX_PATH];
        wchar_t FileName[MAX_PATH];

        if (MainWindow_OpenFileDialog(Globals.hMainWindowHandle, pFilePath))
        {

            if (MainWindow_Open(pFilePath))
            {

                // Delete all items in playlist and add current opened file
                MainWindow_ListView_DeleteAllRows(Globals.hListViewHandle);

                // Add File into Listview
                MainWindow_SplitFilePath(pFilePath, Folder, FileName);
                MainWindow_ListView_InsertRow(Globals.hListViewHandle, FileName, Folder);

                // Highlight Played Item (in this case the first item)
                MainWindow_ListView_SetPlayIndex(Globals.hListViewHandle, 0);

                // Play file
                MainWindow_Play();
            }
        }
        else
        {
            MessageBox(Globals.hMainWindowHandle, L"Unable to Open this file", L"Open File Error", MB_ICONERROR | MB_OK);
        }

        */

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
    case ID_TOOLS_ENHANCER:
        // TODO: Remove This Dialog
        //DialogBox(Globals.hMainWindowInstance, MAKEINTRESOURCE(IDD_ENHANCER), Globals.hMainWindowHandle, EnhancerProc);
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
void MainWindow_Status_SetText(HWND hStatusHandle, wchar_t* pText)
{
   SendMessage(hStatusHandle, SB_SETTEXT, MAKEWORD(SB_SIMPLEID, 0), (LPARAM)pText);
}

/// <summary>
/// Create ListView control (it fits the space between rebar and status)
/// </summary>
HWND MainWindow_CreateListView(HWND hOwnerHandle)
{
    HWND hListbox;
    HWND hHeader;
    RECT WindowRect;
    RECT RebarRect;
    RECT ListRect;
    RECT StatusRect;

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

    hListbox = CreateWindowEx(
        0,										//	dwExStyle
        WC_LISTVIEW, 							//	lpClassName
        L"Playlist",							//	lpWindowName
        WS_CHILD | WS_VISIBLE |
        WS_VSCROLL | WS_HSCROLL | 
        LVS_REPORT | LVS_SHOWSELALWAYS |
        LVS_SINGLESEL,	//	dwStyle
        ListRect.left,										//	x
        ListRect.top,                                       //	y
        ListRect.right,										//	nWidth
        ListRect.bottom,									//	nHeight
        hOwnerHandle,										//	hWndParent
        (HMENU) MW_ID_LISTVIEW ,							//	hMenu
        GetModuleHandle(NULL),								//	hInstance
        NULL												//	lpParam
    );

    // Set ListView style
    ListView_SetExtendedListViewStyle(hListbox, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_SHOWSELALWAYS);

    // Hide Focus Dots
    SendMessage(hListbox, WM_CHANGEUISTATE, (WPARAM)MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    // Set ListView Background color(TODO: Manage Light Colors)
    ListView_SetBkColor(hListbox, ColorPolicy_Get_Background_Color());
    ListView_SetTextColor(hListbox, ColorPolicy_Get_TextOnBackground_Color());

    SetWindowSubclass(hListbox, WA_UI_Listview_Proc, MW_ID_LISTVIEW, 0);

    hHeader = ListView_GetHeader(hListbox);

    SetWindowTheme(hHeader, L"ItemsView", NULL); // DarkMode
    SetWindowTheme(hListbox, L"ItemsView", NULL); // DarkMode

    return hListbox;

}

/// <summary>
/// Insert a Row in the ListView
/// </summary>
void MainWindow_ListView_InsertRow(HWND hWnd, wchar_t* col1, wchar_t* col2)
{
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
    lvItem.iSubItem = 1;
    lvItem.pszText = col1;
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    ListView_SetItem(hWnd, &lvItem);

    lvItem.iSubItem = 2;
    lvItem.pszText = col2;
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    ListView_SetItem(hWnd, &lvItem);
}

/// <summary>
/// Destroy ListView Control
/// </summary>
void MainWindow_DestroyListView()
{
    RemoveWindowSubclass(Globals2.hListView, WA_UI_Listview_Proc, MW_ID_LISTVIEW);
}

/// <summary>
/// Add Column to ListView
/// </summary>
void MainWindow_ListView_AddColumn(HWND hListboxHandle, wchar_t *pColumnText, int32_t ColumnWidth, int32_t ColumnIndex)
{
    LV_COLUMN columns;

    columns.mask = LVCF_TEXT | LVCF_WIDTH;
    columns.cx = ColumnWidth;
    columns.pszText = pColumnText;
    columns.cchTextMax = wcslen(columns.pszText);
    columns.iSubItem = ColumnIndex;

    ListView_InsertColumn(hListboxHandle, ColumnIndex, &columns);
}

/// <summary>
/// Delete a Column from ListView
/// </summary>
void MainWindow_ListView_DeleteColumn(HWND hListboxHandle, int32_t ColumnIndex)
{
    ListView_DeleteColumn(hListboxHandle, ColumnIndex);
}

/// <summary>
/// Create Listview Column Layouts
/// </summary>
void MainWindow_ListView_InitColumns(HWND hListboxHandle)
{
    MainWindow_ListView_AddColumn(Globals2.hListView, L"#", 20, 0);
    MainWindow_ListView_AddColumn(Globals2.hListView, L"File Name", 350, 1);
    MainWindow_ListView_AddColumn(Globals2.hListView, L"File Path", 250, 2);
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
    ListView_DeleteAllItems(hListboxHandle);
}

/// <summary>
/// Highlight Item in Playing
/// </summary>
void MainWindow_ListView_SetPlayIndex(HWND hListboxHandle, int32_t nIndex)
{
    // Remove Current Item Selection
    if (Globals2.nCurrentPlayingIndex != MW_LW_INVALID_INDEX)
    {
        ListView_SetItemText(Globals2.hListView, Globals2.nCurrentPlayingIndex, 0, L"");
    }

    if (nIndex != MW_LW_INVALID_INDEX)
        ListView_SetItemText(Globals2.hListView, nIndex, 0, L">");    
  
   
    Globals2.nLastPlayedIndex = Globals2.nCurrentPlayingIndex;
    Globals2.nCurrentPlayingIndex = nIndex;
    
}

/// <summary>
/// Create Main Window UI
/// </summary>
/// <param name="hMainWindow">= Parent Window Handle</param>
void MainWindow_CreateUI(HWND hMainWindow)
{
    wchar_t pWelcomeString[MAX_PATH];

    // Switch to Dark Mode (If Enabled)
    if (DarkMode_IsSupported() && DarkMode_IsEnabled())
    {
        DarkMode_AllowDarkModeForWindow(hMainWindow, true);
        DarkMode_RefreshTitleBarThemeColor(hMainWindow);
    }

    // Create main controls
    Globals2.hMainMenu = MainWindow_CreateMenu(hMainWindow);
    Globals2.hStatus = MainWindow_CreateStatus(hMainWindow);
    Globals2.hRebar = MainWindow_CreateRebar(hMainWindow);
    Globals2.hListView = MainWindow_CreateListView(hMainWindow);


    // Create Visualizations resources for Globals.hStaticHandle
   // Globals.bVisualEnabled = vis_Init(&Globals.pVisual, Globals.hStaticHandle, VIS_INSAMPLES);

    // Init Enhancer and create a callback to DSP function 
    // in Enhancer.c



    // Assign Initial Status to Stopped
    Globals2.dwCurrentStatus = MW_STOPPED;
    Globals2.bFileIsOpen = false;
    Globals2.bMouseDownOnPosition = false;
    Globals2.bMouseDownOnVolume = false;
    Globals2.nLastPlayedIndex = MW_LW_INVALID_INDEX;
    Globals2.nCurrentPlayingIndex = MW_LW_INVALID_INDEX;

       
    // Add columns to listbox
    MainWindow_ListView_InitColumns(Globals2.hListView); 


    // Show Welcome Message in status bar
    LoadString(GetModuleHandle(NULL), IDS_STATUS_WELCOME, pWelcomeString, MAX_PATH);
    MainWindow_Status_SetText(Globals2.hStatus, pWelcomeString);
}

/// <summary>
/// Destroy Loaded UI
/// </summary>
void MainWindow_DestroyUI()
{
   
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
        WS_EX_STATICEDGE,                               // no extended styles 
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
/// <param name="lpwsPath">Pointer to a string that contains the path</param>
bool MainWindow_OpenFileDialog(HWND hOwnerHandle, LPWSTR lpwsPath)
{
    // TODO: Create Open File Dialog
    return false;
}

/// <summary>
/// Handle Play Command
/// </summary>
void MainWindow_Play()
{
    // TODO: Delete This Test Function
    if (MainWindow_Open(L"C:\\Users\\Marco\\Desktop\\test.mp3"))
        WA_Playback_Engine_Play();

    switch (Globals2.dwCurrentStatus)
    {
    case WA_STATUS_PAUSE:
        // TODO: Handle Play / Pause
        /*
        if (DecoderManager_UnPause(Globals.pDecoderManager))
        {
            Globals.PlaybackStatus = MW_PLAYING;

            // Highlight Play button
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(TRUE, 0));
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(FALSE, 0));

            // Create Timer for position updates
            SetTimer(Globals.hMainWindowHandle, MW_ID_POS_TIMER, 1000, NULL);

            // Create Timer for Spectrum Analylis
            SetTimer(Globals.hMainWindowHandle, MW_ID_SPECTRUM_TIMER, 40, NULL);

            // Enable Position Trackbar
            EnableWindow(Globals.hPositionTrackbarHandle, true);
        }
        */
        break;
    case WA_STATUS_STOP:
        /*
        if (DecoderManager_Play(Globals.pDecoderManager))
        {
            Globals.PlaybackStatus = MW_PLAYING;

            // Highlight Play button
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(TRUE, 0));
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(FALSE, 0));

            // Create Timer for position updates
            SetTimer(Globals.hMainWindowHandle, MW_ID_POS_TIMER, 1000, NULL);

            // Create Timer for Spectrum Analylis
            SetTimer(Globals.hMainWindowHandle, MW_ID_SPECTRUM_TIMER, 40, NULL);

            // Enable Position Trackbar
            EnableWindow(Globals.hPositionTrackbarHandle, true);
        }
        */
        break;
    }
}


/// <summary>
/// Handle Pause Command
/// </summary>
void MainWindow_Pause()
{
    // TODO: Handle Pause
    /*
    if (Globals.PlaybackStatus == MW_PLAYING)
    {
        if (DecoderManager_Pause(Globals.pDecoderManager))
        {
            Globals.PlaybackStatus = MW_PAUSING;

            // Check Pause Button
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(TRUE, 0));
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(FALSE, 0));

            KillTimer(Globals.hMainWindowHandle, MW_ID_POS_TIMER);
            KillTimer(Globals.hMainWindowHandle, MW_ID_SPECTRUM_TIMER);

            // Disable Position Trackbar
            EnableWindow(Globals.hPositionTrackbarHandle, false);
        }
        
    }
    */
}

/// <summary>
/// Handle Stop Command
/// </summary>
void MainWindow_Stop()
{

    WA_Playback_Engine_Stop();
    // TODO: Handle Stop
    /*
    if (Globals.PlaybackStatus != MW_STOPPED)
    {
        if (DecoderManager_Stop(Globals.pDecoderManager))
        {
            Globals.PlaybackStatus = MW_STOPPED;
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(FALSE, 0));
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(FALSE, 0));
            SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(TRUE, 0));

            KillTimer(Globals.hMainWindowHandle, MW_ID_POS_TIMER);
            KillTimer(Globals.hMainWindowHandle, MW_ID_SPECTRUM_TIMER);

            // Reset Position Trackbar to 0
            MainWindow_UpdatePositionTrackbar(0);

            // Clear Background
            MainWindow_DrawSpectrum();

            // Disable Position Trackbar
            EnableWindow(Globals.hPositionTrackbarHandle, false);
        }
       
    }
    */
}

/// <summary>
/// Open a file to play from a specified Path (not a dialog) 
/// </summary>
/// <param name="lpwPath">Path of file to open</param>
/// <returns>true on success, false otherwise</returns>
bool MainWindow_Open(const wchar_t* lpwPath)
{
    uint64_t uDuration = 0;

    if (Globals2.dwCurrentStatus != MW_STOPPED)
        MainWindow_Stop();

    if (Globals2.bFileIsOpen)
        MainWindow_CloseFile();

    if (!WA_Playback_Engine_OpenFile(lpwPath))
        return false;

    // TODO: If Fail to Get Duration Disable Trackback for this audio stream
    WA_Playback_Engine_Get_Duration(&uDuration);

    // Set Trackbar Range
    SendMessage(Globals2.hPositionTrackbar, TBM_SETRANGEMIN, false, 0);
    SendMessage(Globals2.hPositionTrackbar, TBM_SETRANGEMAX, true, (LPARAM)uDuration);

    // Update Main Window Title with file path
    MainWindow_UpdateWindowTitle(lpwPath, false);

    return true;
}

bool MainWindow_CloseFile()
{
    // TODO: Handle UI States
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
    // Update only if there is no mouse operations in progress
    if (!Globals2.bMouseDownOnPosition)
        SendMessage(Globals2.hPositionTrackbar, TBM_SETPOS, TRUE, (LPARAM)uNewValue);
    /*
    MainWindow_UpdateStatusText(Globals2.hStatus,
        Globals.pDecoderManager->uCurrentSamplerate,
        Globals.pDecoderManager->uCurrentChannels,
        uNewValue);
     */
}

/// <summary>
/// Update Window on End of Stream Event
/// </summary>
DWORD MainWindow_HandleEndOfStreamMsg()
{

    // TODO: Handle End of Stream
    _RPTW0(_CRT_WARN, L"Main Window End Of Stream\n");
    WA_Playback_Engine_Stop();
    /*
    if (Globals.PlaybackStatus != MW_STOPPED)
    {       
        Globals.PlaybackStatus = MW_STOPPED;
        SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PLAY, MAKEWORD(FALSE, 0));
        SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_PAUSE, MAKEWORD(FALSE, 0));
        SendMessage(Globals.hToolbarHandle, TB_CHECKBUTTON, WM_TOOLBAR_STOP, MAKEWORD(TRUE, 0));

        KillTimer(Globals.hMainWindowHandle, MW_ID_POS_TIMER);
        KillTimer(Globals.hMainWindowHandle, MW_ID_SPECTRUM_TIMER);

        // Reset Position Trackbar to 0
        MainWindow_UpdatePositionTrackbar(0);

        // Clear Background
        MainWindow_DrawSpectrum();

        // Disable Position Trackbar
        EnableWindow(Globals.hPositionTrackbarHandle, false);
    }
    */

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
       // _RPTF0(_CRT_WARN, "Position Trackbar: Mouse Down\n");
        break;
    case WM_LBUTTONUP:
        Globals2.bMouseDownOnPosition = false;
        //_RPTF0(_CRT_WARN, "Position Trackbar: Mouse Up\n");
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
        //_RPTF0(_CRT_WARN, "Volume Trackbar: Mouse Down\n");
        break;
    case WM_LBUTTONUP:
        Globals2.bMouseDownOnVolume = false;
       // _RPTF0(_CRT_WARN, "Volume Trackbar: Mouse Up\n");
        break;
    case WM_CAPTURECHANGED:
    {
        //_RPTF0(_CRT_WARN, "Volume Trackbar: Capture Changed\n");    

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
    
  
}


/// <summary>
/// Update Volume From the value in the volume trackbar
/// </summary>
void MainWindow_UpdateVolumeFromTrackbarValue()
{
    DWORD dwVolumeValue;

    dwVolumeValue = SendMessage(Globals2.hVolumeTrackbar, TBM_GETPOS, 0, 0);
    dwVolumeValue = min(dwVolumeValue, UINT8_MAX);      

    //DecoderManager_SetVolume(Globals.pDecoderManager, (uint8_t*)&dwVolumeValue); 

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
    uint64_t uPositionInMs)
{
    wchar_t StatusText[MAX_PATH];
    uint32_t uHour, uMinute, uSeconds;

    uSeconds = (uint32_t) (uPositionInMs / 1000);
    uMinute = uSeconds / 60;
    uHour = uMinute / 60;

    uSeconds = uSeconds % 60;
    uMinute = uMinute % 60;

    ZeroMemory(StatusText, sizeof(StatusText));


    if (swprintf_s(StatusText, 
        MAX_PATH, 
        L"%d Hz | %d Channels | Playback: %02d:%02d:%02d", 
        uSamplerate, 
        uChannels,
        uHour,
        uMinute,
        uSeconds))
    {
        MainWindow_Status_SetText(hStatusHandle, StatusText);
    }
}

void MainWindow_InitDarkMode(HWND hMainWindow)
{


}


void MainWindow_DestroyDarkMode()
{

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
        uint16_t dwVolumeValue;

        dwVolumeValue = (uint16_t) lParam;

        // Skip Invalid Values
        if (dwVolumeValue > MW_VOLUME_MAX)
            return WA_ERROR_BADPARAM;

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
    return TRUE;
}


