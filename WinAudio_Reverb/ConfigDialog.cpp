#include "pch.h"
#include "RevModel.h"
#include "WA_Reverb.h"
#include "Globals.h"
#include "resource.h"

WA_Reverb* pInstance = nullptr;


static BOOL Settings_Handle_WM_Command(HWND hDialog, WORD ControlID, WORD Message, HWND hControl)
{
    switch (Message)
    {
    case BN_CLICKED:
        switch (ControlID)
        {
        case IDOK:
            EndDialog(hDialog, 0);
            return TRUE;
        }
    }

    return FALSE;

}

INT_PTR CALLBACK ConfigDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_INITDIALOG:
    {
        if (!lParam)
            EndDialog(hwndDlg, 0);

        pInstance = (WA_Reverb*)lParam;


        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WET), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WET), TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WET), TBM_SETPOS, TRUE, static_cast<int>(pInstance->GetWet() * 100));

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DRY), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DRY), TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DRY), TBM_SETPOS, TRUE, static_cast<int>(pInstance->GetDry() * 100));

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DAMP), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DAMP), TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_DAMP), TBM_SETPOS, TRUE, static_cast<int>(pInstance->GetDampness() * 100));

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WIDTH), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WIDTH), TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_WIDTH), TBM_SETPOS, TRUE, static_cast<int>(pInstance->GetRoomWidth() * 100));

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SIZE), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SIZE), TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SIZE), TBM_SETPOS, TRUE, static_cast<int>(pInstance->GetRoomSize() * 100));

       
        return TRUE;
    }
    case WM_NOTIFY:
    {
        NMHDR* pHdr = (NMHDR*)lParam;

        if (!pInstance)
            return FALSE;

#pragma warning(push)
#pragma warning(disable : 26454)
        if (pHdr->code & (UINT)NM_RELEASEDCAPTURE)
        {

            int pos = static_cast<int>(SendMessage(pHdr->hwndFrom, TBM_GETPOS, 0, 0));

            switch (pHdr->idFrom)
            {
            case IDC_SLIDER_WET:
                pInstance->SetWet(pos / 100.0f);
                break;
            case IDC_SLIDER_DRY:
                pInstance->SetDry(pos / 100.0f);
                break;
            case IDC_SLIDER_DAMP:
                pInstance->SetDampness(pos / 100.0f);
                break;
            case IDC_SLIDER_WIDTH:
                pInstance->SetRoomWidth(pos / 100.0f);
                break;
            case IDC_SLIDER_SIZE:
                pInstance->SetRoomSize(pos / 100.0f);
                break;

            }   
            return TRUE;
       }
#pragma warning(pop)

        break;
    }
    case WM_COMMAND:
        return Settings_Handle_WM_Command(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);      
    case WM_CLOSE:
        pInstance = nullptr;
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_DESTROY:
        return TRUE;
    default:
        return FALSE;
    }

    return FALSE;
}