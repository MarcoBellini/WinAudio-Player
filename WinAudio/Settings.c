
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
#include "WA_GEN_INI.h"
#include "WA_UI_Visualizations.h"
#include "globals2.h"


static void Settings_UpdateColors(HWND hCombobox)
{
    ColorPolicy_Close();

    Settings2.CurrentTheme = ComboBox_GetCurSel(hCombobox);

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

    RedrawWindow(Globals2.hMainWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

     
}

static BOOL Settings_Handle_WM_Command(HWND hDialog, WORD ControlID, WORD Message, HWND hControl)
{
    switch (Message)
    {
    case BN_CLICKED:

        switch (ControlID)
        {
        case IDOK:
            EndDialog(GetParent(hControl), 0);
            break;
        case IDC_PLAYNEXTFILE:
            Settings2.bPlayNextItem = (bool)IsDlgButtonChecked(hDialog, IDC_PLAYNEXTFILE);
            break;
        case IDC_SAVEPLAYLIST:
            Settings2.bSavePlaylistOnExit = (bool)IsDlgButtonChecked(hDialog, IDC_SAVEPLAYLIST);
            break;
        }

        break;
    case CBN_SELCHANGE:

        switch (ControlID)
        {
        case IDC_THEME_COMBO:
            Settings_UpdateColors(hControl);
            break;
        case IDC_INPUT_COMBO:
        case IDC_OUTPUT_COMBO:
        case IDC_EFFECT_COMBO:
            break;
        }

        break;

    }

    return FALSE;
}



INT_PTR CALLBACK SettingsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {      
        // TODO: Continua qui 03/05/23, eseguire refactoring
        // https://learn.microsoft.com/en-us/windows/win32/controls/create-a-simple-combo-box
        wchar_t ColorName[10];
        HWND hColorCombo;

        hColorCombo = GetDlgItem(hwndDlg, IDC_THEME_COMBO);

        for (uint32_t i = 0; i < 6; i++)
        {

            wcscpy_s(ColorName, 10, ColorNames[i]);

            ComboBox_AddString(hColorCombo, ColorName);
        }

        ComboBox_SetCurSel(hColorCombo, (DWORD) Settings2.CurrentTheme);

        wchar_t PluginName[100];
        HWND hPluginCombo;
        WA_PluginHeader* pHeader;

        for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
        {
            pHeader = (WA_PluginHeader*)Plugins.pPluginList[i].hVTable;

            switch (Plugins.pPluginList[i].uPluginType)
            {
            case WA_PLUGINTYPE_OUTPUT:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_OUTPUT_COMBO);               

                wcscpy_s(PluginName, 100, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            case WA_PLUGINTYPE_INPUT:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_INPUT_COMBO);
                wcscpy_s(PluginName, 100, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            case WA_PLUGINTYPE_DSP:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_EFFECT_COMBO);
                wcscpy_s(PluginName, 100, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            }
        }

        hPluginCombo = GetDlgItem(hwndDlg, IDC_INPUT_COMBO);
        ComboBox_SetCueBannerText(hPluginCombo, L"Select and Configure...");



        hPluginCombo = GetDlgItem(hwndDlg, IDC_EFFECT_COMBO);
        ComboBox_SetCueBannerText(hPluginCombo, L"None");


        if (Settings2.CurrentMode == Light)       
            SetWindowText(GetDlgItem(hwndDlg, IDC_COLOR_MODE), L"Light");
        else
            SetWindowText(GetDlgItem(hwndDlg, IDC_COLOR_MODE), L"Dark");        


        CheckDlgButton(hwndDlg, IDC_PLAYNEXTFILE, Settings2.bPlayNextItem ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_SAVEPLAYLIST, Settings2.bSavePlaylistOnExit ? BST_CHECKED : BST_UNCHECKED);
     

        return TRUE;
    }
    case WM_COMMAND:
        return Settings_Handle_WM_Command(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND) lParam);
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_DESTROY:        
        return TRUE;      

    default:
        return FALSE;
    }


}




