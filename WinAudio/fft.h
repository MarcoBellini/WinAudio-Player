
#ifndef FFT_H
#define FFT_H

// Define Some Math Const
#ifndef PI
#define PI						3.1415926535897932384626433832795028f
#endif

// Define FFT Sample Size (power of 2)
#define FFT_Size_512			512
#define FFT_Size_1024			1024
#define FFT_Size_2048			2048
#define FFT_Size_4096			4096
#define FFT_Size_8192			8192

// Complex
typedef struct tagComplex
{
	float Re;
	float Im;
} Complex;

// Instance Data
typedef struct tagFFT
{
	Complex* CosSinTable;
	int32_t* BitRevTable;
	float* EqualizeTable;
	float* EnvelopeTable;
	int32_t uInSampleSize;
	int32_t uOutSampleSize;
	int32_t uOutSampleSize2;
	Complex* ComplexSamples;
} FFT;

bool fft_Init(FFT** pHandle, int32_t uInSamples, int32_t uOutSamples);
void fft_Destroy(FFT** pHandle);
void fft_TimeToFrequencyDomain(FFT* pHandle, float* InSamples, float* OutSamples, bool bEqualize);


#endif
