#pragma once

#define WIN32_LEAN_AND_MEAN             // Escludere gli elementi usati raramente dalle intestazioni di Windows
// File di intestazione di Windows
#include <windows.h>


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Wasapi Objects
#define CINTERFACE
#define COBJMACROS
#include <initguid.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>

#include "..\WinAudio\WA_GEN_Messages.h"
#include "..\WinAudio\WA_GEN_Types.h"
#include "..\WinAudio\WA_IN_Input.h"
#include "..\WinAudio\WA_DSP_Effect.h"
#include "..\WinAudio\WA_OUT_Output.h"
