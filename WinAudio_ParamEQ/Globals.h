#ifndef GLOBALS_H
#define GLOBALS_H

#define WA_BIQUAD_ARRAY 8 // Same as UI EQ Bands

#define WA_EQ_STD_Q 1.0f
#define WA_EQ_STD_GAIN 0.0f

#define WA_EQ_GAIN_MAX 10
#define WA_EQ_GAIN_MIN -10

#define WA_EQ_LOGIC_GAIN_MIN 0
#define WA_EQ_LOGIC_GAIN_MAX 50
#define WA_EQ_LOGIC_GAIN_MID 25

static const float WA_EQ_Freq_Array[] = { 30.0f, 80.0f, 150.0f, 400.0f, 1000.0f, 2000.0f, 8000.0f, 16000.0f };
static wchar_t *WA_EQ_INI_SECTION = L"WinAudio_ParamEQ\0";

INT_PTR CALLBACK DialogEQProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct
{
	float Gain[WA_BIQUAD_ARRAY];
	float Q[WA_BIQUAD_ARRAY];
	bool bEnableEq;
} UI;


#endif
