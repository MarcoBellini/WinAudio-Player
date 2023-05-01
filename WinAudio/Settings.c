
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
#include "globals2.h"




BOOL CALLBACK SettingsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {   
        /*
        if (DarkMode_IsSupported() &&
            DarkMode_IsEnabled())
        {
            DarkMode_AllowDarkModeForWindow(hwndDlg, true);
            DarkMode_RefreshTitleBarThemeColor(hwndDlg);
            DarkMode_ApplyMica(hwndDlg);
        }
        */
        // TODO: Continua qui 01/05/23
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


        return TRUE;
    }

    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_DESTROY:
 ;
        return TRUE;
    case WM_COMMAND:
    {

    }
    
        

    default:
        return FALSE;
    }


}




