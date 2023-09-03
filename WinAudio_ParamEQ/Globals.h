#ifndef GLOBALS_H
#define GLOBALS_H

#define WA_BIQUAD_ARRAY 8 // Same as UI EQ Bands

#define WA_EQ_STD_Q 2.0f
#define WA_EQ_GAIN_MAX 10
#define WA_EQ_GAIN_MIN -10

#define WA_EQ_LOGIC_GAIN_MIN 0
#define WA_EQ_LOGIC_GAIN_MAX 50
#define WA_EQ_LOGIC_GAIN_MID 25

static const float WA_EQ_Freq_Array[] = { 30.0f, 81.0f, 112.0f, 380.0f, 890.0f, 1700.0f, 7600.0f, 16000.0f };

INT_PTR CALLBACK DialogEQProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct
{
	float Gain[WA_BIQUAD_ARRAY];
	float Q[WA_BIQUAD_ARRAY];
	bool bEnableEq;
} UI;


#endif
