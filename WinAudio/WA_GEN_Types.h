#ifndef WA_GEN_TYPES_H
#define WA_GEN_TYPES_H

#define WA_SIGNED_SAMPLE	0x002
#define WA_UNSIGNED_SAMPLE	0x004

// Define Simple Wave Fromat
typedef struct TagWA_AudioFormat
{
	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	uint32_t uAvgBytesPerSec;
	uint16_t uBlockAlign;
	uint8_t uSampleType; // Signed or Unsigned
	uint64_t dwChannelMask; // Used when channels are > 2 otherwise use 0x00000F
} WA_AudioFormat;

// Define a Simple Metadata Container
typedef struct TagWA_AudioMetadata
{
	wchar_t Title[30];
	wchar_t Artist[30];
	wchar_t Album[30];
	uint16_t Genre;
} WA_AudioMetadata;

// Define a Time Holder
typedef struct TagWA_TimeSpan
{
	uint32_t Hours;
	uint32_t Minutes;
	uint32_t Seconds;
	uint32_t Milliseconds;
} WA_TimeSpan;

// 0-Index Based
#define WA_PLUGINTYPE_INVALID	0x0U
#define WA_PLUGINTYPE_INPUT		0x1U
#define WA_PLUGINTYPE_OUTPUT	0x2U
#define WA_PLUGINTYPE_DSP		0x3U
#define WA_PLUGINTYPE_MAX WA_PLUGINTYPE_DSP

typedef struct TagWA_PluginHeader
{
	const uint16_t uPluginType;			// Define the Plugin Type (Input, Output or DSP)
	const uint16_t puVersion;			// Plugin Version
	const wchar_t* lpwDescription;		// Plugin Decription

	HWND hMainWindow;					// WinAudio Main Window Handle (Filled by WinAudio)
	bool bActive;
} WA_PluginHeader;


// Export this function from your plugin. Used with LoadLibrary and GetProcAddress
typedef void* WA_HMODULE;
typedef void* HCOOKIE; // Store a Pointer to Private Data

// Moved to WA_GEN_PluginLoader.c
// typedef WA_HMODULE* (*WA_Plugin_GetHeader)(void);

typedef struct TagWA_Plugin
{
	HMODULE hModule;
	uint16_t uPluginType;
	WA_HMODULE hVTable;
} WA_Plugin;

#endif
