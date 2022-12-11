
#ifndef SPECTRUM_BUFFER_H
#define SPECTRUM_BUFFER_H

struct tagSpectrumBuffer;
typedef struct tagSpectrumBuffer* SB_HANDLE;


typedef bool (*pSpectrumBuffer_Create)(SB_HANDLE pHandle, uint16_t uBufferSize);
typedef void (*pSpectrumBuffer_Destroy)(SB_HANDLE pHandle);
typedef bool (*pSpectrumBuffer_Write)(SB_HANDLE pHandle, int8_t *pByteBuffer, uint16_t uBufferSize);
typedef bool (*pSpectrumBuffer_ReadFrom)(SB_HANDLE pHandle, int8_t *pByteBuffer, uint16_t uStartPosition, uint16_t uBufferLen);
typedef void (*pSpectrumBuffer_Reset)(SB_HANDLE pHandle);

typedef struct tagSpectrumBuffer
{
	pSpectrumBuffer_Create SpectrumBuffer_Create;
	pSpectrumBuffer_Destroy SpectrumBuffer_Destroy;
	pSpectrumBuffer_Write SpectrumBuffer_Write;
	pSpectrumBuffer_ReadFrom SpectrumBuffer_ReadFrom;
	pSpectrumBuffer_Reset SpectrumBuffer_Reset;

	void* pPrivateData;

} SpectrumBuffer;

bool SpectrumBuffer_Initialize(SB_HANDLE pHandle);
bool SpectrumBuffer_Uninitialize(SB_HANDLE pHandle);

#endif
