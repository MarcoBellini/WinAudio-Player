
#ifndef UTILITY_H

#define UTILITY_H

// Alloc Utilities
bool ult_safe_new_malloc(void** pInPointer, size_t nSize);
void ult_safe_release_malloc(void** pInPointer);

// PCM Utility
void ult_8b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount);
void ult_8b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount);
void ult_16b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount);
void ult_16b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount);

void ult_8b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut,  uint32_t uCount);
void ult_8b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount);
void ult_16b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut,  uint32_t uCount);
void ult_16b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount);

void ult_16b_2c_bytes_to_16b_1c_float(const int8_t* pIn, float* pOut, uint32_t uCount);

#endif
