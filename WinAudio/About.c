
#include "stdafx.h"
#include "resource.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_Playlist.h"
#include "WA_UI_Visualizations.h"
#include "Globals2.h"

#define ABOUT_BUILD_STR_LEN 50

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{


    switch (message)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIcon(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        wchar_t BuildUnicode[ABOUT_BUILD_STR_LEN];
        char BuildAnsi[ABOUT_BUILD_STR_LEN];

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        SendDlgItemMessage(hDlg, IDC_LOGO_CTRL, STM_SETICON, (WPARAM)hIcon, NULL);

        // Convert ANSI macro to UNICODE
        sprintf_s(BuildAnsi, ABOUT_BUILD_STR_LEN, "%u / %s %s\0", MW_ID_BUILD_NR, __TIME__, __DATE__);
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, BuildAnsi, -1, BuildUnicode, ABOUT_BUILD_STR_LEN);             

        SetDlgItemText(hDlg, IDC_BUILD_STATIC, BuildUnicode);


#if  _WIN64
        SetDlgItemText(hDlg, IDC_PLATFORM_STATIC, L"x64");

#else
        SetDlgItemText(hDlg, IDC_PLATFORM_STATIC, L"x86");
#endif

     

        return TRUE;
    }
    case WM_COMMAND:

        if (wParam == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;            
        }

        return FALSE;
    case WM_NOTIFY:
    {
        LPNMHDR pHdr = (LPNMHDR)lParam;
        

        if ((pHdr->code == NM_CLICK) && (pHdr->idFrom = IDC_GITHUB_LINK))
        {

            PNMLINK pnmLink = (PNMLINK)lParam;

            ShellExecute(NULL, L"open", pnmLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
                      

            return TRUE;
        }

        return FALSE;
    }
    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return TRUE;
    case WM_DESTROY:
        return TRUE;

    default:
        return FALSE;
    }


}