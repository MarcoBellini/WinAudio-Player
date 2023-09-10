#include "pch.h"
#include "resource.h"
#include "WA_Biquad.h"
#include "Globals.h"



static void WA_EQ_Gain_To_Trackbar(HWND hTrackbar, float fGain)
{
    float fValue;
    int32_t nPosition;

    // Convert Scales
    fValue = ((float)(fGain - WA_EQ_GAIN_MIN) / (WA_EQ_GAIN_MAX - WA_EQ_GAIN_MIN)) * (WA_EQ_LOGIC_GAIN_MAX - WA_EQ_LOGIC_GAIN_MIN) + WA_EQ_LOGIC_GAIN_MIN;

    // Invert Values
    nPosition = (WA_EQ_LOGIC_GAIN_MAX - (int32_t) fValue);

    SendMessage(hTrackbar, TBM_SETPOS, TRUE, nPosition);
}

static void WA_EQ_PrepareUI(HWND hwndDlg)
{
    wchar_t Buffer[10];

    // Init Trackbars
    for (int32_t i = 0U; i < WA_BIQUAD_ARRAY; i++)
    {
        HWND hTrack = GetDlgItem(hwndDlg, IDC_GAIN1 + i);
        HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT_Q1 + i);

        SendMessage(hTrack, TBM_SETRANGEMIN, FALSE, WA_EQ_LOGIC_GAIN_MIN);
        SendMessage(hTrack, TBM_SETRANGEMAX, FALSE, WA_EQ_LOGIC_GAIN_MAX);

        WA_EQ_Gain_To_Trackbar(hTrack, UI.Gain[i]);

        if (swprintf(Buffer, 10, L"%.1f", UI.Q[i]) > 0)
            SetWindowText(hEdit, Buffer);
        else
            SetWindowText(hEdit, L"1.0");


        CheckDlgButton(hwndDlg, IDC_ENABLE_EQ, UI.bEnableEq ? BST_CHECKED : BST_UNCHECKED);
                

    }

}


static float WA_EQ_Trackbar_To_Gain(HWND hTrackbar)
{
    float fValue;
    int32_t nPosition;

    nPosition = (int32_t) SendMessage(hTrackbar, TBM_GETPOS, 0, 0);

    // Invert Values
    nPosition = (WA_EQ_LOGIC_GAIN_MAX - nPosition);

    // Convert Scales
    fValue = ((float)(nPosition - WA_EQ_LOGIC_GAIN_MIN) / (WA_EQ_LOGIC_GAIN_MAX - WA_EQ_LOGIC_GAIN_MIN)) * (WA_EQ_GAIN_MAX - WA_EQ_GAIN_MIN) + WA_EQ_GAIN_MIN;

    return fValue;
}

static float WA_EQ_WChar_To_Float(HWND hEdit)
{
    wchar_t Buffer[10];
    wchar_t* pEnd;

    float fValue = 1.0f;

    if (GetWindowText(hEdit, Buffer, 10))
    {
        fValue = wcstof(Buffer, &pEnd);
    }

    fValue = max(0.1f, fValue);
    fValue = min(5.0f, fValue);

    return fValue;
}

static void WA_EQ_UpdateGain(UINT Id, HWND hTrackbar)
{
    switch (Id)
    {
    case IDC_GAIN1:
        UI.Gain[0] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN2:
        UI.Gain[1] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN3:
        UI.Gain[2] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN4:
        UI.Gain[3] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN5:
        UI.Gain[4] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN6:
        UI.Gain[5] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN7:
        UI.Gain[6] = WA_EQ_Trackbar_To_Gain(hTrackbar);
        break;
    case IDC_GAIN8:
        UI.Gain[7] = WA_EQ_Trackbar_To_Gain(hTrackbar);

    }

}

static void WA_EQ_UpdateQ(UINT Id, HWND hEdit)
{
    switch (Id)
    {
    case IDC_EDIT_Q1:
        UI.Q[0] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q2:
        UI.Q[1] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q3:
        UI.Q[2] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q4:
        UI.Q[3] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q5:
        UI.Q[4] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q6:
        UI.Q[5] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q7:
        UI.Q[6] = WA_EQ_WChar_To_Float(hEdit);
        break;
    case IDC_EDIT_Q8:
        UI.Q[7] = WA_EQ_WChar_To_Float(hEdit);

    }

}

static BOOL Settings_Handle_WM_Command(HWND hDialog, WORD ControlID, WORD Message, HWND hControl)
{
    switch (Message)
    {

    case EN_UPDATE:
        WA_EQ_UpdateQ(ControlID, hControl);
        return TRUE;
    case BN_CLICKED:
        switch (ControlID)
        {
        case IDOK:
            EndDialog(hDialog, 0);
            return TRUE;
        case IDC_ENABLE_EQ:
            UI.bEnableEq = IsDlgButtonChecked(hDialog, IDC_ENABLE_EQ) ? true : false;
            return TRUE;
        }

    }

    return FALSE;

}


INT_PTR CALLBACK DialogEQProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_INITDIALOG:
    {
        WA_EQ_PrepareUI(hwndDlg);
        return TRUE;
    }
    case WM_NOTIFY:
    {
        NMHDR* pHdr = (NMHDR*)lParam;

#pragma warning(push)
#pragma warning(disable : 26454)
        if (pHdr->code & (UINT) NM_RELEASEDCAPTURE)
        {
            WA_EQ_UpdateGain((UINT) pHdr->idFrom, pHdr->hwndFrom);
            return TRUE;        
        }
#pragma warning(pop)

        break;
    }
    case WM_COMMAND:
        return Settings_Handle_WM_Command(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_DESTROY:
        return TRUE;

    default:
        return FALSE;
    }

    return FALSE;
}