#pragma once

#define WIN32_LEAN_AND_MEAN             // Escludere gli elementi usati raramente dalle intestazioni di Windows
// File di intestazione di Windows
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <memory>
#include <CommCtrl.h>

extern "C" 
{
#include "..\WinAudio\WA_GEN_Messages.h"
#include "..\WinAudio\WA_GEN_Types.h"
#include "..\WinAudio\WA_DSP_Effect.h"
}