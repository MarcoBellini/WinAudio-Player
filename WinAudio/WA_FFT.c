/*
  LICENSE
  -------
	Copyright 2005-2013 Nullsoft, Inc.
	All rights reserved.
	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:
	  * Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.
	  * Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.
	  * Neither the name of Nullsoft nor the names of its contributors may be used to
		endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
	OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/




/*
	This is a Modified C port of Nullsoft FFT class
	https://github.com/WACUP/vis_classic/tree/master/FFTNullsoft


	Modified version to allocate all arrays at compile-time 
	and avoid unnecessary mallocs.
*/


#include "stdafx.h"
#include "WA_FFT.h"

// Complex
typedef struct TagComplex
{
	float Re;
	float Im;
} Complex;


// Instance Data
struct TagWA_FFT
{
	Complex CosSinTable[WA_FFT_OUTPUT_SIZE_HALF];
	int32_t BitRevTable[WA_FFT_OUTPUT_SIZE];
	float EqualizeTable[WA_FFT_OUTPUT_SIZE_HALF];
	float EnvelopeTable[WA_FFT_INPUT_SIZE];
	int32_t uInSampleSize;
	int32_t uOutSampleSize;
	int32_t uOutSampleSize2;
	Complex ComplexSamples[WA_FFT_OUTPUT_SIZE];
};

static inline bool IsPowerOfTwo(int32_t uVal)
{
	return (uVal != 0) && ((uVal & (uVal - 1)) == 0);
}

static void InitBitRevTable(WA_FFT* This)
{

	for (int32_t i = 0; i < This->uOutSampleSize; i++)
		This->BitRevTable[i] = i;

	for (int32_t i = 0, j = 0; i < This->uOutSampleSize; i++)
	{
		if (j > i)
		{
			int32_t temp = This->BitRevTable[i];
			This->BitRevTable[i] = This->BitRevTable[j];
			This->BitRevTable[j] = temp;
		}

		int32_t m = This->uOutSampleSize >> 1;

		while (m >= 1 && j >= m)
		{
			j -= m;
			m >>= 1;
		}

		j += m;
	}
}

/// <summary>
/// this precomputation is for multiplying the waveform sample 
///  by a bell-curve-shaped envelope, so we don't see the ugly 
///  frequency response (oscillations) of a square filter.
/// 
/// a power of 1.0 will compute the FFT with exactly one convolution.
/// a power of 2.0 is like doing it twice; the resulting frequency
///    output will be smoothed out and the peaks will be "fatter".
///  a power of 0.5 is closer to not using an envelope, and you start
///   to get the frequency response of the square filter as 'power'
///    approaches zero; the peaks get tighter and more precise, but
///    you also see small oscillations around their bases.
/// </summary>

static void InitEnvelopeTable(WA_FFT* This)
{

	float fMult = 1.0f / (float)This->uInSampleSize * 6.2831853f;
	float fPower = 0.2f;

	if (fPower == 1.0f)
		for (int32_t i = 0; i < This->uInSampleSize; i++)
			This->EnvelopeTable[i] = 0.5f + 0.5f * sinf(i * fMult - 1.5707963268f);
	else
		for (int32_t i = 0; i < This->uInSampleSize; i++)
			This->EnvelopeTable[i] = powf(0.5f + 0.5f * sinf(i * fMult - 1.5707963268f), fPower);

}

/// <summary>
/// Setup a log table
/// the part of a log curve to use is from 1 to <log base>
/// a bit of an adjustment is added to 1 to account for the sharp drop off at the low end
/// inverse half is (<base> - 1) / <elements>
/// equalize table will have values from > ~0 to <= 1
/// </summary>
static void InitEqualizeTable(WA_FFT* This)
{	
	float bias = 0.04f;
	float inv_half_nfreq;

	for (int32_t i = 0; i < This->uOutSampleSize2; i++)
	{
		inv_half_nfreq = (9.0f - bias) / (float)(This->uOutSampleSize2);
		This->EqualizeTable[i] = log10f(1.0f + bias + (float)(i + 1) * inv_half_nfreq);
		bias /= 1.0025f;
	}

}

static void InitCosSinTable(WA_FFT* This)
{
	int32_t dftsize = 2;
	int32_t i = 0;

	while (dftsize <= This->uOutSampleSize)
	{
		float theta = (float)(-2.0f * PI / (float)dftsize);
		This->CosSinTable[i].Re = cosf(theta);
		This->CosSinTable[i].Im = sinf(theta);
		i++;
		dftsize <<= 1;
	}
}


WA_FFT* WA_FFT_New()
{
	WA_FFT* This;

	if (!IsPowerOfTwo(WA_FFT_OUTPUT_SIZE))
		return NULL;
	
	This = (WA_FFT*) malloc(sizeof(WA_FFT));

	if(!This)
		return NULL;


	This->uOutSampleSize = WA_FFT_OUTPUT_SIZE;
	This->uOutSampleSize2 = WA_FFT_OUTPUT_SIZE_HALF;
	This->uInSampleSize = WA_FFT_INPUT_SIZE;


	InitBitRevTable(This);
	InitEqualizeTable(This);
	InitEnvelopeTable(This);
	InitCosSinTable(This);

	return This;
}


void WA_FFT_Delete(WA_FFT* This)
{
	if(This)
	{
		free(This);
		This = NULL;
	}
}

void WA_FFT_TimeToFrequencyDomain(WA_FFT* This, float* InSamples, float* OutSamples, bool bEqualize)
{
	int32_t dftsize = 2;
	int32_t t = 0;


	// Reverse Bits and apply Envelope Table
	for (int32_t i = 0; i < This->uOutSampleSize; i++)
	{
		int32_t idx = This->BitRevTable[i];

		if (idx < This->uInSampleSize)
		{
			This->ComplexSamples[i].Re = InSamples[idx] * (This->EnvelopeTable[idx]);
			This->ComplexSamples[i].Im = 0.0f;
		}			
		else
		{
			This->ComplexSamples[i].Re = 0.0f;
			This->ComplexSamples[i].Im = 0.0f;
		}

	}


	// Perform FFT
	while (dftsize <= This->uOutSampleSize)
	{
		float wpr = This->CosSinTable[t].Re;
		float wpi = This->CosSinTable[t].Im;
		float wr = 1.0f;
		float wi = 0.0f;
		int hdftsize = dftsize >> 1;

		for (int32_t m = 0; m < hdftsize; m += 1)
		{
			for (int32_t i = m; i < This->uOutSampleSize; i += dftsize)
			{
				int32_t j = i + hdftsize;
				float tempr = wr * This->ComplexSamples[j].Re - wi * This->ComplexSamples[j].Im;
				float tempi = wr * This->ComplexSamples[j].Im + wi * This->ComplexSamples[j].Re;


				This->ComplexSamples[j].Re = This->ComplexSamples[i].Re - tempr;
				This->ComplexSamples[j].Im = This->ComplexSamples[i].Im - tempi;
				This->ComplexSamples[i].Re += tempr;
				This->ComplexSamples[i].Im += tempi;
			}

			float wtemp = wr;
			wr = wr * wpr - wi * wpi;
			wi = wi * wpr + wtemp * wpi;
		}

		dftsize <<= 1;
		t++;
	}

	// Take Magnitude and apply equalize to scale to a log10 values
	for (int32_t i = 0; i < This->uOutSampleSize2; i++)
	{

		if (bEqualize == true)
			OutSamples[i] = sqrtf(This->ComplexSamples[i].Re * This->ComplexSamples[i].Re +
				This->ComplexSamples[i].Im * This->ComplexSamples[i].Im) * This->EqualizeTable[i];
		else
			OutSamples[i] = sqrtf(This->ComplexSamples[i].Re * This->ComplexSamples[i].Re +
				This->ComplexSamples[i].Im * This->ComplexSamples[i].Im);
	}
}

