#ifndef WA_DPI_H
#define WA_DPI_H


void WA_DPI_Init(HWND hwnd);
void WA_DPI_Destroy();

void WA_DPI_AdjustRectForDPI(RECT* rc);
void WA_DPI_AdjustSizeForDPI(INT* Size);
INT WA_DPI_AdjustSizeForDPI2(INT Size); 
float WA_DPI_AdjustSizeForDPI2F(float Size);
void WA_DPI_AdjustWindowForDPI(HWND hwnd, INT UnscaledWidth, INT UnscaldedHeight);
INT WA_DPI_GetCurrentDPI();
void WA_DPI_SetCurrentDPI(INT dpi);


#endif