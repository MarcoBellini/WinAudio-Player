#pragma once

#define WIN32_LEAN_AND_MEAN             // Escludere gli elementi usati raramente dalle intestazioni di Windows
// File di intestazione di Windows
#include <windows.h>


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "..\WinAudio\WA_GEN_Messages.h"
#include "..\WinAudio\WA_GEN_Types.h"
#include "..\WinAudio\WA_IN_Input.h"