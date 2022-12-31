#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_UI_Rebar.h"
#include "WA_UI_Trackbar.h"



void WA_UI_Rebar_Init(HWND hWnd)
{
    // Nothing to do Here
}

void WA_UI_Rebar_Close(HWND hWnd)
{
    // Nothing to do Here
}

LRESULT CALLBACK WA_UI_Rebar_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    // Use Standard Rebar if Light Theme is Enabled
    if (!DarkMode_IsEnabled())
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);


    switch (uMsg)
    {
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rect;

        // Paint Background
        if (GetClientRect(hWnd, &rect) == TRUE)
            FillRect(hdc, &rect, ColorPolicy_Get_Background_Brush());

        return TRUE;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        LONG WindowStyle;
        BOOL bShowBandBorders;
        UINT uBandCount;
        RECT rcBand, rcIntersect, rcBorder;
        HPEN OldPen;

        WindowStyle = GetWindowLong(hWnd, GWL_STYLE);
        bShowBandBorders = (WindowStyle & RBS_BANDBORDERS) > 0 ? 1 : 0;
        uBandCount = SendMessage(hWnd, RB_GETBANDCOUNT, 0, 0);

        hdc = BeginPaint(hWnd, &ps);

        if (bShowBandBorders)
        {
            OldPen = SelectPen(hdc, ColorPolicy_Get_Surface_Pen());

            for (UINT i = 0; i < uBandCount; i++)
            {
                SendMessage(hWnd, RB_GETBANDBORDERS, (WPARAM)i, (LPARAM)(&rcBorder));
                SendMessage(hWnd, RB_GETRECT, (WPARAM)i, (LPARAM)(&rcBand));

                if (IntersectRect(&rcIntersect, &rcBand, &ps.rcPaint))
                {

                    POINT Vertical[2];

                    LineTo(hdc, rcBand.right, rcBand.top);
                    MoveToEx(hdc, rcBand.left - rcBorder.left, rcBand.bottom, NULL);
                    LineTo(hdc, rcBand.right, rcBand.bottom);
                    MoveToEx(hdc, rcBand.right, rcBand.top, NULL);

                    Vertical[0].x = rcBand.right;
                    Vertical[0].y = rcBand.top;
                    Vertical[1].x = rcBand.right;
                    Vertical[1].y = rcBand.bottom;

                    Polyline(hdc, Vertical, 2);
                }
            }

            SelectPen(hdc, OldPen);
        }


        EndPaint(hWnd, &ps);

        return 0;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}