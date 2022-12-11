#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_UI_ListView.h"

LRESULT CALLBACK WA_UI_Listview_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

    switch (uMsg)
    {
    case WM_NOTIFY: // Header custom draw
    {
        // Use Standard Listview if Light Theme is Enabled
        if (!DarkMode_IsEnabled())
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);


        LPNMHDR lpHdr = (LPNMHDR)lParam;

 // Disable warning Arithmetic overflow: '-' operation produces a negative 
 // unsigned result at compile time
#pragma warning( push )
#pragma warning( disable : 26454 )

        if (lpHdr->code & NM_CUSTOMDRAW)
        {
            LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;

            switch (nmcd->dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
            {               
                SetTextColor(nmcd->hdc, ColorPolicy_Get_TextOnBackground_Color());
                return CDRF_DODEFAULT;
            }
            }
        }
#pragma warning( pop )
    }
    case WM_THEMECHANGED:
    {
        HWND hHeader = ListView_GetHeader(hWnd);

        DarkMode_AllowDarkModeForWindow(hWnd, DarkMode_IsEnabled());
        DarkMode_AllowDarkModeForWindow(hHeader, DarkMode_IsEnabled());

        SendMessage(hHeader, WM_THEMECHANGED, wParam, lParam);

        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

        break;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// TODO: Manage Custom Draw Better
LRESULT WA_UI_Listview_CustomDraw(HWND hWnd, LPNMLVCUSTOMDRAW lplvcd)
{
    switch (lplvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
    {
        if ((lplvcd->nmcd.dwItemSpec % 2) == 0)
            lplvcd->clrTextBk = ColorPolicy_Get_Background_Color();
        else
            lplvcd->clrTextBk = ColorPolicy_Get_Primary_Color();

        lplvcd->clrText = ColorPolicy_Get_TextOnSurface_Color();

        return CDRF_NEWFONT;
    }
    default:
        return CDRF_DODEFAULT;
    }
}