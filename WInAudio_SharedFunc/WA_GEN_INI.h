

#ifndef WA_GEN_INI_H
#define WA_GEN_INI_H


struct TagWA_Ini;
typedef struct TagWA_Ini WA_Ini;

WA_Ini* WA_Ini_New(void);
void WA_Ini_Delete(WA_Ini* This);

// Read Section
bool WA_Ini_Read_Bool(WA_Ini* This, bool Default, wchar_t *Section, wchar_t* Key);
int8_t WA_Ini_Read_Int8(WA_Ini* This, int8_t Default, wchar_t *Section, wchar_t* Key);
int16_t WA_Ini_Read_Int16(WA_Ini* This, int16_t Default, wchar_t *Section, wchar_t* Key);
int32_t WA_Ini_Read_Int32(WA_Ini* This, int32_t Default, wchar_t *Section, wchar_t* Key);
uint8_t WA_Ini_Read_UInt8(WA_Ini* This, uint8_t Default, wchar_t *Section, wchar_t* Key);
uint16_t WA_Ini_Read_UInt16(WA_Ini* This, uint16_t Default, wchar_t *Section, wchar_t* Key);
uint32_t WA_Ini_Read_UInt32(WA_Ini* This, uint32_t Default, wchar_t *Section, wchar_t* Key);
float WA_Ini_Read_Float(WA_Ini* This, float Default, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Read_String(WA_Ini* This, wchar_t *lpwBuffer, DWORD dwBufferSize, wchar_t* lpwDefault, wchar_t *Section, wchar_t* Key);
bool WA_Ini_Read_Struct(WA_Ini* This, void* pStruct, DWORD dwStructSize,  wchar_t* Section, wchar_t* Key);

// Write Section
bool WA_Ini_Write_Bool(WA_Ini* This, bool bValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_Int8(WA_Ini* This, int8_t nValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_Int16(WA_Ini* This, int16_t nValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_Int32(WA_Ini* This, int32_t nValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_UInt8(WA_Ini* This, uint8_t uValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_UInt16(WA_Ini* This, uint16_t uValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_UInt32(WA_Ini* This, uint32_t uValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_Float(WA_Ini* This, float uValue, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_String(WA_Ini* This, wchar_t* lpwBuffer, DWORD dwBufferSize, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Write_Struct(WA_Ini* This, void* pStruct, DWORD dwStructSize, wchar_t* Section, wchar_t* Key);


// Delete
bool WA_Ini_Delete_Key(WA_Ini* This, wchar_t* Section, wchar_t* Key);
bool WA_Ini_Delete_Section(WA_Ini* This, wchar_t* Section);

#endif
