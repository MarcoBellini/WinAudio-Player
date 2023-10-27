#ifndef WA_VOLUME_BOOST_H
#define WA_VOLUME_BOOST_H

// Opaque Structure Declaration
struct tagWA_Boost;
typedef struct tagWA_Boost WA_Boost;


WA_Boost* WA_Volume_Boost_Init();
void WA_Volume_Boost_Delete(WA_Boost* pHandle);
void WA_Volume_Boost_Update(WA_Boost* pHandle, uint32_t uAvgBytesPerSec, uint32_t uChannels, double fFactor);
void WA_Volume_Boost_Process(WA_Boost* pHandle, double *pBuffer, uint32_t nBufferCount);



#endif