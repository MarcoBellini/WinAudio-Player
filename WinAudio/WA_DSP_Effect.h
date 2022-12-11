#ifndef WA_DSP_EFFECT_H
#define WA_DSP_EFFECT_H

// Forward Reference
typedef struct TagWA_Effect WA_Effect;


struct TagWA_Effect
{
	WA_PluginHeader Header;			// Store Common Plugins Info

	bool (*WA_Effect_New)(WA_Effect* This);		// Called at WM_CREATE. Return true to Enable Plugin
	void (*WA_Effect_Delete)(WA_Effect* This);    // Called at WM_DESTROY 

	uint32_t (*WA_Effect_UpdateFormat)(WA_Effect* This, const WA_AudioFormat* pAudioFormat); // Update Wave Format for effect. Return WA_OK or Error Code

	uint32_t (*WA_Effect_Process)(WA_Effect* This, int8_t *pBuffer, uint32_t uBufferLen, uint32_t* puProcessedBytes); // Process Audio Samples. Return WA_OK or Error Code
	// pBuffer = Raw data to be processed
	// uBufferLen = Valid Data Pointed by pBuffer
	// puProcessedBytes = Number of valid bytes stored in pBuffer
	
	void (*WA_Effect_ConfigDialog)(WA_Effect* This, HWND hParent); // Show Plugin Configuration Dialog

	HCOOKIE hPluginData;			// Store Plugin Private Data Here
};

#endif
