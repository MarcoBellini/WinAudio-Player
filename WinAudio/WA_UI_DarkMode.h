#ifndef WA_UI_DARKMODE_H
#define WA_UI_DARKMODE_H

void DarkMode_Init(bool bFixScrollbars);
void DarkMode_Close();
bool DarkMode_IsSupported();
bool DarkMode_IsEnabled();
void DarkMode_AllowDarkModeForApp(bool bAllow);
bool DarkMode_IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void DarkMode_RefreshTitleBarThemeColor(HWND hWnd);
bool DarkMode_AllowDarkModeForWindow(HWND hWnd, bool bAllow);
bool DarkMode_ApplyMica(HWND hWnd);
bool DarkMode_IsHighContrast();
void DarkMode_HandleThemeChange();

#endif