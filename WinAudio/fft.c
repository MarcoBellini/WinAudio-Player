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
This is a  C port of Nullsoft FFT class
https://github.com/WACUP/vis_classic/tree/master/FFTNullsoft
*/


#include "stdafx.h"
#include "fft.h"



static inline bool IsPowerOfTwo(int32_t uVal)
{
	return (uVal != 0) && ((uVal & (uVal - 1)) == 0);
}

static void InitBitRevTable(FFT* pHandle)
{

	for (int32_t i = 0; i < pHandle->uOutSampleSize; i++)
		pHandle->BitRevTable[i] = i;

	for (int32_t i = 0, j = 0; i < pHandle->uOutSampleSize; i++)
	{
		if (j > i)
		{
			int32_t temp = pHandle->BitRevTable[i];
			pHandle->BitRevTable[i] = pHandle->BitRevTable[j];
			pHandle->BitRevTable[j] = temp;
		}

		int32_t m = pHandle->uOutSampleSize >> 1;

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

static void InitEnvelopeTable(FFT* pHandle)
{

	float fMult = 1.0f / (float)pHandle->uInSampleSize * 6.2831853f;
	float fPower = 0.2f;

	if (fPower == 1.0f)
		for (int32_t i = 0; i < pHandle->uInSampleSize; i++)
			pHandle->EnvelopeTable[i] = 0.5f + 0.5f * sinf(i * fMult - 1.5707963268f);
	else
		for (int32_t i = 0; i < pHandle->uInSampleSize; i++)
			pHandle->EnvelopeTable[i] = powf(0.5f + 0.5f * sinf(i * fMult - 1.5707963268f), fPower);

}

/// <summary>
/// Setup a log table
/// the part of a log curve to use is from 1 to <log base>
/// a bit of an adjustment is added to 1 to account for the sharp drop off at the low end
/// inverse half is (<base> - 1) / <elements>
/// equalize table will have values from > ~0 to <= 1
/// </summary>
static void InitEqualizeTable(FFT* pHandle)
{	
	float bias = 0.04f;
	float inv_half_nfreq;

	for (int32_t i = 0; i < pHandle->uOutSampleSize2; i++)
	{
		inv_half_nfreq = (9.0f - bias) / (float)(pHandle->uOutSampleSize2);
		pHandle->EqualizeTable[i] = log10f(1.0f + bias + (float)(i + 1) * inv_half_nfreq);
		bias /= 1.0025f;
	}

}

static void InitCosSinTable(FFT* pHandle)
{
	int32_t dftsize = 2;
	int32_t i = 0;

	while (dftsize <= pHandle->uOutSampleSize)
	{
		float theta = (float)(-2.0f * PI / (float)dftsize);
		pHandle->CosSinTable[i].Re = cosf(theta);
		pHandle->CosSinTable[i].Im = sinf(theta);
		i++;
		dftsize <<= 1;
	}
}


bool fft_Init(FFT** pHandle, int32_t uInSamples, int32_t uOutSamples)
{

	// Allow only 2^n Output Samples
	if (!IsPowerOfTwo(uOutSamples))
		return false;

	// Alloc Memory to hold session datas
	(*pHandle) = (FFT*) malloc(sizeof(FFT));

	if (!(*pHandle))
		return false;

	// Store Samples size
	(*pHandle)->uOutSampleSize = uOutSamples;
	(*pHandle)->uOutSampleSize2 = uOutSamples / 2;
	(*pHandle)->uInSampleSize = uInSamples;

	(*pHandle)->BitRevTable = (int32_t*)malloc(uOutSamples * sizeof(int32_t));
	(*pHandle)->EqualizeTable = (float*)malloc(uOutSamples / 2 * sizeof(float));
	(*pHandle)->EnvelopeTable = (float*)malloc(uInSamples * sizeof(float));
	(*pHandle)->CosSinTable = (Complex*)malloc(uOutSamples / 2 * sizeof(Complex));
	(*pHandle)->ComplexSamples = (Complex*)malloc(uOutSamples * sizeof(Complex));

	// Check if we have valid pointers, otherwise free
	// allocated memory and return false
	if ((!(*pHandle)->BitRevTable) ||
		(!(*pHandle)->EqualizeTable) ||
		(!(*pHandle)->EnvelopeTable) ||
		(!(*pHandle)->CosSinTable) ||
		(!(*pHandle)->ComplexSamples))
	{
		if ((*pHandle)->BitRevTable)
			free((*pHandle)->BitRevTable);

		if ((*pHandle)->EqualizeTable)
			free((*pHandle)->EqualizeTable);

		if ((*pHandle)->EnvelopeTable)
			free((*pHandle)->EnvelopeTable);

		if ((*pHandle)->CosSinTable)
			free((*pHandle)->CosSinTable);

		if ((*pHandle)->ComplexSamples)
			free((*pHandle)->ComplexSamples);

		(*pHandle)->BitRevTable = NULL;
		(*pHandle)->EqualizeTable = NULL;
		(*pHandle)->EnvelopeTable = NULL;
		(*pHandle)->CosSinTable = NULL;
		(*pHandle)->ComplexSamples = NULL;

		free((*pHandle));
		(*pHandle) = NULL;

		return false;
	}

	// Init Tables
	InitBitRevTable(*pHandle);
	InitEqualizeTable(*pHandle);
	InitEnvelopeTable(*pHandle);
	InitCosSinTable(*pHandle);

	return true;
}
void fft_Destroy(FFT** pHandle)
{
	if ((*pHandle))
	{
		if((*pHandle)->BitRevTable)
			free((*pHandle)->BitRevTable);

		if ((*pHandle)->EqualizeTable)
			free((*pHandle)->EqualizeTable);

		if ((*pHandle)->EnvelopeTable)
			free((*pHandle)->EnvelopeTable);

		if ((*pHandle)->CosSinTable)
			free((*pHandle)->CosSinTable);

		if ((*pHandle)->ComplexSamples)
			free((*pHandle)->ComplexSamples);

		(*pHandle)->BitRevTable = NULL;
		(*pHandle)->EqualizeTable = NULL;
		(*pHandle)->EnvelopeTable = NULL;
		(*pHandle)->CosSinTable = NULL;
		(*pHandle)->ComplexSamples = NULL;


		free((*pHandle));
		(*pHandle) = NULL;
	}
}

void fft_TimeToFrequencyDomain(FFT* pHandle, float* InSamples, float* OutSamples, bool bEqualize)
{
	int32_t dftsize = 2;
	int32_t t = 0;


	// Reverse Bits and apply Envelope Table
	for (int32_t i = 0; i < pHandle->uOutSampleSize; i++)
	{
		int32_t idx = pHandle->BitRevTable[i];

		if (idx < pHandle->uInSampleSize)
		{
			pHandle->ComplexSamples[i].Re = InSamples[idx] * (pHandle->EnvelopeTable[idx]);
			pHandle->ComplexSamples[i].Im = 0.0f;
		}			
		else
		{
			pHandle->ComplexSamples[i].Re = 0.0f;
			pHandle->ComplexSamples[i].Im = 0.0f;
		}

	}


	// Perform FFT
	while (dftsize <= pHandle->uOutSampleSize)
	{
		float wpr = pHandle->CosSinTable[t].Re;
		float wpi = pHandle->CosSinTable[t].Im;
		float wr = 1.0f;
		float wi = 0.0f;
		int hdftsize = dftsize >> 1;

		for (int32_t m = 0; m < hdftsize; m += 1)
		{
			for (int32_t i = m; i < pHandle->uOutSampleSize; i += dftsize)
			{
				int32_t j = i + hdftsize;
				float tempr = wr * pHandle->ComplexSamples[j].Re - wi * pHandle->ComplexSamples[j].Im;
				float tempi = wr * pHandle->ComplexSamples[j].Im + wi * pHandle->ComplexSamples[j].Re;


				pHandle->ComplexSamples[j].Re = pHandle->ComplexSamples[i].Re - tempr;
				pHandle->ComplexSamples[j].Im = pHandle->ComplexSamples[i].Im - tempi;
				pHandle->ComplexSamples[i].Re += tempr;
				pHandle->ComplexSamples[i].Im += tempi;
			}

			float wtemp = wr;
			wr = wr * wpr - wi * wpi;
			wi = wi * wpr + wtemp * wpi;
		}

		dftsize <<= 1;
		t++;
	}

	// Take Magnitude and apply equalize to scale to a log10 values
	for (int32_t i = 0; i < pHandle->uOutSampleSize2; i++)
	{
		float f = sqrtf(pHandle->ComplexSamples[i].Re * pHandle->ComplexSamples[i].Re +
			pHandle->ComplexSamples[i].Im * pHandle->ComplexSamples[i].Im);

		if (bEqualize == true)
			OutSamples[i] = (pHandle->EqualizeTable[i]) * f;
		else
			OutSamples[i] = f;
	}
}

