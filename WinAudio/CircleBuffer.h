
#ifndef CIRCLEBUFFER_H
#define CIRCLEBUFFER_H

// Some example of memory mapping
// https://gist.github.com/rygorous/3158316
// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2
// https://stackoverflow.com/questions/1016888/windows-ring-buffer-without-copying

struct tagCircleBuffer;
typedef struct tagCircleBuffer* BUFFER_HANDLE;

typedef bool (*pCircleBuffer_Create)(BUFFER_HANDLE pHandle, uint32_t uBufferSize);
typedef void (*pCircleBuffer_Destroy)(BUFFER_HANDLE pHandle);
typedef void (*pCircleBuffer_Reset)(BUFFER_HANDLE pHandle);
typedef bool (*pCircleBuffer_Write)(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen);
typedef bool (*pCircleBuffer_Read)(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen);
typedef bool (*pCircleBuffer_FreeSpace)(BUFFER_HANDLE pHandle, uint32_t *puFreeSpace);
typedef bool (*pCircleBuffer_UsedSpace)(BUFFER_HANDLE pHandle, uint32_t* puUsedSpace);
typedef bool (*pCircleBuffer_Length)(BUFFER_HANDLE pHandle, uint32_t* puBufferLen);


typedef struct tagCircleBuffer
{
	// Instance Data
	void* pModulePrivateData;

	// Function Pointers
	pCircleBuffer_Create CircleBuffer_Create;
	pCircleBuffer_Destroy CircleBuffer_Destroy;
	pCircleBuffer_Reset CircleBuffer_Reset;
	pCircleBuffer_Write CircleBuffer_Write;
	pCircleBuffer_Read CircleBuffer_Read;
	pCircleBuffer_FreeSpace CircleBuffer_FreeSpace;
	pCircleBuffer_UsedSpace CircleBuffer_UsedSpace;
	pCircleBuffer_Length CircleBuffer_Length;
} CircleBuffer;


bool CircleBuffer_Initialize(BUFFER_HANDLE pHandle);
bool CircleBuffer_Uninitialize(BUFFER_HANDLE pHandle);

#endif
