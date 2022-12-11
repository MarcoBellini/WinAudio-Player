
#include "stdafx.h"
#include "CircleBuffer.h"

// Function Declaration
bool CircleBuffer_Create(BUFFER_HANDLE pHandle, uint32_t uBufferSize);
void CircleBuffer_Destroy(BUFFER_HANDLE pHandle);
void CircleBuffer_Reset(BUFFER_HANDLE pHandle);
bool CircleBuffer_Write(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen);
bool CircleBuffer_Read(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen);
bool CircleBuffer_FreeSpace(BUFFER_HANDLE pHandle, uint32_t* puFreeSpace);
bool CircleBuffer_UsedSpace(BUFFER_HANDLE pHandle, uint32_t* puUsedSpace);
bool CircleBuffer_Length(BUFFER_HANDLE pHandle, uint32_t* puBufferLen);

// Instance Data
typedef struct tagCircleBufferInstance
{
	int8_t* pBuffer;
	uint32_t uBufferSize;
	uint32_t uWriteIndex;
	uint32_t uReadIndex;
	uint32_t uValidData;
} CircleBufferInstance;

bool CircleBuffer_Initialize(BUFFER_HANDLE pHandle)
{
	// Check if we have a valid pointer
	if (!pHandle)
	{
		_RPTF0(_CRT_WARN, "CircleBuffer: Invalid Pointer");
		return false;
	}
		
	// Alloc memory for module instance
	pHandle->pModulePrivateData = malloc(sizeof(CircleBufferInstance));

	if (!pHandle->pModulePrivateData)
	{
		_RPTF0(_CRT_WARN, "CircleBuffer: Fail to allocate memory");
		return false;
	}

	// Reset memory
	ZeroMemory(pHandle->pModulePrivateData, sizeof(CircleBufferInstance));


	// Assign Function Pointers
	pHandle->CircleBuffer_Create = &CircleBuffer_Create;
	pHandle->CircleBuffer_Destroy = &CircleBuffer_Destroy;
	pHandle->CircleBuffer_FreeSpace = &CircleBuffer_FreeSpace;
	pHandle->CircleBuffer_UsedSpace = &CircleBuffer_UsedSpace;
	pHandle->CircleBuffer_Read = &CircleBuffer_Read;
	pHandle->CircleBuffer_Write = &CircleBuffer_Write;
	pHandle->CircleBuffer_Length = &CircleBuffer_Length;
	pHandle->CircleBuffer_Reset = &CircleBuffer_Reset;

	return true;
}

bool CircleBuffer_Uninitialize(BUFFER_HANDLE pHandle)
{
	// Check if we have a valid pointer
	if (!pHandle)
	{
		_RPTF0(_CRT_WARN, "CircleBuffer: Invalid Pointer");
		return false;
	}

	// Free allocated memory
	if (!pHandle->pModulePrivateData)
	{
		_RPTF0(_CRT_WARN, "CircleBuffer: Fail to free memory");
		return false;
	}

	// Ensure to Destroy memory
	CircleBuffer_Destroy(pHandle);

	// Free private data
	if (pHandle->pModulePrivateData)
	{
		free(pHandle->pModulePrivateData);
		pHandle->pModulePrivateData = NULL;
	}
	

	return true;
}

bool CircleBuffer_Create(BUFFER_HANDLE pHandle, uint32_t uBufferSize) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	_ASSERT(uBufferSize > 0);

	if (uBufferSize > 0)
		pInstance->pBuffer = (int8_t*)malloc(uBufferSize);

	// Check if allocation is ok
	if (!pInstance->pBuffer)
		return false;

	// Clear Memory
	ZeroMemory(pInstance->pBuffer, uBufferSize);

	// Reset Vars
	pInstance->uBufferSize = uBufferSize;
	pInstance->uReadIndex = 0;
	pInstance->uWriteIndex = 0;
	pInstance->uValidData = 0;
	
	_RPTF1(_CRT_WARN, "Created Circle Buffer, size: %d \n", uBufferSize);

	return true;
}

void CircleBuffer_Destroy(BUFFER_HANDLE pHandle) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;


	// Free allocated memory
	if (pInstance->pBuffer)
	{
		free(pInstance->pBuffer);
		pInstance->pBuffer = NULL;

		_RPTF0(_CRT_WARN, "Destroyed Circle Buffer\n");
	}
		
}

void CircleBuffer_Reset(BUFFER_HANDLE pHandle) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return;

	pInstance->uReadIndex = 0;
	pInstance->uWriteIndex = 0;
	pInstance->uValidData = 0;	

}

bool CircleBuffer_Write(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return false;

	// Check if there is enough free space to
	// fill the requested len
	if ((pInstance->uBufferSize - pInstance->uValidData) < uBufferLen)
		return false;

	// Check if we need to split the read in two parts
	if ((pInstance->uWriteIndex + uBufferLen) <= pInstance->uBufferSize)
	{
		/*
		memcpy(pInstance->pBuffer + pInstance->uWriteIndex,
			pByteBuffer,
			uBufferLen);

		*/

		memcpy_s(pInstance->pBuffer + pInstance->uWriteIndex,
			pInstance->uBufferSize,
			pByteBuffer, 
			uBufferLen);

		pInstance->uWriteIndex += uBufferLen;
		pInstance->uValidData += uBufferLen;
	}
	else
	{
		uint32_t uFirstPart, uSecondPart;

		uFirstPart = pInstance->uBufferSize - pInstance->uWriteIndex;
		uSecondPart = uBufferLen - uFirstPart;


		_ASSERT(uSecondPart > 0);

		/*
		memcpy(pInstance->pBuffer + pInstance->uWriteIndex,
			pByteBuffer,
			uFirstPart);

		memcpy(pInstance->pBuffer,
			pByteBuffer + uFirstPart,
			uSecondPart);

		*/

		memcpy_s(pInstance->pBuffer + pInstance->uWriteIndex,
			pInstance->uBufferSize,
			pByteBuffer,
			uFirstPart);

		memcpy_s(pInstance->pBuffer,
			pInstance->uBufferSize,
			pByteBuffer + uFirstPart,
			uSecondPart);

		pInstance->uWriteIndex = uSecondPart;
		pInstance->uValidData += uBufferLen;
	}
	

	return true;
}

bool CircleBuffer_Read(BUFFER_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBufferLen) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return false;

	// Check if there is enough valid data to
	// fill the requested len
	if (pInstance->uValidData < uBufferLen)
		return false;

	// Check if we need to split the read in two parts
	if ((pInstance->uReadIndex + uBufferLen) <= pInstance->uBufferSize)
	{
		/*
		memcpy(pByteBuffer,
				pInstance->pBuffer + pInstance->uReadIndex,
				uBufferLen);

		*/

		memcpy_s(pByteBuffer,
			uBufferLen,
			pInstance->pBuffer + pInstance->uReadIndex,
			uBufferLen);

		pInstance->uReadIndex += uBufferLen;
		pInstance->uValidData -= uBufferLen;
	}
	else
	{
		uint32_t uFirstPart, uSecondPart;

		uFirstPart = pInstance->uBufferSize - pInstance->uReadIndex;
		uSecondPart = uBufferLen - uFirstPart;

		_ASSERT(uSecondPart > 0);

		/*
		memcpy(pByteBuffer,
			pInstance->pBuffer + pInstance->uReadIndex,
			uFirstPart);

		memcpy(pByteBuffer + uFirstPart,
			pInstance->pBuffer,
			uSecondPart);

		*/

		memcpy_s(pByteBuffer,
			uBufferLen,
			pInstance->pBuffer + pInstance->uReadIndex,
			uFirstPart);

		memcpy_s(pByteBuffer + uFirstPart,
			uBufferLen,
			pInstance->pBuffer,
			uSecondPart);

		pInstance->uReadIndex = uSecondPart;
		pInstance->uValidData -= uBufferLen;
	}
	

	return true;
}

bool CircleBuffer_FreeSpace(BUFFER_HANDLE pHandle, uint32_t* puFreeSpace) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return false;


	(*puFreeSpace) = pInstance->uBufferSize - pInstance->uValidData;

	return true;
}

bool CircleBuffer_UsedSpace(BUFFER_HANDLE pHandle, uint32_t* puUsedSpace) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return false;

	(*puUsedSpace) = pInstance->uValidData;

	return true;
}

bool CircleBuffer_Length(BUFFER_HANDLE pHandle, uint32_t* puBufferLen) 
{
	CircleBufferInstance* pInstance = (CircleBufferInstance*)pHandle->pModulePrivateData;

	if (!pInstance->pBuffer)
		return false;

	(*puBufferLen) = pInstance->uBufferSize;

	return true;
}