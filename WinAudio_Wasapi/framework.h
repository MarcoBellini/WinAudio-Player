#pragma once

#define WIN32_LEAN_AND_MEAN        
#include <windows.h>

// Standard Libs
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Wasapi Objects
#define CINTERFACE
#define COBJMACROS
#include <initguid.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>
#include <avrt.h>

// Header to needed to build outputs
#include "..\WinAudio\WA_GEN_Messages.h"
#include "..\WinAudio\WA_GEN_Types.h"
#include "..\WinAudio\WA_IN_Input.h"
#include "..\WinAudio\WA_DSP_Effect.h"
#include "..\WinAudio\WA_OUT_Output.h"
