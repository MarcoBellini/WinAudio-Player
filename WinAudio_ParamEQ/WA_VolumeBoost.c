
#include "pch.h"
#include "WA_VolumeBoost.h"


#define WA_SILENCE_VALUE					0.000079433 // -62DB


struct tagWA_Boost
{
	double fMaxMultiplier;
	double fMaxGain;
	uint32_t fAttackTimeMs;
	uint32_t fReleaseTimeMs;
	uint32_t uAvgBytesPerSec;
	uint32_t uChannels;
	uint32_t uAttackBytes;
	uint32_t uReleaseBytes;
	uint32_t uElapsedAttackBytes;
	uint32_t uElapsedReleaseBytes;


	double fCurrentPeakValue;
	uint32_t uCurrentPeakPosition;
	bool bPeakIsFound;

	double fMuliplierFactor;
	double fTargetMuliplierFactor;
};

static inline uint32_t WA_Volume_Boost_MsToBytes(WA_Boost* pHandle, uint32_t uInMs)
{
	return (uInMs * pHandle->uAvgBytesPerSec) / 1000;

}

static void WA_Volume_Boost_Find_Peak(WA_Boost* pHandle, const double* pBuffer, uint32_t nBufferCount)
{
	double fAbsValue;

	pHandle->fCurrentPeakValue = 0.0;
	pHandle->bPeakIsFound = false;

	for (uint32_t cn = 0U; cn < pHandle->uChannels; cn++)
	{
		for (uint32_t i = cn; i < nBufferCount; i+= pHandle->uChannels)
		{
			fAbsValue = fabs(pBuffer[i]);

			if (pHandle->fCurrentPeakValue < fAbsValue)
			{
				if (fAbsValue > WA_SILENCE_VALUE)
				{
					pHandle->fCurrentPeakValue = fAbsValue;
					pHandle->bPeakIsFound = true;
				}
			}
		}
	}	
}

static inline void WA_Volume_Boost_Calc_Target_Multiplier(WA_Boost* pHandle)
{
	pHandle->fTargetMuliplierFactor = pHandle->fMaxGain / pHandle->fCurrentPeakValue;

	if (pHandle->fTargetMuliplierFactor > pHandle->fMaxMultiplier)
		pHandle->fTargetMuliplierFactor = pHandle->fMaxMultiplier;

}

static void WA_Volume_Boost_Multiply_Samples(WA_Boost* pHandle, double* pBuffer, uint32_t nBufferCount)
{
	double fRatio, fStep;

	if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
	{
		fRatio = pHandle->fTargetMuliplierFactor - pHandle->fMuliplierFactor;
		fStep = fRatio / (pHandle->uAttackBytes - pHandle->uElapsedAttackBytes);		
		pHandle->uElapsedReleaseBytes = 0;
	}
	else
	{
		fRatio = pHandle->fMuliplierFactor - pHandle->fTargetMuliplierFactor;
		fStep = fRatio / (pHandle->uReleaseBytes - pHandle->uElapsedReleaseBytes);		
		pHandle->uElapsedAttackBytes = 0;
	}


	for (uint32_t cn = 0U; cn < pHandle->uChannels; cn++)
	{
		for (uint32_t i = cn; i < nBufferCount; i += pHandle->uChannels)
		{
			
			if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
			{
				// Increase Volume
				pHandle->fMuliplierFactor += fStep;
				pHandle->uElapsedAttackBytes += 1;

				if (pHandle->uElapsedAttackBytes == pHandle->uAttackBytes)
				{
					pHandle->uElapsedAttackBytes = 0;
					pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;
					_RPTF1(_CRT_WARN, "Increased: %f \n", pHandle->fMuliplierFactor);
				}
			}
			else
			{				
				// Decrease Volume
				pHandle->fMuliplierFactor -= fStep;
				pHandle->uElapsedReleaseBytes += 1;

				if (pHandle->uElapsedReleaseBytes == pHandle->uReleaseBytes)
				{
					pHandle->uElapsedReleaseBytes = 0;
					pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;
				}				
			}

			pBuffer[i] *= pHandle->fMuliplierFactor;
		}
	}


}



WA_Boost* WA_Volume_Boost_Init()
{
	WA_Boost* pHandle;


	pHandle = (WA_Boost*) malloc(sizeof(WA_Boost));

	// Return NULL on malloc Fail
	if (!pHandle)
		return NULL;

	// Filter Params
	pHandle->fAttackTimeMs = 1000;		// 1000 ms
	pHandle->fReleaseTimeMs = 40;		// 40 ms
	pHandle->fMaxGain = 1.0;		// from 0.0f to 1.0f
	pHandle->fMaxMultiplier = 5.0;	// Max Mutiplier Factor

	// Reset Values
	pHandle->fMuliplierFactor = 1.0;
	pHandle->uAttackBytes = 0;
	pHandle->uReleaseBytes = 0;
	pHandle->fCurrentPeakValue = 0;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;
	pHandle->fTargetMuliplierFactor = 1.0;
	pHandle->uElapsedAttackBytes = 0;
	pHandle->uElapsedReleaseBytes = 0;

	return pHandle;
}

void WA_Volume_Boost_Delete(WA_Boost* pHandle)
{
	if (pHandle)
	{
		free(pHandle);
		pHandle = NULL;
	}
}

void WA_Volume_Boost_Update(WA_Boost* pHandle, uint32_t uAvgBytesPerSec, uint32_t uChannels, double fFactor)
{
	// Reset Values
	pHandle->fMuliplierFactor = 1.0;
	pHandle->fCurrentPeakValue = 0;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;
	pHandle->fTargetMuliplierFactor = 1.0;
	pHandle->uElapsedAttackBytes = 0;
	pHandle->uElapsedReleaseBytes = 0;

	pHandle->fMaxMultiplier = fFactor;


	pHandle->uAvgBytesPerSec = uAvgBytesPerSec;
	pHandle->uChannels = uChannels;

	pHandle->uAttackBytes = WA_Volume_Boost_MsToBytes(pHandle, pHandle->fAttackTimeMs);
	pHandle->uReleaseBytes = WA_Volume_Boost_MsToBytes(pHandle, pHandle->fReleaseTimeMs);
}



void WA_Volume_Boost_Process(WA_Boost* pHandle, double* pBuffer, uint32_t nBufferCount)
{
	// Find The Peak of Current Sample
	WA_Volume_Boost_Find_Peak(pHandle, pBuffer, nBufferCount);

	// Process Audio only if we found non silence data
	if (pHandle->bPeakIsFound)
	{
		WA_Volume_Boost_Calc_Target_Multiplier(pHandle);
		WA_Volume_Boost_Multiply_Samples(pHandle, pBuffer, nBufferCount);
	}

}