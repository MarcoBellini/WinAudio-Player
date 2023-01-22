
#ifndef WA_FFT_H
#define WA_FFT_H

// Define Some Math Const
#ifndef PI
#define PI						3.1415926535897932384626433832795028f
#endif


// Store Here The size of FFT
#define WA_FFT_INPUT_SIZE 2048
#define WA_FFT_OUTPUT_SIZE 4096
#define WA_FFT_OUTPUT_SIZE_HALF 2048


struct TagWA_FFT;
typedef struct TagWA_FFT WA_FFT;

WA_FFT* WA_FFT_New();
void WA_FFT_Delete(WA_FFT* This);
void WA_FFT_TimeToFrequencyDomain(WA_FFT* This, float* InSamples, float* OutSamples, bool bEqualize);


#endif
