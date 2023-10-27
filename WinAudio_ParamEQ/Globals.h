#ifndef GLOBALS_H
#define GLOBALS_H

#define WA_BIQUAD_ARRAY 8 // Same as UI EQ Bands

#define WA_EQ_STD_Q 1.0
#define WA_EQ_STD_GAIN 0.0

#define WA_EQ_GAIN_MAX 10
#define WA_EQ_GAIN_MIN -10

#define WA_EQ_LOGIC_GAIN_MIN 0
#define WA_EQ_LOGIC_GAIN_MAX 50
#define WA_EQ_LOGIC_GAIN_MID 25



static const double WA_EQ_Freq_Array[] = { 30.0, 80.0, 150.0, 400.0, 1000.0, 2000.0, 8000.0, 16000.0 };
static wchar_t *WA_EQ_INI_SECTION = L"WinAudio_ParamEQ\0";

INT_PTR CALLBACK DialogEQProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct
{
	double Gain[WA_BIQUAD_ARRAY];
	double Q[WA_BIQUAD_ARRAY];
	bool bEnableEq;
	bool bEnableBoost;
	double fVolumeMult;
} UI;


#endif
