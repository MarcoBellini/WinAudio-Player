// A fork of https://github.com/hosackm/BiquadFilter

#include "pch.h"
#include "WA_Biquad.h"


#ifndef PI
#define PI 3.141592653589793238462f
#endif

#ifndef LN2
#define LN2 0.69314718055994530942f
#endif

#define WA_BIQUAD_MAX_CHANNELS 7

struct TagWA_Biquad
{
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float prev_input_1[WA_BIQUAD_MAX_CHANNELS];
	float prev_input_2[WA_BIQUAD_MAX_CHANNELS];
	float prev_output_1[WA_BIQUAD_MAX_CHANNELS];
	float prev_output_2[WA_BIQUAD_MAX_CHANNELS];
};

WA_Biquad* WA_Biquad_New()
{
	WA_Biquad* pHandle = NULL;	

	pHandle = (WA_Biquad*) malloc(sizeof(WA_Biquad));

	if (!pHandle)	
		return NULL;	
	

	return pHandle;
}

void WA_Biquad_Delete(WA_Biquad* This)
{
	if (This)
	{
		free(This);
		This = NULL;
	}
}

void WA_Biquad_Update(WA_Biquad* This, BIQUAD_FILTER type, float frequency, float q,
	float dbGain,
	uint32_t sample_rate)
{
	float A, omega, sn, cs, alpha, beta;

	A = powf(10.0f, dbGain / 40.0f); //convert to db
	omega = 2.0f * PI * frequency / (float)sample_rate;
	sn = sinf(omega);
	cs = cosf(omega);
	alpha = sn / (2.0f * q); // sn * sinhf(LN2 / 2.0f * q * omega / sn);
	beta = 2 * sqrtf(A);

	switch (type)
	{
	case LOWPASS:
		This->b0 = (1.0f - cs) / 2.0f;
		This->b1 = 1.0f - cs;
		This->b2 = (1.0f - cs) / 2.0f;
		This->a0 = 1.0f + alpha;
		This->a1 = -2.0f * cs;
		This->a2 = 1.0f - alpha;
		break;
	case HIGHPASS:
		This->b0 = (1.0f + cs) / 2.0f;
		This->b1 = -(1.0f + cs);
		This->b2 = (1.0f + cs) / 2.0f;
		This->a0 = 1.0f + alpha;
		This->a1 = -2.0f * cs;
		This->a2 = 1.0f - alpha;
		break;
	case BANDPASS:
		This->b0 = alpha;
		This->b1 = 0.0f;
		This->b2 = -alpha;
		This->a0 = 1.0f + alpha;
		This->a1 = -2.0f * cs;
		This->a2 = 1.0f - alpha;
		break;
	case NOTCH:
		This->b0 = 1.0f;
		This->b1 = -2.0f * cs;
		This->b2 = 1.0f;
		This->a0 = 1.0f + alpha;
		This->a1 = -2.0f * cs;
		This->a2 = 1.0f - alpha;
		break;
	case PEAK:
		This->b0 = 1.0f + (alpha * A);
		This->b1 = -2.0f * cs;
		This->b2 = 1.0f - (alpha * A);
		This->a0 = 1.0f + (alpha / A);
		This->a1 = -2.0f * cs;
		This->a2 = 1.0f - (alpha / A);
		break;
	case LOWSHELF:
		This->b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + beta * sn);
		This->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
		This->b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - beta * sn);
		This->a0 = (A + 1.0f) + (A - 1.0f) * cs + beta * sn;
		This->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
		This->a2 = (A + 1.0f) + (A - 1.0f) * cs - beta * sn;
		break;
	case HIGHSHELF:
		This->b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + beta * sn);
		This->b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
		This->b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - beta * sn);
		This->a0 = (A + 1.0f) - (A - 1.0f) * cs + beta * sn;
		This->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
		This->a2 = (A + 1.0f) - (A - 1.0f) * cs - beta * sn;
		break;
	}


	This->a1 /= (This->a0);
	This->a2 /= (This->a0);
	This->b0 /= (This->a0);
	This->b1 /= (This->a0);
	This->b2 /= (This->a0);

	ZeroMemory(This->prev_input_1, sizeof(This->prev_input_1));
	ZeroMemory(This->prev_input_2, sizeof(This->prev_input_2));
	ZeroMemory(This->prev_output_1, sizeof(This->prev_output_1));
	ZeroMemory(This->prev_output_2, sizeof(This->prev_output_2));	
}


void WA_Biquad_Process(WA_Biquad* This, float* pBuffer, uint32_t uCount, uint32_t uChannels)
{

	float fInValue;

	_ASSERT(uCount > 0);
	_ASSERT(This != NULL);
	_ASSERT(pBuffer != NULL);

	if (uChannels > WA_BIQUAD_MAX_CHANNELS)
		return;

	
	for (uint32_t cn = 0U; cn < uChannels; cn++)
	{	

		for (uint32_t i = cn; i < uCount; i+= uChannels)
		{
			fInValue = pBuffer[i];

			pBuffer[i] = (This->b0 * fInValue) +
				(This->b1 * This->prev_input_1[cn]) +
				(This->b2 * This->prev_input_2[cn]) -
				(This->a1 * This->prev_output_1[cn]) -
				(This->a2 * This->prev_output_2[cn]);

			This->prev_input_2[cn] = This->prev_input_1[cn];
			This->prev_input_1[cn] = fInValue;
			This->prev_output_2[cn] = This->prev_output_1[cn];
			This->prev_output_1[cn] = pBuffer[i];	
		}
	}

}


