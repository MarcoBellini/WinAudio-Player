#ifndef STDAFX_H
#define STDAFX_H

#ifndef UNICODE
#define UNICODE
#endif 

#define WIN32_LEAN_AND_MEAN

// Target to Windows 10
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

// Add visual styles (Only VC Compiler)
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")


#include <Windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <Ole2.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <math.h>
#include <dwmapi.h>
#include "d2d1_586.h"



#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


#endif