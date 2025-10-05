#include "stdafx.h"
#include "WA_DPI.h"

UINT uCurrentDPI = USER_DEFAULT_SCREEN_DPI;

void WA_DPI_Init(HWND hwnd)
{
	uCurrentDPI = GetDpiForWindow(hwnd);
}

void WA_DPI_Destroy()
{
	// No specific cleanup required for DPI settings in this context
	// This function can be used for future cleanup if needed
}

void WA_DPI_AdjustRectForDPI(RECT* rc)
{

}

void WA_DPI_AdjustSizeForDPI(INT* Size)
{
    if(Size == NULL)
		return;

    if (uCurrentDPI == USER_DEFAULT_SCREEN_DPI)
        return;

	*Size = MulDiv(*Size, uCurrentDPI, USER_DEFAULT_SCREEN_DPI);
}

INT WA_DPI_AdjustSizeForDPI2(INT Size)
{
    if (uCurrentDPI == USER_DEFAULT_SCREEN_DPI)
        return Size;

    return MulDiv(Size, uCurrentDPI, USER_DEFAULT_SCREEN_DPI);
}

float WA_DPI_AdjustSizeForDPI2F(float Size)
{
    if (uCurrentDPI == USER_DEFAULT_SCREEN_DPI)
        return Size;

    return Size * uCurrentDPI / USER_DEFAULT_SCREEN_DPI;
}

void WA_DPI_AdjustWindowForDPI(HWND hwnd, INT UnscaledWidth, INT UnscaldedHeight)
{
    RECT rcWindow;

    if (uCurrentDPI == USER_DEFAULT_SCREEN_DPI)
        return;

    if (hwnd == NULL || UnscaledWidth <= 0 || UnscaldedHeight <= 0)
		return;

    if (GetWindowRect(hwnd, &rcWindow) == FALSE)
        return;


    rcWindow.right = rcWindow.left + MulDiv(UnscaledWidth, uCurrentDPI, USER_DEFAULT_SCREEN_DPI);
    rcWindow.bottom = rcWindow.top + MulDiv(UnscaldedHeight, uCurrentDPI, USER_DEFAULT_SCREEN_DPI);

    SetWindowPos(hwnd, NULL, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

INT WA_DPI_GetCurrentDPI()
{
    return uCurrentDPI;
}

void WA_DPI_SetCurrentDPI(INT dpi)
{
    if (dpi <= 0)
        return;

	uCurrentDPI = dpi;
}