
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

static void Settings_UpdateEffect(HWND hCombo)
{
    int nIndex;
    WA_Output* pOut;
    WA_Effect* pEffect;    

    pOut = Globals2.pOutput;

    if (!pOut)
        return;

    nIndex = ComboBox_GetCurSel(hCombo);

    if (nIndex == 0)
    {
        pOut->WA_Output_Enable_Process_DSP(pOut, false);
        pOut->pEffect = NULL;
        Globals2.pEffect = NULL;        
    }
    else
    {
        ComboBox_GetText(hCombo, Settings2.pActiveEffectDescr, MW_MAX_PLUGIN_DESC);

        for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
        {
            if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_DSP)
            {
                pEffect = Plugins.pPluginList[i].hVTable;

                if (wcscmp(Settings2.pActiveEffectDescr, pEffect->Header.lpwDescription) == 0)
                {

                    pOut->WA_Output_Enable_Process_DSP(pOut, false);
                    pOut->pEffect = pEffect;
                    Globals2.pEffect = pEffect;
                    pOut->WA_Output_Enable_Process_DSP(pOut, true);

                    break;
                }
            }
        }

    }
}

static void Settings_Configure_Plugin(HWND hCombo, uint16_t uPluginType)
{
    WA_Output* pOut;
    WA_Effect* pEffect;
    WA_Input* pIn;
    WA_HMODULE hModule;
    WA_PluginHeader* Header;
    wchar_t pPluginName[MW_MAX_PLUGIN_DESC];

    hModule = NULL;

    ComboBox_GetText(hCombo, pPluginName, MW_MAX_PLUGIN_DESC);

    for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == uPluginType)
        {
            Header = Plugins.pPluginList[i].hVTable;

            if (wcscmp(pPluginName, Header->lpwDescription) == 0)
            {
                hModule = Plugins.pPluginList[i].hVTable;         

                break;
            }
        }
    }

    if (!hModule)
    {
        MessageBox(GetParent(hCombo), L"Select a plugin to configure", L"WinAudio", MB_OK | MB_ICONINFORMATION);
        return;
    }


    switch (uPluginType)
    {

        case WA_PLUGINTYPE_INPUT:
            pIn = (WA_Input*)hModule;
            pIn->WA_Input_ConfigDialog(pIn, Globals2.hMainWindow);
            break;
        case WA_PLUGINTYPE_OUTPUT:
            pOut = (WA_Output*)hModule;
            pOut->WA_Output_ConfigDialog(pOut, Globals2.hMainWindow);
            break;
        case WA_PLUGINTYPE_DSP:
            pEffect = (WA_Output*)hModule;
            pEffect->WA_Effect_ConfigDialog(pEffect, Globals2.hMainWindow);
            break;

    }

   
}

static BOOL Settings_Handle_WM_Command(HWND hDialog, WORD ControlID, WORD Message, HWND hControl)
{
    switch (Message)
    {
    case BN_CLICKED:

        switch (ControlID)
        {
        case IDOK:
            EndDialog(hDialog, 0);
            break;
        case IDC_PLAYNEXTFILE:
            Settings2.bPlayNextItem = (bool)IsDlgButtonChecked(hDialog, IDC_PLAYNEXTFILE);
            break;
        case IDC_SAVEPLAYLIST:
            Settings2.bSavePlaylistOnExit = (bool)IsDlgButtonChecked(hDialog, IDC_SAVEPLAYLIST);
            break;
        case IDC_BUTTON_CONF_INPUT:
            Settings_Configure_Plugin(GetDlgItem(hDialog, IDC_INPUT_COMBO), WA_PLUGINTYPE_INPUT);
            break;
        case IDC_BUTTON_CONF_OUTPUT:
            Settings_Configure_Plugin(GetDlgItem(hDialog, IDC_OUTPUT_COMBO), WA_PLUGINTYPE_OUTPUT);
            break;
        case IDC_BUTTON_CONF_EFFECT:
            Settings_Configure_Plugin(GetDlgItem(hDialog, IDC_EFFECT_COMBO), WA_PLUGINTYPE_DSP);
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
            break;
        case IDC_OUTPUT_COMBO:        
            ComboBox_GetText(hControl, Settings2.pActiveOutputDescr, MW_MAX_PLUGIN_DESC);
            break;            
        case IDC_EFFECT_COMBO:        
            Settings_UpdateEffect(hControl);       
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

        wchar_t ColorName[WA_UI_COLORPOLICY_COLOR_NAMES_TEXT_LEN];
        wchar_t PluginName[MW_MAX_PLUGIN_DESC];
        HWND hColorCombo;        
        HWND hPluginCombo;
        WA_PluginHeader* pHeader;

        // Fill Theme Color Section
        hColorCombo = GetDlgItem(hwndDlg, IDC_THEME_COMBO);

        for (uint32_t i = 0; i < WA_UI_COLORPOLICY_COLOR_NAMES_LEN; i++)
        {

            wcscpy_s(ColorName, WA_UI_COLORPOLICY_COLOR_NAMES_TEXT_LEN, WA_UI_ColoPolicy_ColorNamesArr[i]);
            ComboBox_AddString(hColorCombo, ColorName);
        }

        ComboBox_SetCurSel(hColorCombo, (DWORD) Settings2.CurrentTheme);

        // Fill Plugins section
        for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
        {
            pHeader = (WA_PluginHeader*)Plugins.pPluginList[i].hVTable;

            switch (Plugins.pPluginList[i].uPluginType)
            {
            case WA_PLUGINTYPE_OUTPUT:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_OUTPUT_COMBO);   
                wcscpy_s(PluginName, MW_MAX_PLUGIN_DESC, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            case WA_PLUGINTYPE_INPUT:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_INPUT_COMBO);
                wcscpy_s(PluginName, MW_MAX_PLUGIN_DESC, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            case WA_PLUGINTYPE_DSP:
                hPluginCombo = GetDlgItem(hwndDlg, IDC_EFFECT_COMBO);
                wcscpy_s(PluginName, MW_MAX_PLUGIN_DESC, pHeader->lpwDescription);
                ComboBox_AddString(hPluginCombo, PluginName);
                break;
            }
        }

        // Add "None" element on Effects combo
        hPluginCombo = GetDlgItem(hwndDlg, IDC_EFFECT_COMBO);
        ComboBox_AddString(hPluginCombo, L"(None)");
        ComboBox_SetCurSel(hPluginCombo, 0);


        hPluginCombo = GetDlgItem(hwndDlg, IDC_OUTPUT_COMBO);
        ComboBox_SelectString(hPluginCombo, -1, Settings2.pActiveOutputDescr);

        hPluginCombo = GetDlgItem(hwndDlg, IDC_INPUT_COMBO);
        ComboBox_SetCueBannerText(hPluginCombo, L"Select and Configure...");


        hPluginCombo = GetDlgItem(hwndDlg, IDC_EFFECT_COMBO);
        ComboBox_SelectString(hPluginCombo, -1, Settings2.pActiveEffectDescr);


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




