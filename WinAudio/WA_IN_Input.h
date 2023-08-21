#ifndef WA_IN_INPUT_H
#define WA_IN_INPUT_H


// Forward Reference
typedef struct TagWA_Input WA_Input;


struct TagWA_Input
{
	WA_PluginHeader Header;			// Store Common Plugins Info

	wchar_t* lpwFilterName;			// Plugin Supported Filter Name "MPEG Audio\0"
	wchar_t* lpwFilterExtensions;	// Plugin Supported Extensions "*.mp3;*.mp2;*mp1\0"

	bool (*WA_Input_New)(WA_Input *This);		// Called at WM_CREATE. Return true to Enable Plugin
	void (*WA_Input_Delete)(WA_Input* This);  // Called at WM_DESTROY 

	uint32_t (*WA_Input_Open)(WA_Input* This, const wchar_t* lpwFilePath); // Open a new File or URL. Return WA_OK or Error Code
	void (*WA_Input_Close)(WA_Input* This);	// Close File

	bool (*WA_Input_IsStreamSeekable)(WA_Input* This); // Get if a stream is Seekable (URL maybe cannot be seekable)

	uint64_t(*WA_Input_Duration)(WA_Input* This); // Get Duration in Ms
	uint64_t(*WA_Input_Position)(WA_Input* This); // Get Position in Ms

	uint32_t(*WA_Input_Seek)(WA_Input* This, uint64_t uNewPosition); // Seek to a New Position in Ms. Offset from Begin. Return WA_OK or Error Code

	uint32_t(*WA_Input_Read)(WA_Input* This, int8_t* pBuffer, uint32_t uBufferLen, uint32_t* puReadedBytes);  // Read Decoded data
	// pBuffer = Allocated byte buffer to contains at least uBufferLen bytes
	// uBufferLen = Max bytes to Read
	// puReadedBytes = Readed Bytes
	// Return WA_OK or Error Code. Return WA_ERROR_ENDOFFILE at the end of file


	void (*WA_Input_ConfigDialog)(WA_Input* This, HWND hParent); // Show Plugin Configuration Dialog
	void (*WA_Input_PluginDialog)(WA_Input* This, HWND hParent); // Show Plugin Custom Dialog (ES. Tag Editing, or CD Selection)

	uint32_t (*WA_Input_GetMetadata)(WA_Input* This, WA_AudioMetadata* pMetadata); // Get Audio Metadata for current opened file. Return WA_OK or Error Code
	uint32_t (*WA_Input_GetFormat)(WA_Input* This, WA_AudioFormat* pFormat);		   // Get Audio Wave Format for current opened file.. Return WA_OK or Error Code

	uint32_t(*WA_Input_GetFileInfo) (WA_Input* This, const wchar_t *lpwFilePath, WA_AudioFormat* pFormat, WA_AudioMetadata* pMetadata, uint64_t* puDuration); // Get WaveFormat and Metadata for a generic supported file. Return WA_OK or Error Code

	HCOOKIE hPluginData;	// Store Plugin Private Data Here
};



#endif
