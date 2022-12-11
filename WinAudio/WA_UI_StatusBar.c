
#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_UI_StatusBar.h"

#include <Vssym32.h>
#include <Vsstyle.h>


HTHEME hStatusTheme = NULL;

void WA_UI_Status_Init(HWND hWnd)
{
    if (!hStatusTheme)
        hStatusTheme = OpenThemeData(hWnd, L"Status");

}

void WA_UI_Status_Close(HWND hWnd)
{
    if (hStatusTheme)
    {
        CloseThemeData(hStatusTheme);
        hStatusTheme = NULL;
    }
}


/// <summary>
/// Get Max Text Length
/// </summary>
/// <param name="hWnd">Status Handle</param>
/// <param name="dwParts">Total parts of Status</param>
/// <returns>Length</returns>
static inline WORD WA_Status_GetMaxTextLen(HWND hWnd, WORD dwParts)
{
    WORD wMaxTextLen = 0;
    WORD wCurrentTextLen;
    DWORD dwResult;

    for (WORD i = 0; i < dwParts; i++)
    {
        dwResult = SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0);

        wCurrentTextLen = LOWORD(dwResult);
        wMaxTextLen = (wCurrentTextLen > wMaxTextLen) ? wCurrentTextLen : wMaxTextLen;
    }

    // Include Null Termitating Char
    wMaxTextLen++;

    return wMaxTextLen;
}

/// <summary>
/// Draw Themed Status Bar Grip
/// </summary>
/// <param name="hdc">Device Context</param>
/// <param name="hStatusTheme">Valid HTHEME param see OpenThemeData</param>
/// <param name="rcStatus">RECT of Status</param>
static void WA_Status_Draw_StatusGrip(HDC hdc, HTHEME hStatusTheme, LPRECT rcStatus)
{
    RECT GripRect;
    SIZE GripSize;
    HRESULT hr;

    hr = GetThemePartSize(hStatusTheme, hdc, SP_GRIPPER, 0, NULL, TS_DRAW, &GripSize);

    // Skip Grip Drawing on Fail
    if SUCCEEDED(hr)
    {
        GripRect.left = rcStatus->right - GripSize.cx;
        GripRect.right = rcStatus->right;
        GripRect.top = rcStatus->bottom - GripSize.cy;
        GripRect.bottom = rcStatus->bottom;

        DrawThemeBackground(hStatusTheme, hdc, SP_GRIPPER, 0, &GripRect, NULL);
    }
}

/// <summary>
/// Paint "SB_SIMPLE" Status
/// </summary>
static void WA_Status_PaintSimple(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcStatus; 
    WORD wMaxTextLen; 
    LPWSTR lpwStatusText;
    HFONT hOldFont;
    LONG StatusStyle;
    BOOL bStatusHasGrip;

    if (!hStatusTheme)
        WA_UI_Status_Init(hWnd);

    GetClientRect(hWnd, &rcStatus);

    StatusStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    bStatusHasGrip = ((StatusStyle & SBARS_SIZEGRIP) > 0) ? TRUE : FALSE;

    hdc = BeginPaint(hWnd, &ps);

    // Clear Background
    if (ps.fErase == TRUE)
        FillRect(hdc, &rcStatus, ColorPolicy_Get_Surface_Brush());

    // Get Max Length (only 1 memory allocation)
    wMaxTextLen = WA_Status_GetMaxTextLen(hWnd, 1);

    if (wMaxTextLen > 0)    
    {        
        lpwStatusText = (wchar_t*) calloc(wMaxTextLen, sizeof(wchar_t));

        if (lpwStatusText)
        {
            SendMessage(hWnd, SB_GETTEXT, 0, (LPARAM)lpwStatusText);
            
            SetTextColor(hdc, ColorPolicy_Get_TextOnSurface_Color());
            SetBkColor(hdc, ColorPolicy_Get_Surface_Color());
            hOldFont = SelectFont(hdc, ColorPolicy_Get_Default_Font());
            
            DrawText(hdc, lpwStatusText, wMaxTextLen, &rcStatus, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            free(lpwStatusText);
            lpwStatusText = NULL;

            SelectFont(hdc, hOldFont);           
        }

        // Draw Status Size Grip (if enabled)
        if (bStatusHasGrip)        
            WA_Status_Draw_StatusGrip(hdc, hStatusTheme, &rcStatus);        
        
    } 

    EndPaint(hWnd, &ps);
}

/// <summary>
/// Paint non "SB_SIMPLE" Status
/// </summary>
static void WA_Status_PaintComplex(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcStatus, rcPartRect, rcIntersec;
    DWORD dwParts;
    HFONT hOldFont;
    LONG StatusStyle;
    BOOL bStatusHasGrip;
    HPEN hOldPen;    
    WORD wMaxTextLen;
    LPWSTR lpwStatusText;
    POINT Separator[2];

    if (!hStatusTheme)
        WA_UI_Status_Init(hWnd);

    GetClientRect(hWnd, &rcStatus);

    StatusStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    bStatusHasGrip = ((StatusStyle & SBARS_SIZEGRIP) > 0) ? TRUE : FALSE;

    hdc = BeginPaint(hWnd, &ps);

    // Clear Background
    if (ps.fErase == TRUE)
        FillRect(hdc, &rcStatus, ColorPolicy_Get_Surface_Brush());

    dwParts = SendMessage(hWnd, SB_GETPARTS, 0, 0);
    
    SetTextColor(hdc, ColorPolicy_Get_TextOnSurface_Color());
    SetBkColor(hdc, ColorPolicy_Get_Surface_Color());

    hOldPen = SelectPen(hdc, ColorPolicy_Get_Secondary_Pen());
    hOldFont = SelectFont(hdc, ColorPolicy_Get_Default_Font());

    // Get Max Length (only 1 memory allocation)
    wMaxTextLen = WA_Status_GetMaxTextLen(hWnd, (WORD) dwParts);

    if (wMaxTextLen > 0)
    {
        lpwStatusText = (wchar_t*) calloc(wMaxTextLen, sizeof(wchar_t));

        if (lpwStatusText)
        {
            // Draw All Parts
            for (DWORD i = 0; i < dwParts; i++)
            {

                SendMessage(hWnd, SB_GETRECT, i, (LPARAM)&rcPartRect);

                //Draw Only Requested Parts
                if (IntersectRect(&rcIntersec, &ps.rcPaint, &rcPartRect))
                {
             
                    SendMessage(hWnd, SB_GETTEXT, i, (LPARAM)lpwStatusText);

                    DrawText(hdc, lpwStatusText, -1, &rcPartRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

                    // Draw a vertical line between items (Skip Last Item)
                    if (i < (dwParts - 1))
                    {
                        Separator[0].x = rcPartRect.right;
                        Separator[0].y = rcPartRect.top + 2;
                        Separator[1].x = rcPartRect.right;
                        Separator[1].y = rcPartRect.bottom - 3;
                        Polyline(hdc, Separator, 2);
                    }             
                }
            }
            
            free(lpwStatusText);
            lpwStatusText = NULL;

        }
    }

    SelectFont(hdc, hOldFont);
    SelectPen(hdc, hOldPen);

    // Draw Status Size Grip (if enabled)
    if (bStatusHasGrip)
        WA_Status_Draw_StatusGrip(hdc, hStatusTheme, &rcStatus);

    EndPaint(hWnd, &ps);   
}


LRESULT CALLBACK WA_UI_Status_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

    // Use Standard Status if Light Theme is Enabled
    if(!DarkMode_IsEnabled() && (uMsg != WM_THEMECHANGED))
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);


    switch (uMsg)
    {
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rect;

        // Paint Background
        if (GetClientRect(hWnd, &rect) == TRUE)
            FillRect(hdc, &rect, ColorPolicy_Get_Surface_Brush());

        return TRUE;
    }
    case WM_PAINT:
    {
        BOOL IsStatusSimple = SendMessage(hWnd, SB_ISSIMPLE, 0, 0);

        // Paint Status
        if (IsStatusSimple)
            WA_Status_PaintSimple(hWnd);            
        else
            WA_Status_PaintComplex(hWnd);   

        return 0;
    }
    case WM_THEMECHANGED:
    {
        WA_UI_Status_Close(hWnd); // TODO: Fix This. Never called for DarkMode_IsEnabled() check above
        return 0;     
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

}