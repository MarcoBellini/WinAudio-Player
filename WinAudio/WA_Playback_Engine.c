
#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_PluginLoader.h"
#include "WA_Playback_Engine.h"
#include "WA_GEN_Playlist.h"
#include "Globals2.h"

static wchar_t WA_Playback_Engine_SupportedFileString[] = L"All Supported Files\0";

/// <summary>
/// Return a Pointer to a WA_Input Plugin that supports input file or return NULL on fail
/// </summary>
/// <param name="lpwPath">Local path of file</param>
/// <returns>Pointer to a WA_Input* struct or NULL on fail</returns>
static WA_Input* WA_Playback_Engine_Find_Decoder(const wchar_t* lpwPath)
{
    WA_Input* pIn = NULL;
    wchar_t* lpwPluginExtension;
    wchar_t* lpwExtension;

    lpwExtension = PathFindExtension(lpwPath);

    for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_INPUT)
        {
            pIn = (WA_Input*)Plugins.pPluginList[i].hVTable;
            lpwPluginExtension = pIn->lpwFilterExtensions;

            if (wcsstr(lpwPluginExtension, lpwExtension))
            {
                return pIn;
            }
        }
    }

    return NULL;
}

/// <summary>
/// Try to open a local file
/// </summary>
/// <param name="lpwPath">Path to a local file</param>
/// <returns>True on success</returns>
bool WA_Playback_Engine_OpenFile(const wchar_t* lpwPath)
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    WA_Effect* pEffect = NULL;
    uint32_t uResult;

    // Stop Before Close
    if (Globals2.dwCurrentStatus != MW_STOPPED)
        WA_Playback_Engine_Stop();

    // and Close file
    if (Globals2.bFileIsOpen)
        WA_Playback_Engine_CloseFile();

    if (!PathFileExists(lpwPath))
    {
        MessageBox(Globals2.hMainWindow, L"File not Found", L"WinAudio Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    pIn = WA_Playback_Engine_Find_Decoder(lpwPath);

    if (!pIn)
    {
        MessageBox(Globals2.hMainWindow, L"File not Supported", L"WinAudio Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }


    uResult = pIn->WA_Input_Open(pIn, lpwPath);

    if (uResult != WA_OK)
    {
        MessageBox(Globals2.hMainWindow, L"Input Plugin cannot open this file", L"WinAudio Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    // Assign Current Input (Assign NULL on Close)
    Globals2.pInput = pIn;

    // Get Active Output and DSP
    pOut = Globals2.pOutput;
    pEffect = Globals2.pEffect;

    // Assign Plugins to Output
    pOut->pIn = pIn;
    pOut->pEffect = pEffect;



    // Try to open output
    uResult = pOut->WA_Output_Open(pOut, &Globals2.uOutputLatency);

    if (uResult != WA_OK)
    {
        WA_Playback_Engine_CloseFile();
        MessageBox(Globals2.hMainWindow, L"Cannot open Output", L"WinAudio Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }



    Globals2.bFileIsOpen = true;
    Globals2.dwCurrentStatus = MW_STOPPED;
   

    return true;
}

/// <summary>
/// Close a file opened with WA_Playback_Engine_OpenFile function
/// </summary>
/// <returns>True on Success</returns>
bool WA_Playback_Engine_CloseFile()
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    WA_Effect* pEffect = NULL;

    if (!Globals2.bFileIsOpen)
        return false;

    // Stop Before Close
    if (Globals2.dwCurrentStatus != MW_STOPPED)
        WA_Playback_Engine_Stop();

    pIn = Globals2.pInput;
    pOut = Globals2.pOutput;
    pEffect = Globals2.pEffect;
    
    // Close Plugins
    pOut->WA_Output_Close(pOut);
    pIn->WA_Input_Close(pIn);
    
    Globals2.bFileIsOpen = false;
    Globals2.dwCurrentStatus = MW_STOPPED;

    return true;
}

uint32_t WA_Playback_Engine_GetExtFilter(COMDLG_FILTERSPEC **pFilter)
{
    uint32_t uInputCount = 0U;
    uint32_t uIndex;

    // Count Input plugins
    for (uint32_t i = 0U; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_INPUT)
            uInputCount++;
    }

    if (uInputCount == 0)
        return false;

    // Use index = 0 to aggregate all supported types
    uInputCount++;

    (*pFilter) = (COMDLG_FILTERSPEC*) calloc(uInputCount, sizeof(COMDLG_FILTERSPEC));

    if (!(*pFilter))
        return false;

    uIndex = 1U;

    for (uint32_t i = 0U; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_INPUT)
        {
            WA_Input* pIn = (WA_Input*) Plugins.pPluginList[i].hVTable;

            (*pFilter)[uIndex].pszName = pIn->lpwFilterName;
            (*pFilter)[uIndex].pszSpec = pIn->lpwFilterExtensions;

            uIndex++;
        }
           
    }

    (*pFilter)[0].pszName = WA_Playback_Engine_SupportedFileString;
    (*pFilter)[0].pszSpec = calloc(MAX_PATH, sizeof(wchar_t));

    // Free Resources on Fail
    if (!(*pFilter)[0].pszSpec)
    {
        free((*pFilter));
        (*pFilter) = NULL;
        return 0U;
    }

    for (uint32_t i = 1U; i < (uInputCount); i++)
    {
        wcscat_s((*pFilter)[0].pszSpec, MAX_PATH, (*pFilter)[i].pszSpec);

        // Add comma to every extensions
        if(i != (uInputCount - 1U))
            wcscat_s((*pFilter)[0].pszSpec, MAX_PATH, L";");
    }       
    

    return uInputCount;
}


bool WA_Playback_Engine_Play(void)
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_STOPPED)
        return false;

    pIn = Globals2.pInput;
    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Play(pOut);

    if (uResult != WA_OK)
        return false;

    Globals2.dwCurrentStatus = MW_PLAYING;

    return true;
}

bool WA_Playback_Engine_Pause(void)
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    pIn = Globals2.pInput;
    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Pause(pOut);

    if (uResult != WA_OK)
        return false;

    Globals2.dwCurrentStatus = MW_PAUSING;

    return true;
}

bool WA_Playback_Engine_Resume(void)
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PAUSING)
        return false;

    pIn = Globals2.pInput;
    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Resume(pOut);

    if (uResult != WA_OK)
        return false;

    Globals2.dwCurrentStatus = MW_PLAYING;

    return true;
}

bool WA_Playback_Engine_Stop(void)
{
    WA_Input* pIn = NULL;
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if ((Globals2.dwCurrentStatus != MW_PLAYING) && 
        (Globals2.dwCurrentStatus != MW_PAUSING))
        return false;

    pIn = Globals2.pInput;
    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Stop(pOut);

    if (uResult != WA_OK)
        return false;

    // Seek to Begin on Stop
    uResult = pIn->WA_Input_Seek(pIn, 0);

    if (uResult != WA_OK)
        return false;

    Globals2.dwCurrentStatus = MW_STOPPED;

    return true;
}

bool WA_Playback_Engine_Seek(uint64_t uNewPositionMs)
{
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Seek(pOut, uNewPositionMs);

    return (uResult == WA_OK) ? true : false;
}

bool WA_Playback_Engine_Get_Position(uint64_t *uCurrentPositionMs)
{
    WA_Output* pOut = NULL;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    pOut = Globals2.pOutput;

    uResult = pOut->WA_Output_Get_Position(pOut, uCurrentPositionMs);

    return (uResult == WA_OK) ? true : false;
}

bool WA_Playback_Engine_Get_Duration(uint64_t* uCurrentDurationMs)
{
    WA_Input* pIn = NULL;

    if (!Globals2.bFileIsOpen)
        return false;

    pIn = Globals2.pInput;


    (*uCurrentDurationMs) = pIn->WA_Input_Duration(pIn);

    return true;
}

bool WA_Playback_Engine_Set_Volume(uint8_t uNewVolume)
{
    WA_Output *pOut;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    pOut = Globals2.pOutput;

    if (!pOut)
        return false;

    uResult = pOut->WA_Output_Set_Volume(pOut, uNewVolume);

    return (uResult == WA_OK) ? true : false;
}

bool WA_Playback_Engine_Get_Volume(uint8_t* puNewVolume)
{
    WA_Output* pOut;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;

    pOut = Globals2.pOutput;

    if (!pOut)
        return false;

    uResult = pOut->WA_Output_Get_Volume(pOut, puNewVolume);

    return (uResult == WA_OK) ? true : false;
}


/// <summary>
/// Get Current Playing Audio Data
/// </summary>
/// <param name="pBuffer">Pointer to a int8_t array with a <paramref name="uBufferLen"></paramref> length</param>
/// <param name="uBufferLen">Length of a buffer pointed by <paramref name="pBuffer"></paramref> param</param>
/// <returns>True on Success</returns>
bool WA_Playback_Engine_Get_Buffer(int8_t* pBuffer, uint32_t uBufferLen)
{
    if (!Globals2.bFileIsOpen)
        return false;
}

/// <summary>
/// Get Current Stream Audio Format
/// </summary>
/// <param name="pFormat"></param>
/// <returns></returns>
bool WA_Playback_Engine_Get_Current_Format(WA_AudioFormat* pFormat)
{
    WA_Input* pIn;

    // Check for Bad Pointer
    if (!pFormat)
        return false;

    if (!Globals2.bFileIsOpen)
        return false;

    pIn = Globals2.pInput;

    if (!pIn)
        return false;

    return (pIn->WA_Input_GetFormat(pIn, pFormat) == WA_OK) ? true : false;
}

/// <summary>
/// Load Plugins and Initialize all of them. 
/// Set Output to first plugin found (Modified later when loading settings).
/// </summary>
/// <returns>True on success</returns>
bool WA_Playback_Engine_New(void)
{

    // Load and Initialize Plugins
    if (WA_GEN_PluginLoader_Load(Globals2.hMainWindow) != WA_OK)
    {
        MessageBox(NULL, L"No Input or Output Plugins Found, cannot initialize the player", L"WinAudio Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (WA_GEN_PluginLoader_Call_New() != WA_OK)
    {
        WA_GEN_PluginLoader_Unload();
        MessageBox(NULL, L"Unable to initialize any plugins", L"WinAudio Error", MB_OK | MB_ICONERROR);
        return false; 
    }

    Globals2.pInput = NULL;
    Globals2.pOutput = NULL;
    Globals2.pEffect = NULL;

    // Set first output plugin as Default (modified when loading settings)
    for (uint32_t i = 0; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_OUTPUT)
        {
            Globals2.pOutput = (WA_Output*)Plugins.pPluginList[i].hVTable;
            break;
        }
    }

    return true;
}

/// <summary>
/// Call Delete on all Plugins and Unload
/// </summary>
/// <returns>True on success</returns>
bool WA_Playback_Engine_Delete(void)
{
    // Close Plugins
    WA_GEN_PluginLoader_Call_Delete();
    WA_GEN_PluginLoader_Unload();

    return true;
}