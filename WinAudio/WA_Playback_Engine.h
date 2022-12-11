#ifndef PLAYBACK_ENGINE_H
#define PLAYBACK_ENGINE_H


bool WA_Playback_Engine_New(void);
bool WA_Playback_Engine_Delete(void);

// TODO: Get Extensions Filter for Open File Dialog


bool WA_Playback_Engine_OpenFile(const wchar_t* lpwPath);
bool WA_Playback_Engine_CloseFile();

bool WA_Playback_Engine_Play(void);
bool WA_Playback_Engine_Pause(void);
bool WA_Playback_Engine_Resume(void);
bool WA_Playback_Engine_Stop(void);

bool WA_Playback_Engine_Seek(uint64_t uNewPositionMs);
bool WA_Playback_Engine_Get_Position(uint64_t* uCurrentPositionMs);
bool WA_Playback_Engine_Get_Duration(uint64_t* uCurrentDurationMs);

bool WA_Playback_Engine_Set_Volume(uint8_t uNewVolume);
bool WA_Playback_Engine_Get_Volume(uint8_t* puNewVolume);

bool WA_Playback_Engine_Get_Buffer(int8_t* pBuffer, uint32_t uBufferLen);

#endif
