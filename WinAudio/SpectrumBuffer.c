
#include "stdafx.h"
#include "SpectrumBuffer.h"


bool SpectrumBuffer_Create(SB_HANDLE pHandle, uint16_t uBufferSize);
void SpectrumBuffer_Destroy(SB_HANDLE pHandle);
bool SpectrumBuffer_Write(SB_HANDLE pHandle, int8_t* pByteBuffer, uint16_t uBufferLen);
bool SpectrumBuffer_ReadFrom(SB_HANDLE pHandle, int8_t* pByteBuffer, uint16_t uStartPosition, uint16_t uBufferLen);
void SpectrumBuffer_Reset(SB_HANDLE pHandle);

/// <summary>
/// Module Instance Data
/// </summary>
typedef struct tagSpectrumBufferInstance
{
	uint16_t uWriteIndex;
	int8_t* pBuffer;
	uint16_t uBufferSize;
} SpectrumBufferInstance;




bool SpectrumBuffer_Initialize(SB_HANDLE pHandle)
{
	SpectrumBufferInstance* pInstance;

	// Allocate Mem for module instance
	pInstance = (SpectrumBufferInstance*) malloc(sizeof(SpectrumBufferInstance));

	// Check if we have a valid pointer
	if (!pInstance)
		return false;

	// Assign Pointer to Public Struct
	pHandle->pPrivateData = (void*) pInstance;

	// Assign Pointer to Module functions
	pHandle->SpectrumBuffer_Create = &SpectrumBuffer_Create;
	pHandle->SpectrumBuffer_Destroy = &SpectrumBuffer_Destroy;
	pHandle->SpectrumBuffer_ReadFrom = &SpectrumBuffer_ReadFrom;
	pHandle->SpectrumBuffer_Reset = &SpectrumBuffer_Reset;
	pHandle->SpectrumBuffer_Write = &SpectrumBuffer_Write;

	return true;
}

bool SpectrumBuffer_Uninitialize(SB_HANDLE pHandle)
{
	// Free resources
	if (pHandle->pPrivateData)
	{
		free(pHandle->pPrivateData);
		pHandle->pPrivateData = NULL;
	}
		
	return true;
}


bool SpectrumBuffer_Create(SB_HANDLE pHandle, uint16_t uBufferSize)
{
	SpectrumBufferInstance* pInstance = (SpectrumBufferInstance *) pHandle->pPrivateData;

	_ASSERT(uBufferSize > 0);

	// Alloc Memory for Buffer
	pInstance->pBuffer = (int8_t*) malloc(uBufferSize);

	// Check if we havce a valid pointer
	if (!pInstance->pBuffer)
		return false;

	ZeroMemory(pInstance->pBuffer, uBufferSize);
	pInstance->uBufferSize = uBufferSize;
	pInstance->uWriteIndex = 0;

	_RPTF1(_CRT_WARN, "Created Spectrum Buffer, size: %d \n", uBufferSize);

	return true;
}

void SpectrumBuffer_Destroy(SB_HANDLE pHandle)
{
	SpectrumBufferInstance* pInstance = (SpectrumBufferInstance*)pHandle->pPrivateData;

	// Free resources
	if (pInstance->pBuffer)
	{
		free(pInstance->pBuffer);
		pInstance->pBuffer = NULL;

		_RPTF0(_CRT_WARN, "Destroyed Spectrum Buffer\n");
	}
}

bool SpectrumBuffer_Write(SB_HANDLE pHandle, int8_t* pByteBuffer, uint16_t uBufferLen)
{
	SpectrumBufferInstance* pInstance = (SpectrumBufferInstance*)pHandle->pPrivateData;

	_ASSERT(uBufferLen > 0);

	if (pInstance->uBufferSize < uBufferLen)
		return false;


	if ((uBufferLen + pInstance->uWriteIndex) <= pInstance->uBufferSize)
	{
		/*
		memcpy(pInstance->pBuffer + pInstance->uWriteIndex,
			pByteBuffer,
			uBufferLen);
		*/

		memcpy_s(pInstance->pBuffer + pInstance->uWriteIndex,
			uBufferLen,
			pByteBuffer,
			uBufferLen);

		pInstance->uWriteIndex += uBufferLen;
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
			uBufferLen,
			pByteBuffer,
			uFirstPart);

		memcpy_s(pInstance->pBuffer,
			uBufferLen,
			pByteBuffer + uFirstPart,
			uSecondPart);


		pInstance->uWriteIndex = uSecondPart;
	}

	return true;
}

bool SpectrumBuffer_ReadFrom(SB_HANDLE pHandle, int8_t* pByteBuffer, uint16_t uStartPosition, uint16_t uBufferLen)
{
	SpectrumBufferInstance* pInstance = (SpectrumBufferInstance*)pHandle->pPrivateData;

	_ASSERT(uBufferLen > 0);
	_ASSERT(uStartPosition <= pInstance->uBufferSize);

	if (pInstance->uBufferSize < uBufferLen)
		return false;

	if (pInstance->uBufferSize < uStartPosition)
		return false;

	if ((uStartPosition + uBufferLen) <= pInstance->uBufferSize)
	{
		/*
		memcpy(pByteBuffer,
			pInstance->pBuffer + uStartPosition,
			uBufferLen);
		*/

		memcpy_s(pByteBuffer,
			pInstance->uBufferSize,
			pInstance->pBuffer + uStartPosition,
			uBufferLen);
	}
	else
	{
		uint32_t uFirstPart, uSecondPart;

		uFirstPart = pInstance->uBufferSize - uStartPosition;
		uSecondPart = uBufferLen - uFirstPart;

		_ASSERT(uSecondPart > 0);

		/*
		memcpy(pByteBuffer,
			pInstance->pBuffer + uStartPosition,
			uFirstPart);

		memcpy(pByteBuffer + uFirstPart,
			pInstance->pBuffer,
			uSecondPart);
		*/

		memcpy_s(pByteBuffer,
			pInstance->uBufferSize,
			pInstance->pBuffer + uStartPosition,
			uFirstPart);

		memcpy_s(pByteBuffer + uFirstPart,
			pInstance->uBufferSize,
			pInstance->pBuffer,
			uSecondPart);
	}

	return true;
}

void SpectrumBuffer_Reset(SB_HANDLE pHandle)
{
	SpectrumBufferInstance* pInstance = (SpectrumBufferInstance*)pHandle->pPrivateData;

	pInstance->uWriteIndex = 0;
	ZeroMemory(pInstance->pBuffer, pInstance->uBufferSize);
}
