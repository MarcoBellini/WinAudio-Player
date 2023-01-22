
#include "pch.h"
#include "WA_Guids.h"
#include "WA_Macro.h"
#include "WA_CircleBuffer.h"


/// <summary>
/// Module Instance Data
/// </summary>
struct tagWA_CircleBuffer
{
	uint32_t uWriteIndex;
	int8_t* pBuffer;
	uint32_t uBufferSize;
};


WA_CircleBuffer* WA_CircleBuffer_New(uint32_t uBufferSize)
{
	WA_CircleBuffer* This;

	if (uBufferSize == 0)
		return NULL;

	This = (WA_CircleBuffer*)calloc(1, sizeof(WA_CircleBuffer));

	if (!This)
		return NULL;


	This->pBuffer = (int8_t*)calloc(uBufferSize, sizeof(int8_t));

	if (!This->pBuffer)
	{
		free(This);
		return NULL;
	}

	This->uBufferSize = uBufferSize;	
	This->uWriteIndex = 0;

	return This;
}

void WA_CircleBuffer_Delete(WA_CircleBuffer* This)
{
	if (!This)
		return;

	if (This->pBuffer)
		free(This->pBuffer);

	free(This);
	This = NULL;
}

bool WA_CircleBuffer_Write(WA_CircleBuffer* This, int8_t* pBuffer, uint32_t uBufferSize)
{
	uint32_t uIndex;

	if (!This)
		return false;

	uIndex = 0U;
	while (uIndex < uBufferSize)
	{
		This->pBuffer[This->uWriteIndex] = pBuffer[uIndex];
		uIndex++;
		This->uWriteIndex++;
		This->uWriteIndex = This->uWriteIndex % This->uBufferSize;
		
	}


	return true;
}

bool WA_CircleBuffer_ReadFrom(WA_CircleBuffer* This, int8_t* pBuffer, uint32_t uBufferSize, uint32_t uPosition)
{
	uint32_t uIndex;

	if (!This)
		return false;

	if (This->uBufferSize < uPosition)
		return false;

	if (!pBuffer)
		return false;

	if (uBufferSize == 0U)
		return false;
	

	uIndex = 0U;
	while (uIndex < uBufferSize)
	{
		pBuffer[uIndex] = This->pBuffer[uPosition];
		uIndex++;
		uPosition++;
		uPosition = uPosition % This->uBufferSize;	

	}

	return true;
}

void WA_CircleBuffer_Reset(WA_CircleBuffer* This)
{
	if (!This)
		return;

	This->uWriteIndex = 0U;

}