
#include "stdafx.h"
#include "biquad.h"



Biquad* biquad_new()
{
	Biquad* pHandle = NULL;	

	pHandle = (Biquad*)malloc(sizeof(Biquad));

	if (!pHandle)	
		return NULL;	
	

	return pHandle;
}

void biquad_update(Biquad* pHandle, BIQUAD_FILTER type, float frequency, float q,
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
		pHandle->b0 = (1.0f - cs) / 2.0f;
		pHandle->b1 = 1.0f - cs;
		pHandle->b2 = (1.0f - cs) / 2.0f;
		pHandle->a0 = 1.0f + alpha;
		pHandle->a1 = -2.0f * cs;
		pHandle->a2 = 1.0f - alpha;
		break;
	case HIGHPASS:
		pHandle->b0 = (1.0f + cs) / 2.0f;
		pHandle->b1 = -(1.0f + cs);
		pHandle->b2 = (1.0f + cs) / 2.0f;
		pHandle->a0 = 1.0f + alpha;
		pHandle->a1 = -2.0f * cs;
		pHandle->a2 = 1.0f - alpha;
		break;
	case BANDPASS:
		pHandle->b0 = alpha;
		pHandle->b1 = 0.0f;
		pHandle->b2 = -alpha;
		pHandle->a0 = 1.0f + alpha;
		pHandle->a1 = -2.0f * cs;
		pHandle->a2 = 1.0f - alpha;
		break;
	case NOTCH:
		pHandle->b0 = 1.0f;
		pHandle->b1 = -2.0f * cs;
		pHandle->b2 = 1.0f;
		pHandle->a0 = 1.0f + alpha;
		pHandle->a1 = -2.0f * cs;
		pHandle->a2 = 1.0f - alpha;
		break;
	case PEAK:
		pHandle->b0 = 1.0f + (alpha * A);
		pHandle->b1 = -2.0f * cs;
		pHandle->b2 = 1.0f - (alpha * A);
		pHandle->a0 = 1.0f + (alpha / A);
		pHandle->a1 = -2.0f * cs;
		pHandle->a2 = 1.0f - (alpha / A);
		break;
	case LOWSHELF:
		pHandle->b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + beta * sn);
		pHandle->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
		pHandle->b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - beta * sn);
		pHandle->a0 = (A + 1.0f) + (A - 1.0f) * cs + beta * sn;
		pHandle->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
		pHandle->a2 = (A + 1.0f) + (A - 1.0f) * cs - beta * sn;
		break;
	case HIGHSHELF:
		pHandle->b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + beta * sn);
		pHandle->b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
		pHandle->b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - beta * sn);
		pHandle->a0 = (A + 1.0f) - (A - 1.0f) * cs + beta * sn;
		pHandle->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
		pHandle->a2 = (A + 1.0f) - (A - 1.0f) * cs - beta * sn;
		break;
	}


	pHandle->a1 /= (pHandle->a0);
	pHandle->a2 /= (pHandle->a0);
	pHandle->b0 /= (pHandle->a0);
	pHandle->b1 /= (pHandle->a0);
	pHandle->b2 /= (pHandle->a0);


	pHandle->prev_input_1 = 0.0f;
	pHandle->prev_input_2 = 0.0f;
	pHandle->prev_output_1 = 0.0f;
	pHandle->prev_output_2 = 0.0f;
}


void biquad_process(Biquad* pHandle, float* pBuffer, uint32_t uCount)
{

	float fInValue;


	_ASSERT(uCount > 0);
	_ASSERT(pHandle != NULL);
	_ASSERT(pBuffer != NULL);

	pHandle->prev_input_1 = 0.0f;
	pHandle->prev_input_2 = 0.0f;
	pHandle->prev_output_1 = 0.0f;
	pHandle->prev_output_2 = 0.0f;

	
	for (uint32_t i = 0U; i < uCount; i++)
	{
		fInValue = pBuffer[i];

		pBuffer[i] = (pHandle->b0 * fInValue) +
			(pHandle->b1 * pHandle->prev_input_1) +
			(pHandle->b2 * pHandle->prev_input_2) -
			(pHandle->a1 * pHandle->prev_output_1) -
			(pHandle->a2 * pHandle->prev_output_2);

		pHandle->prev_input_2 = pHandle->prev_input_1;
		pHandle->prev_input_1 = fInValue;
		pHandle->prev_output_2 = pHandle->prev_output_1;
		pHandle->prev_output_1 = pBuffer[i];
	}

}


void biquad_delete(Biquad* pHandle)
{
	if (pHandle)
	{
		free(pHandle);
		pHandle = NULL;
	}
}