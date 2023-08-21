#ifndef WA_OUT_OUTPUT_H
#define WA_OUT_OUTPUT_H


// Forward Reference
typedef struct TagWA_Output WA_Output;


struct TagWA_Output 
{
	WA_PluginHeader Header;			// Store Common Plugins Info

	WA_Input* pIn;					// Store Current Input Plugin (With file Opened and ready to read PCM Data)
	WA_Effect* pEffect;				// Store Current DSP Plugin (Prepared to process PCM Data readed from input)


	bool (*WA_Output_New)(WA_Output* This);		// Called at WM_CREATE. Return true to Enable Plugin
	void (*WA_Output_Delete)(WA_Output* This);    // Called at WM_DESTROY 


	uint32_t(*WA_Output_Open)(WA_Output* This, uint32_t* puBufferLatency); // Open Output. 
	// puBufferLatency is the Length of the buffer. If Null No Value Is Returned
	// Return WA_OK or Error Code. Return WA_ERROR_ENDOFFILE at the end of file

	void (*WA_Output_Close)(WA_Output* This);    // Close Output


	uint32_t(*WA_Output_Play)(WA_Output* This);	//Return WA_OK or Error Code
	uint32_t(*WA_Output_Pause)(WA_Output* This);	//Return WA_OK or Error Code
	uint32_t(*WA_Output_Resume)(WA_Output* This);	//Return WA_OK or Error Code
	uint32_t(*WA_Output_Stop)(WA_Output* This);	//Return WA_OK or Error Code

	uint32_t(*WA_Output_Seek)(WA_Output* This,uint64_t uNewPositionMs);	//Seek to a new Position in Ms. Return WA_OK or Error Code
	uint32_t(*WA_Output_Get_Position)(WA_Output* This, uint64_t *uPositionMs); // Get current playback Position in Ms. Return WA_OK or Error Code

	uint32_t(*WA_Output_Set_Volume)(WA_Output* This, uint8_t uNewVolume);	// Set new Volume Value (0-255)
	uint32_t(*WA_Output_Get_Volume)(WA_Output* This, uint8_t *puVolume);		// Get Volume Value

	uint32_t(*WA_Output_Get_BufferData)(WA_Output* This, int8_t* pBuffer, uint32_t uBufferLen); // Get Current on Playing Data. Return WA_OK or Error Code
	// uBufferLen must be at least puBufferLatency size or the function return BUFFEROVERFLOW error

	uint32_t (*WA_Output_Enable_Process_DSP)(WA_Output* This, bool bEnable); // Enable or Disable DSP Processing. 
	// bEnable = false -> Disable Output to Call DSP Process() function
	// bEnable = true -> Enable Output to Call DSP Process() function
	 
	void (*WA_Output_ConfigDialog)(WA_Input* This, HWND hParent); // Show Plugin Configuration Dialog

	HCOOKIE hPluginData;			// Store Plugin Private Data Here
};

#endif
