#ifndef WA_DPI_H
#define WA_DPI_H


void WA_DPI_Init(HWND hwnd);
void WA_DPI_Destroy();

INT WA_DPI_ScaleInt(INT Size); 
float WA_DPI_ScaleFloat(float Size);
void WA_DPI_ScaleWindow(HWND hwnd, INT UnscaledWidth, INT UnscaledHeight);
INT WA_DPI_GetCurrentDPI();
void WA_DPI_SetCurrentDPI(INT dpi);


#endif