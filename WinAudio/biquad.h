
// A fork of https://github.com/hosackm/BiquadFilter

#ifndef BIQUAD_H
#define BIQUAD_H


#ifndef PI
#define PI 3.141592653589793238462f
#endif

#ifndef LN2
#define LN2 0.69314718055994530942f
#endif


typedef enum tagBIQUAD_FILTER {
	LOWPASS = 0,
	HIGHPASS = 1,
	BANDPASS = 2,
	NOTCH = 3,
	PEAK = 4,
	LOWSHELF = 5,
	HIGHSHELF = 6,
} BIQUAD_FILTER;

typedef struct tagBiquad
{
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float prev_input_1;
	float prev_input_2;
	float prev_output_1;
	float prev_output_2;
} Biquad;

/// <summary>
/// Create new instance of Biquad Filter
/// http://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
/// </summary>
Biquad* biquad_new();

void biquad_update(Biquad* pHandle, BIQUAD_FILTER type, float frequency, float q,
	float dbGain,
	uint32_t sample_rate);


void biquad_process(Biquad* pHandle, float* pBuffer, uint32_t uCount);


void biquad_delete(Biquad* pHandle);

#endif
