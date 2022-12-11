#ifndef WA_UI_REBAR_H
#define WA_UI_REBAR_H

void WA_UI_Rebar_Init(HWND hWnd);
void WA_UI_Rebar_Close(HWND hWnd);

LRESULT CALLBACK WA_UI_Rebar_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#endif
