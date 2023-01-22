#ifndef WA_CIRCLEBUFFER_H
#define WA_CIRCLEBUFFER_H

struct tagWA_CircleBuffer;
typedef struct tagWA_CircleBuffer WA_CircleBuffer;

/// <summary>
/// Create new Circle Buffer to store data
/// </summary>
/// <param name="uBufferSize">Size in bytes of the buffer</param>
/// <returns>NULL on fail or a Pointer to a WA_CircleBuffer Instance</returns>
WA_CircleBuffer* WA_CircleBuffer_New(uint32_t uBufferSize);

/// <summary>
/// Free memory allocated with WA_CircleBuffer_New function
/// </summary>
/// <param name="This">Current Instance Pointer</param>
void WA_CircleBuffer_Delete(WA_CircleBuffer* This);

/// <summary>
/// Write data in the Circle Buffer
/// </summary>
/// <param name="This">Instance Pointer</param>
/// <param name="pBuffer">Pointer to a Byte Buffer</param>
/// <param name="uBufferSize">Size of the Buffer</param>
/// <returns>True on Success</returns>
bool WA_CircleBuffer_Write(WA_CircleBuffer* This, int8_t* pBuffer, uint32_t uBufferSize);

/// <summary>
/// Read Data from the Circle Buffer
/// </summary>
/// <param name="This">Instance Pointer</param>
/// <param name="pBuffer">Pointer to an Empty Buffer</param>
/// <param name="uBufferSize">Size of the Buffer</param>
/// <param name="uPosition">Position in bytes from which to start reading </param>
/// <returns>True on Success</returns>
bool WA_CircleBuffer_ReadFrom(WA_CircleBuffer* This, int8_t* pBuffer, uint32_t uBufferSize, uint32_t uPosition);

/// <summary>
/// Reset Write Index to Position 0
/// </summary>
/// <param name="This">Instance Pointer</param>
void WA_CircleBuffer_Reset(WA_CircleBuffer* This);

#endif
