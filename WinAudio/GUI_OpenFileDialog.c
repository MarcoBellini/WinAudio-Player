
#include "stdafx.h"


#include "GUI_OpenFileDialog.h"


/*
Remarks: Using Shell File Dialog increment memory usage from
         7-8 MB to 15-16MB and this value increment every time
         a file dialog is shown. Bug of Windows 10??

*/

/// <summary>
/// Create a File Dialog
/// </summary>
/// <param name="hOwnerHandle">= Main Window Handle</param>
/// <param name="lpwsPath">= Path to file</param>
bool Shell_SingleFileOpenDialog(HWND hOwnerHandle, LPWSTR lpwsPath)
{
    IFileOpenDialog* pFileOpen;
    HRESULT hr;
    bool bResult = false;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (LPVOID*)(&pFileOpen));

    if (SUCCEEDED(hr))
    {

        // Prepare File Dialog
        COMDLG_FILTERSPEC Filter;

        Filter.pszName = L"Supported Files";
        Filter.pszSpec = L"*.wav; *.mp3"; // TODO: Make this filte dynamic

        IFileOpenDialog_SetFileTypes(pFileOpen, 1U, &Filter);
        IFileOpenDialog_SetFileTypeIndex(pFileOpen, 1U);

        IFileOpenDialog_SetTitle(pFileOpen, L"Open an audio file...");

        // Show the Open dialog box.
        hr = IFileOpenDialog_Show(pFileOpen, hOwnerHandle);
       

        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {

            IShellItem* pItem;
            hr = IFileOpenDialog_GetResult(pFileOpen , &pItem);

            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = IShellItem_GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                // Display the file name to the user.
                if (SUCCEEDED(hr))
                {
                    // Copy the path to return var
                    wcscpy_s(lpwsPath, MAX_PATH, pszFilePath);
                    CoTaskMemFree(pszFilePath);

                    bResult = true;
                }
                IShellItem_Release(pItem);
                pItem = NULL;
            }
        }
        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    return bResult;
}


bool Shell_MultipleFilesOpenDialog(HWND hOwnerHandle, ShellFilesArray** pFilesArray, DWORD* dwCount)
{

    IFileOpenDialog* pFileOpen;
    HRESULT hr;
    bool bResult = false;

    (*dwCount) = 0;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (LPVOID*)(&pFileOpen));

    if (SUCCEEDED(hr))
    {

        // Prepare File Dialog
        COMDLG_FILTERSPEC Filter;

        Filter.pszName = L"Supported Files";
        Filter.pszSpec = L"*.wav; *.mp3"; // TODO: Make this filte dynamic

        IFileOpenDialog_SetFileTypes(pFileOpen, 1U, &Filter);
        IFileOpenDialog_SetFileTypeIndex(pFileOpen, 1U);

        IFileOpenDialog_SetTitle(pFileOpen, L"Select files...");

        // Allow Multiple Selections
        IFileOpenDialog_SetOptions(pFileOpen, FOS_ALLOWMULTISELECT);

        // Show the Open dialog box.
        hr = IFileOpenDialog_Show(pFileOpen, hOwnerHandle);


        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {

            IShellItemArray* pItemsArray;

            hr = IFileOpenDialog_GetResults(pFileOpen, &pItemsArray);

            if SUCCEEDED(hr)
            {
                DWORD dwFilesCount;

                // Get The number of Selected Files
                hr = IShellItemArray_GetCount(pItemsArray, &dwFilesCount);

                if (SUCCEEDED(hr) && (dwFilesCount > 0))
                {
                    (*pFilesArray) = (ShellFilesArray*)malloc(dwFilesCount * sizeof(ShellFilesArray));

                    (*dwCount) = dwFilesCount;
                    
                    if ((*pFilesArray))
                    {
                        IShellItem* pItem;

                        // Get The Path of Each Selected File
                        for (uint32_t i = 0; i < dwFilesCount; i++)
                        {
                            hr = IShellItemArray_GetItemAt(pItemsArray, i, &pItem);

                            if SUCCEEDED(hr)
                            {

                                PWSTR pszFilePath;
                                hr = IShellItem_GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                                // Display the file name to the user.
                                if (SUCCEEDED(hr))
                                {
                                    // Copy the path to ShellFilesArray
                                    wcscpy_s((*pFilesArray)[i].lpwsPath, MAX_PATH, pszFilePath);
                                    CoTaskMemFree(pszFilePath);   

                                    bResult = true;
                                }

                                // Release Resources
                                IShellItem_Release(pItem);
                            }

                        }
                    }

                }

                IShellItemArray_Release(pItemsArray);
                pItemsArray = NULL;
            }
           
        }

        IFileOpenDialog_Release(pFileOpen);
        pFileOpen = NULL;
    }

    return bResult;

}