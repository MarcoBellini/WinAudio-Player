#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_UI_Trackbar.h"

#include <Vssym32.h>
#include <Vsstyle.h>

// Part of this function are taken from:
// https://github.com/wine-mirror/wine/blob/master/dlls/comctl32/trackbar.c
static void WA_UI_Trackbar_DrawThumb(HWND hWnd, HDC hdc, LPRECT lpThumbRect, UINT uItemState, DWORD dwStyle)
{
    POINT ptCursor;
    BOOL bMouseHover = FALSE;
    HBRUSH hCurrentBrush, oldbr;
    POINT points[6];
    int PointDepth;
    HPEN hCurrentPen, hOldPen;
    
    if (!IsWindowEnabled(hWnd))
    {
        hCurrentBrush = ColorPolicy_Get_SecondaryVariant_Brush();
        hCurrentPen = ColorPolicy_Get_SecondaryVariant_Pen();
    }
    else
    {
        // Check if mouse is Down on Thumb 
        if ((uItemState & CDIS_SELECTED) || (uItemState & CDIS_HOT))
        {
            hCurrentBrush = ColorPolicy_Get_Secondary_Brush();
            hCurrentPen = ColorPolicy_Get_Secondary_Pen();
        }
        else
        {
            // Check if Mouse is on Thumb Rect
            if (GetCursorPos(&ptCursor))
                if (ScreenToClient(hWnd, &ptCursor))
                    bMouseHover = PtInRect(lpThumbRect, ptCursor);

            if (bMouseHover)
            {
                hCurrentBrush = ColorPolicy_Get_PrimaryVariant_Brush();
                hCurrentPen = ColorPolicy_Get_PrimaryVariant_Pen();
            }

            else
            {
                hCurrentBrush = ColorPolicy_Get_Primary_Brush();
                hCurrentPen = ColorPolicy_Get_Primary_Pen();
            }
        }
    }



    // from wine project
    if (dwStyle & TBS_BOTH)
    {
        FillRect(hdc, lpThumbRect, hCurrentBrush);
    }
    else
    {
        if (dwStyle & TBS_VERT)
        {
            PointDepth = (lpThumbRect->bottom - lpThumbRect->top) / 2;
            if (dwStyle & TBS_LEFT)
            {
                points[0].x = lpThumbRect->right - 1;
                points[0].y = lpThumbRect->top;
                points[1].x = lpThumbRect->right - 1;
                points[1].y = lpThumbRect->bottom - 1;
                points[2].x = lpThumbRect->left + PointDepth;
                points[2].y = lpThumbRect->bottom - 1;
                points[3].x = lpThumbRect->left;
                points[3].y = lpThumbRect->top + PointDepth;
                points[4].x = lpThumbRect->left + PointDepth;
                points[4].y = lpThumbRect->top;
                points[5].x = points[0].x;
                points[5].y = points[0].y;
            }
            else
            {
                points[0].x = lpThumbRect->right;
                points[0].y = lpThumbRect->top + PointDepth;
                points[1].x = lpThumbRect->right - PointDepth;
                points[1].y = lpThumbRect->bottom - 1;
                points[2].x = lpThumbRect->left;
                points[2].y = lpThumbRect->bottom - 1;
                points[3].x = lpThumbRect->left;
                points[3].y = lpThumbRect->top;
                points[4].x = lpThumbRect->right - PointDepth;
                points[4].y = lpThumbRect->top;
                points[5].x = points[0].x;
                points[5].y = points[0].y;
            }
        }
        else
        {
            PointDepth = (lpThumbRect->right - lpThumbRect->left) / 2;
            if (dwStyle & TBS_TOP)
            {
                points[0].x = lpThumbRect->left + PointDepth;
                points[0].y = lpThumbRect->top + 1;
                points[1].x = lpThumbRect->right - 1;
                points[1].y = lpThumbRect->top + PointDepth + 1;
                points[2].x = lpThumbRect->right - 1;
                points[2].y = lpThumbRect->bottom - 1;
                points[3].x = lpThumbRect->left;
                points[3].y = lpThumbRect->bottom - 1;
                points[4].x = lpThumbRect->left;
                points[4].y = lpThumbRect->top + PointDepth + 1;
                points[5].x = points[0].x;
                points[5].y = points[0].y;
            }
            else
            {
                points[0].x = lpThumbRect->right - 1;
                points[0].y = lpThumbRect->top;
                points[1].x = lpThumbRect->right - 1;
                points[1].y = lpThumbRect->bottom - PointDepth - 1;
                points[2].x = lpThumbRect->left + PointDepth;
                points[2].y = lpThumbRect->bottom - 1;
                points[3].x = lpThumbRect->left;
                points[3].y = lpThumbRect->bottom - PointDepth - 1;
                points[4].x = lpThumbRect->left;
                points[4].y = lpThumbRect->top;
                points[5].x = points[0].x;
                points[5].y = points[0].y;
            }
        }

        hOldPen = SelectPen(hdc, hCurrentPen);
        oldbr = SelectBrush(hdc, hCurrentBrush);
        SetPolyFillMode(hdc, WINDING);
        Polygon(hdc, points, 6);
        SelectBrush(hdc, oldbr);
        SelectBrush(hdc, hOldPen);
    }

}

LRESULT WA_UI_Trackbar_CustomDraw(HWND hWnd, LPNMCUSTOMDRAW lpCustomDraw)
{
    if (!DarkMode_IsEnabled())
        return CDRF_DODEFAULT;

    DWORD dwItemSpec = (DWORD) lpCustomDraw->dwItemSpec;
    DWORD dwDrawStage = lpCustomDraw->dwDrawStage;
    UINT uItemState = lpCustomDraw->uItemState;      


    switch (dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
    {

        switch (dwItemSpec)
        {
        case TBCD_CHANNEL:
        {
            FillRect(lpCustomDraw->hdc, &lpCustomDraw->rc, ColorPolicy_Get_Surface_Brush());
            return CDRF_SKIPDEFAULT;
        }
        case TBCD_THUMB:
        {
            DWORD dwStyle = (DWORD) GetWindowLongPtr(hWnd, GWL_STYLE);

            WA_UI_Trackbar_DrawThumb(hWnd, lpCustomDraw->hdc, &lpCustomDraw->rc, uItemState, dwStyle);  
            
            return CDRF_SKIPDEFAULT;
        }
        case TBCD_TICS:
        {
            return CDRF_DODEFAULT;
        }
        }

    }
    }

   

    return CDRF_DODEFAULT;
}
