#ifndef WA_UI_LISTVIEW_H
#define WA_UI_LISTVIEW_H


// Subclass Listview Header
LRESULT CALLBACK WA_UI_Listview_Proc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Process WM_NOTIFY Message
LRESULT WA_UI_Listview_OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
