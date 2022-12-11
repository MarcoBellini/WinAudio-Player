
#include "stdafx.h"
#include "globals.h"
#include "biquad.h"

struct
{
    uint32_t uSamplerate;
    Biquad* LowShelf;
    Biquad* HighShelf;
    float fPeakValue;
    float fGainValue;
} Enhancer;


BOOL CALLBACK EnhancerProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {      

        return TRUE;
    }

    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_DESTROY:
 ;
        return TRUE;
    case WM_COMMAND:
    {

    }
    
        

    default:
        return FALSE;
    }
}

// Update Samplerate


void Enhancer_Init()
{

}

void Enhancer_Destroy()
{

}


void Enhancer_Update(uint32_t uSamplerate)
{

}