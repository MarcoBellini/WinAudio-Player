// A fork of https://github.com/hosackm/BiquadFilter

#ifndef BIQUAD_H
#define BIQUAD_H


typedef enum tagBIQUAD_FILTER {
	LOWPASS = 0,
	HIGHPASS = 1,
	BANDPASS = 2,
	NOTCH = 3,
	PEAK = 4,
	LOWSHELF = 5,
	HIGHSHELF = 6,
} BIQUAD_FILTER;

struct TagWA_Biquad;
typedef struct TagWA_Biquad WA_Biquad;


WA_Biquad* WA_Biquad_New();
void WA_Biquad_Delete(WA_Biquad* This);

void WA_Biquad_Update(WA_Biquad* This, BIQUAD_FILTER type, double frequency, double q,
	double dbGain,
					uint32_t sample_rate);


void WA_Biquad_Process(WA_Biquad* This, double* pBuffer, uint32_t uCount, uint32_t uChannels);




#endif
