// https://github.com/ysc3839/win32-darkmode

#include "stdafx.h"
#include <sdkddkver.h>
#include "WA_UI_DarkMode.h"

#define USE_DWMAPI 1

#if USE_DWMAPI
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

typedef enum TagIMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
} IMMERSIVE_HC_CACHE_MODE;

// 1903 18362
typedef enum TagPreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
} PreferredAppMode;

typedef enum TagWINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
} WINDOWCOMPOSITIONATTRIB;


typedef enum _ACCENT_STATE
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4, // RS4 1803
	ACCENT_ENABLE_HOSTBACKDROP = 5, // RS5 1809
	ACCENT_INVALID_STATE = 6
} ACCENT_STATE;

// This enum is defined in dwmapi.h header on sdk >= 22621
#if (WDK_NTDDI_VERSION < NTDDI_WIN10_NI)
typedef enum _DWM_SYSTEMBACKDROP_TYPE
{
	DWMSBT_AUTO = 0,

	/// <summary>
	/// no backdrop
	/// </summary>
	DWMSBT_NONE = 1,

	/// <summary>
	/// Use tinted blurred wallpaper backdrop (Mica)
	/// </summary>
	DWMSBT_MAINWINDOW = 2,

	/// <summary>
	/// Use Acrylic backdrop
	/// </summary>
	DWMSBT_TRANSIENTWINDOW = 3,

	/// <summary>
	/// Use blurred wallpaper backdrop
	/// </summary>
	DWMSBT_TABBEDWINDOW = 4

} DWM_SYSTEMBACKDROP_TYPE;
#endif



typedef struct _ACCENT_POLICY
{
	ACCENT_STATE AccentState;
	DWORD AccentFlags;
	DWORD GradientColor;
	DWORD AnimationId;
} ACCENT_POLICY;

typedef struct TagWINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;


typedef VOID(WINAPI *fnRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build);
typedef BOOL(WINAPI *fnSetWindowCompositionAttribute)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
typedef BOOL(WINAPI *fnShouldAppsUseDarkMode)(VOID); // ordinal 132
typedef BOOL(WINAPI*fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow); // ordinal 133
typedef BOOL(WINAPI *fnAllowDarkModeForApp)(BOOL allow); // ordinal 135, in 1809
typedef VOID(WINAPI *fnFlushMenuThemes)(VOID); // ordinal 136
typedef VOID(WINAPI *fnRefreshImmersiveColorPolicyState)(VOID); // ordinal 104
typedef BOOL(WINAPI *fnIsDarkModeAllowedForWindow)(HWND hWnd); // ordinal 137
typedef BOOL(WINAPI *fnGetIsImmersiveColorUsingHighContrast)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
typedef HTHEME(WINAPI *fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
typedef BOOL(WINAPI *fnShouldSystemUseDarkMode)(VOID); // ordinal 138
typedef PreferredAppMode(WINAPI *fnSetPreferredAppMode)(PreferredAppMode appMode); // ordinal 135, in 1903
typedef BOOL(WINAPI *fnIsDarkModeAllowedForApp)(VOID); // ordinal 139


fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = NULL;
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = NULL;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = NULL;
fnAllowDarkModeForApp _AllowDarkModeForApp = NULL;
fnFlushMenuThemes _FlushMenuThemes = NULL;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = NULL;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = NULL;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = NULL;
fnOpenNcThemeData _OpenNcThemeData = NULL;
// 1903 18362
fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = NULL;
fnSetPreferredAppMode _SetPreferredAppMode = NULL;


bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
bool g_MicaSupported = false;
DWORD g_buildNumber = 0;
HMODULE hComctl = NULL;
HMODULE hUxtheme = NULL;

// This portion contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.
static PIMAGE_DELAYLOAD_DESCRIPTOR DataDirectoryFromModuleBase(void* moduleBase, size_t entryID)
{
	PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)moduleBase;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)moduleBase + dosHdr->e_lfanew);
	PIMAGE_DATA_DIRECTORY dataDir = ntHdr->OptionalHeader.DataDirectory;

	return (PIMAGE_DELAYLOAD_DESCRIPTOR)((ULONG_PTR)moduleBase + dataDir[entryID].VirtualAddress);
}

static PIMAGE_THUNK_DATA FindAddressByOrdinal(void* moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}

	return NULL;
}

static PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, uint16_t ordinal)
{
	
	PIMAGE_DELAYLOAD_DESCRIPTOR imports = DataDirectoryFromModuleBase(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);

	for (; imports->DllNameRVA; ++imports)
	{
		
		if (_stricmp((LPCSTR)((ULONG_PTR)moduleBase + imports->DllNameRVA), dllName) != 0)
			continue;

		PIMAGE_THUNK_DATA impName = (PIMAGE_THUNK_DATA)((ULONG_PTR)moduleBase + imports->ImportNameTableRVA);
		PIMAGE_THUNK_DATA impAddr = (PIMAGE_THUNK_DATA)((ULONG_PTR)moduleBase + imports->ImportAddressTableRVA);


		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}

	return NULL;
}

HTHEME WINAPI MyOpenThemeData(HWND hWnd, LPCWSTR classList)
{
	if (wcscmp(classList, L"ScrollBar") == 0)
	{
		hWnd = NULL;
		classList = L"Explorer::ScrollBar";
	}

	return _OpenNcThemeData(hWnd, classList);
}

static void FixDarkScrollBar()
{
	hComctl = LoadLibraryExW(L"comctl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	if (hComctl)
	{
		PIMAGE_THUNK_DATA addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
			{
				addr->u1.Function = (ULONG_PTR)((fnOpenNcThemeData)(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
	}
}

static inline bool CheckBuildNumber(DWORD buildNumber)
{
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041 || // 2004
		buildNumber == 19042 || // 20H2
		buildNumber == 19043 || // 21H1
		buildNumber == 19044 || // 21H2
		(buildNumber > 19044 && buildNumber < 22000) || // Windows 10 any version > 21H2 
		buildNumber >= 22000);  // Windows 11 builds
}

static inline bool IsMicaSupported(DWORD buildNumber)
{
	return (buildNumber >= 22621);
}


void DarkMode_Init(bool bFixScrollbars)
{
	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = NULL;
	HMODULE hNTDll = GetModuleHandleW(L"ntdll.dll");
		
	if (hNTDll)
	{
		RtlGetNtVersionNumbers = (fnRtlGetNtVersionNumbers)GetProcAddress(hNTDll, "RtlGetNtVersionNumbers");
	}

	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;

		if (major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber))
		{
			hUxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

			if (hUxtheme)
			{
				_OpenNcThemeData = (fnOpenNcThemeData)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
				_RefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = (fnGetIsImmersiveColorUsingHighContrast)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_ShouldAppsUseDarkMode = (fnShouldAppsUseDarkMode)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				_AllowDarkModeForWindow = (fnAllowDarkModeForWindow)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

				FARPROC ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));

				if (g_buildNumber < 18362)
					_AllowDarkModeForApp = (fnAllowDarkModeForApp)(ord135);
				else
					_SetPreferredAppMode = (fnSetPreferredAppMode)(ord135);

				_FlushMenuThemes = (fnFlushMenuThemes)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = (fnIsDarkModeAllowedForWindow)(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

				HMODULE hUser32 = GetModuleHandleW(L"user32.dll");

				if (hUser32)
				{
					_SetWindowCompositionAttribute = (fnSetWindowCompositionAttribute)(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));
				}


				if (_OpenNcThemeData &&
					_RefreshImmersiveColorPolicyState &&
					_ShouldAppsUseDarkMode &&
					_AllowDarkModeForWindow &&
					(_AllowDarkModeForApp || _SetPreferredAppMode) &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					g_darkModeSupported = true;

					DarkMode_AllowDarkModeForApp(true);
					_RefreshImmersiveColorPolicyState();

					g_darkModeEnabled = _ShouldAppsUseDarkMode() && !DarkMode_IsHighContrast();

					g_MicaSupported = IsMicaSupported(g_buildNumber) && !DarkMode_IsHighContrast();

					if(bFixScrollbars)
						FixDarkScrollBar();

				}
			}
		}
	}
}

void DarkMode_Close()
{

	if (hComctl)
		FreeLibrary(hComctl);

	if (hUxtheme)
		FreeLibrary(hUxtheme);

	hComctl = NULL;
	hUxtheme = NULL;

	g_darkModeSupported = false;
	g_darkModeEnabled = false;
	g_MicaSupported = false; // EDIT: 10/09/2022
}


bool DarkMode_IsSupported()
{
	return g_darkModeSupported;
}


bool DarkMode_IsEnabled()
{
	return g_darkModeEnabled;
}


void DarkMode_AllowDarkModeForApp(bool bAllow)
{
	if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(bAllow);
	else if (_SetPreferredAppMode)
		_SetPreferredAppMode(bAllow ? AllowDark : Default);
}


bool DarkMode_IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	bool is = false;

	if ((message != WM_SETTINGCHANGE) && (message != 0))
		return false;

	if (lParam && CompareStringOrdinal((LPCWCH)(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}


void DarkMode_RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow(hWnd) &&
		_ShouldAppsUseDarkMode() &&
		!DarkMode_IsHighContrast())
	{
		dark = TRUE;
	}

#if USE_DWMAPI
	if (SUCCEEDED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark))))
		return;

	DwmSetWindowAttribute(hWnd, 19, &dark, sizeof(dark));
#else
	if (g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", (HANDLE)((INT_PTR)(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
#endif
}


bool DarkMode_AllowDarkModeForWindow(HWND hWnd, bool bAllow)
{
	if (g_darkModeSupported)
		return _AllowDarkModeForWindow(hWnd, bAllow);


	return false;
}

bool DarkMode_ApplyMica(HWND hWnd)
{
	HRESULT hr;

	if ((g_MicaSupported) && IsWindow(hWnd))
	{
		int value = DWMSBT_MAINWINDOW;
		hr = DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));

		// Set margins, extending the bottom margin
		// see https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmextendframeintoclientarea
		// MARGINS margins = { 0,0,0,25 };
		// hr = DwmExtendFrameIntoClientArea(hWnd, &margins);

		return SUCCEEDED(hr);

	}

	return false;
}


bool DarkMode_IsHighContrast()
{
	HIGHCONTRASTW highContrast = { sizeof(highContrast) };
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
		return highContrast.dwFlags & HCF_HIGHCONTRASTON;
	return false;
}


void DarkMode_HandleThemeChange()
{
	g_darkModeEnabled = _ShouldAppsUseDarkMode() && !DarkMode_IsHighContrast();
}
