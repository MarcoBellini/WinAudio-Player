
#include "stdafx.h"
#include "WA_UI_DarkMode.h"
#include "WA_UI_ColorPolicy.h"
#include "WA_GEN_Types.h"
#include "WA_IN_Input.h"
#include "WA_DSP_Effect.h"
#include "WA_OUT_Output.h"
#include "WA_GEN_Messages.h"
#include "WA_GEN_PluginLoader.h"
#include "WA_GEN_Playback_Engine.h"
#include "WA_GEN_Playlist.h"
#include "WA_UI_Visualizations.h"
#include "Globals2.h"

const static wchar_t WA_Playback_Engine_SupportedFileString[] = L"All Supported Files\0";

/// <summary>
/// Set Active Output and Active Effect from Settings
/// </summary>
static void WA_Playback_Prepare_Output_Effect_From_Settings()
{
    WA_Effect* pEffect = NULL;
    WA_Output* pOut = NULL;
    bool bSkipOutput = false;
    bool bSkipEffect = false;

    for (uint32_t i = 0U; i < Plugins.uPluginsCount; i++)
    {

        switch (Plugins.pPluginList[i].uPluginType)
        {
        case WA_PLUGINTYPE_OUTPUT:
        {
            if (bSkipOutput) continue;

            pOut = (WA_Output*) Plugins.pPluginList[i].hVTable;

            // As Default setting use first Output if pActiveOutputDescr is not set or is invalid
            if(!Globals2.pOutput)
                Globals2.pOutput = pOut;

            if (wcscmp(Settings2.pActiveOutputDescr, pOut->Header.lpwDescription) == 0)
            {
                bSkipOutput = true;
                Globals2.pOutput = pOut;
            }

            break;
        }
        case WA_PLUGINTYPE_DSP:
        {
            if (bSkipEffect) continue;

            pEffect = (WA_Effect*)Plugins.pPluginList[i].hVTable;

            if (wcscmp(Settings2.pActiveEffectDescr, pEffect->Header.lpwDescription) == 0)
            {
                bSkipEffect = true;
                Globals2.pEffect = pEffect;
            }

            break;
        }
        }
    }

}

/// <summary>
/// Return a Pointer to a WA_Input Plugin that supports input file or return NULL on fail
/// </summary>
/// <param name="lpwPath">Local path of file</param>
/// <returns>Pointer to a WA_Input* struct or NULL on fail</returns>
WA_Input* WA_Playback_Engine_Find_Decoder(const wchar_t* lpwPath)
{
    WA_Input* pIn = NULL;
    wchar_t* lpwPluginExtension;
    wchar_t* lpwExtension;

    lpwExtension = PathFindExtension(lpwPath);

    for (uint32_t i = 0U; i < Plugins.uPluginsCount; i++)
    {
      
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_INPUT)
        {
            pIn = (WA_Input*)Plugins.pPluginList[i].hVTable;

            // Skip Inactive Plugins
            if (!pIn->Header.bActive)
                continue;

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
        TaskDialog(Globals2.hMainWindow, Globals2.CurrentProcessHInstance, L"WinAudio Error", L"Unable to open input file", L"The file format is not found, try another file path", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }

    pIn = WA_Playback_Engine_Find_Decoder(lpwPath);

    if (!pIn)
    {       
        TaskDialog(Globals2.hMainWindow, Globals2.CurrentProcessHInstance, L"WinAudio Error", L"Unable to open input file", L"The file format is not supported, try to add a proper plugin to support it", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }


    uResult = pIn->WA_Input_Open(pIn, lpwPath);

    if (uResult != WA_OK)
    {  
        TaskDialog(Globals2.hMainWindow, Globals2.CurrentProcessHInstance, L"WinAudio Error", L"Unable to open input file", L"The file format is not supported, or the input is not able to decode it", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }

    // Assign Current Input (Assign NULL on Close)
    Globals2.pInput = pIn;

    // Get Active Output and DSP
    WA_Playback_Prepare_Output_Effect_From_Settings();
    pOut = Globals2.pOutput;
    pEffect = Globals2.pEffect;

    // Assign Plugins to Output
    pOut->pIn = pIn;
    pOut->pEffect = pEffect;

    if (pEffect)
    {
        pOut->WA_Output_Enable_Process_DSP(pOut, true);
    }
    else
    {
        pOut->WA_Output_Enable_Process_DSP(pOut, false);
    }



    // Try to open output
    uResult = pOut->WA_Output_Open(pOut, &Globals2.uOutputLatency);

    if (uResult != WA_OK)
    {
        pIn->WA_Input_Close(pIn);         
        TaskDialog(Globals2.hMainWindow, Globals2.CurrentProcessHInstance, L"WinAudio Error", L"Unable to open output", L"The file format is not supported, or the output is not active", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }

    // Update Active Effect WFX
    if (pEffect)
    {
        WA_AudioFormat Format;

        if (pIn->WA_Input_GetFormat(pIn, &Format) == WA_OK)
            pEffect->WA_Effect_UpdateFormat(pEffect, &Format);
        else
            pOut->WA_Output_Enable_Process_DSP(pOut, false); // Disable DSP on Error
       
        
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

/// <summary>
/// Get Filters to use in Open/Save file dialogs
/// </summary>
/// <param name="pFilter">Pointer to a COMDLG_FILTERSPEC pointer. Use "free(param)" to free resource after use</param>
/// <param name="pszTemp">Temp string to compose filter</param>
/// <param name="pszTempLen">Length in characters of pszTemp param</param>
/// <returns> >0 on success, 0 on Fail</returns>
uint32_t WA_Playback_Engine_GetExtFilter(COMDLG_FILTERSPEC **pFilter, wchar_t *pszTemp, uint32_t pszTempLen)
{
    uint32_t uInputCount = 0U;
    uint32_t uIndex;

    if ((pszTempLen == 0) || (!pszTemp))
        return 0;

    // Count Input plugins
    for (uint32_t i = 0U; i < Plugins.uPluginsCount; i++)
    {
        if (Plugins.pPluginList[i].uPluginType == WA_PLUGINTYPE_INPUT)
            uInputCount++;
    }

    if (uInputCount == 0)
        return 0;

    // Use index = 0 to aggregate all supported types
    uInputCount++;

    (*pFilter) = (COMDLG_FILTERSPEC*) calloc(uInputCount, sizeof(COMDLG_FILTERSPEC));

    if (!(*pFilter))
        return 0;

    uIndex = 1U;
    
    // Create a filter row for every plugins
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
    ZeroMemory(pszTemp, sizeof(wchar_t) * pszTempLen);

    for (uint32_t i = 1U; i < (uInputCount); i++)
    {
        wcscat_s(pszTemp, pszTempLen, (*pFilter)[i].pszSpec);

        // Add comma to every extensions
        if(i != (uInputCount - 1U))
            wcscat_s(pszTemp, pszTempLen, L";");
    }       
    
    (*pFilter)[0].pszSpec = pszTemp;

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
    WA_Output* pOut;
    uint32_t uResult;

    if (!Globals2.bFileIsOpen)
        return false;

    if (Globals2.dwCurrentStatus != MW_PLAYING)
        return false;
   
    if (uBufferLen == 0U)
        return false;

    if(!pBuffer)
        return false;
    

    pOut = Globals2.pOutput;

    if (!pOut)
        return false;

    uResult = pOut->WA_Output_Get_BufferData(pOut, pBuffer, uBufferLen);

    return (uResult == WA_OK) ? true : false;
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
/// Return if a File is Supported
/// </summary>
/// <param name="lpwPath">Path of a File</param>
/// <returns>True if file is supported</returns>
bool WA_Playback_Engine_IsFileSupported(const wchar_t* lpwPath)
{
    WA_Input* pIn = NULL;

    if (!PathFileExists(lpwPath))
        return false;

    pIn = WA_Playback_Engine_Find_Decoder(lpwPath);

    return (pIn) ? true : false;
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