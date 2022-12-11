#ifndef WA_UI_LISTVIEW_H
#define WA_UI_LISTVIEW_H


LRESULT CALLBACK WA_UI_Listview_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


LRESULT WA_UI_Listview_CustomDraw(HWND hWnd, LPNMLVCUSTOMDRAW lplvcd);

#endif
