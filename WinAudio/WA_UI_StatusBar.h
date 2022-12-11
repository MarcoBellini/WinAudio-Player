
#ifndef WA_UI_STATUSBAR
#define WA_UI_STATUSBAR

void WA_UI_Status_Init(HWND hWnd);
void WA_UI_Status_Close(HWND hWnd);

LRESULT CALLBACK WA_UI_Status_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#endif
