
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



static void WA_SearchDlg_PerformSearch(HWND hListBox, wchar_t* pText, int32_t nTextLen)
{
    DWORD dwCount;
    WA_Playlist_Metadata* pMetadata;
    wchar_t FileName[MAX_PATH];
    wchar_t SearchStr[MAX_PATH];
    wchar_t FileNameListBox[MAX_PATH];

    ListBox_ResetContent(hListBox);

    if (nTextLen < 2)
        return;

    if (!Globals2.pPlaylist)
        return;

    dwCount = WA_Playlist_Get_Count(Globals2.pPlaylist);

    if (dwCount == 0)
        return;    


    wcscpy_s(SearchStr, MAX_PATH, pText);

    // Convert Search string to lowercase
    _wcslwr_s(SearchStr, nTextLen + 1);


    for (DWORD i = 0; i < dwCount; i++)
    {

        pMetadata = WA_Playlist_Get_Item(Globals2.pPlaylist, i);

        // Get File Name and convert to lowercase
        wcscpy_s(FileName, MAX_PATH, PathFindFileName(pMetadata->lpwFilePath));
        _wcslwr_s(FileName,MAX_PATH);

        // Remove Extension from file name
        PathRemoveExtension(FileName);

      // Check if SearchStr is in Filename
        if(wcsstr(FileName, SearchStr))
        {
            int32_t nIndex;

            // Remove Path and Extension and add to the ListBox
            wcscpy_s(FileNameListBox, MAX_PATH, PathFindFileName(pMetadata->lpwFilePath));
            PathRemoveExtension(FileNameListBox);

            nIndex = ListBox_AddString(hListBox, FileNameListBox);


            // Associate Playlist Index to ListBox Item
            ListBox_SetItemData(hListBox, nIndex, i);

        }
    }
}

INT_PTR CALLBACK SearchDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{


    switch (message)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIcon(Globals2.hMainWindowInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);    

        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD Code = HIWORD(wParam);

        
        switch (ID)
        {
        case IDOK:
            if (Code == BN_CLICKED)
            {
                EndDialog(hDlg, 0);
                return TRUE;
            }
            break;
        case IDC_SEARCH_DLG_EDIT:

            if (Code == EN_CHANGE)
            {
                wchar_t Text[MAX_PATH];
                int32_t nTextLen;

                nTextLen = Edit_GetText(GetDlgItem(hDlg, IDC_SEARCH_DLG_EDIT), Text, MAX_PATH);

                WA_SearchDlg_PerformSearch(GetDlgItem(hDlg, IDC_SEARCH_DLG_LIST), Text, nTextLen);

                return TRUE;
            }


            break;
        case IDC_SEARCH_DLG_LIST:
        {
            if (Code == LBN_DBLCLK)
            {           
                int32_t nSelectedItem, nItemIndex;

                nSelectedItem = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_SEARCH_DLG_LIST));

                if (nSelectedItem != LB_ERR)
                {
                    nItemIndex = (int32_t) ListBox_GetItemData(GetDlgItem(hDlg, IDC_SEARCH_DLG_LIST), nSelectedItem);

                    if (nItemIndex != LB_ERR)
                    {                      
                        MainWindow_Open_Playlist_Index((DWORD) nItemIndex);
                        EndDialog(hDlg, 0);

                        return TRUE;
                    }

                }
            }

            break;

        }
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

    return FALSE;
}