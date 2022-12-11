#include "stdafx.h"
#include <vsstyle.h>
#include "WA_UI_UAHMenubar.h"
#include "WA_UI_ColorPolicy.h"



HTHEME g_menuTheme = NULL;


void UAHDrawMenuNCBottomLine(HWND hWnd)
{
    MENUBARINFO mbi = { sizeof(mbi) };
    if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
    {
        return;
    }

    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, NULL, (POINT*)&rcClient, 2);

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);

    OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

    // the rcBar is offset by the window rect
    RECT rcAnnoyingLine = rcClient;
    rcAnnoyingLine.bottom = rcAnnoyingLine.top;
    rcAnnoyingLine.top--;


    HDC hdc = GetWindowDC(hWnd);
    FillRect(hdc, &rcAnnoyingLine, ColorPolicy_Get_Surface_Brush());
    ReleaseDC(hWnd, hdc);
}

// processes messages related to UAH / custom menubar drawing.
// return true if handled, false to continue with normal processing in your wndproc
bool UAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
{
    switch (message)
    {
    case WM_UAHDRAWMENU:
    {
        UAHMENU* pUDM = (UAHMENU*)lParam;
        RECT rc = { 0 };

        // get the menubar rect
        {
            MENUBARINFO mbi = { sizeof(mbi) };
            GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

            RECT rcWindow;
            GetWindowRect(hWnd, &rcWindow);

            // the rcBar is offset by the window rect
            rc = mbi.rcBar;
            OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
        }


        FillRect(pUDM->hdc, &rc, ColorPolicy_Get_Surface_Brush());

        return true;
    }
    case WM_UAHDRAWMENUITEM:
    {
        UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;
        HBRUSH g_brItemBackground = ColorPolicy_Get_Surface_Brush();
        HBRUSH g_brItemBackgroundHot = ColorPolicy_Get_Primary_Brush();
        HBRUSH g_brItemBackgroundSelected = ColorPolicy_Get_Secondary_Brush();

        HBRUSH* pbrBackground = &g_brItemBackground;

        // get the menu item string
        wchar_t menuString[256] = { 0 };
        MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
        {
            mii.dwTypeData = menuString;
            mii.cch = (sizeof(menuString) / 2) - 1;

            GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
        }

        // get the item state for drawing

        DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

        int iTextStateID = 0;
        int iBackgroundStateID = 0;
        {
            if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
                // normal display
                iTextStateID = MPI_NORMAL;
                iBackgroundStateID = MPI_NORMAL;
            }
            if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
                // hot tracking
                iTextStateID = MPI_HOT;
                iBackgroundStateID = MPI_HOT;

                pbrBackground = &g_brItemBackgroundHot;
            }
            if (pUDMI->dis.itemState & ODS_SELECTED) {
                // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                iTextStateID = MPI_HOT;
                iBackgroundStateID = MPI_HOT;

                pbrBackground = &g_brItemBackgroundSelected;
            }
            if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED)) {
                // disabled / grey text
                iTextStateID = MPI_DISABLED;
                iBackgroundStateID = MPI_DISABLED;
            }
            if (pUDMI->dis.itemState & ODS_NOACCEL) {
                dwFlags |= DT_HIDEPREFIX;
            }
        }

        if (!g_menuTheme) {
            g_menuTheme = OpenThemeData(hWnd, L"Menu");
        }

        DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MPI_DISABLED ? ColorPolicy_Get_TextOnSurface_Color() : RGB(0x40, 0x40, 0x40) };

        FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);
        DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);
        return true;
    }
    case WM_UAHMEASUREMENUITEM:
    {
        UAHMEASUREMENUITEM* pMmi = (UAHMEASUREMENUITEM*)lParam;

        // allow the default window procedure to handle the message
        // since we don't really care about changing the width
        *lr = DefWindowProc(hWnd, message, wParam, lParam);

        // but we can modify it here to make it 1/3rd wider for example
        pMmi->mis.itemWidth = (pMmi->mis.itemWidth * 4) / 3;

        return true;
    }
    case WM_THEMECHANGED:
    {
        if (g_menuTheme) {
            CloseThemeData(g_menuTheme);
            g_menuTheme = NULL;
        }
        // continue processing in main wndproc
        return false;
    }
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        *lr = DefWindowProc(hWnd, message, wParam, lParam);
        UAHDrawMenuNCBottomLine(hWnd);
        return true;
        break;
    case WM_NCDESTROY:
        if (g_menuTheme) {
            CloseThemeData(g_menuTheme);
            g_menuTheme = NULL;
        }

        return false;
        break;

    default:
        return false;
    }
}