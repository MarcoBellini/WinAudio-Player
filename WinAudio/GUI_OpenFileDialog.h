#ifndef GUI_OPENFILEDIALOG
#define GUI_OPENFILEDIALOG

typedef struct tagShellFilesArray
{
	wchar_t lpwsPath[MAX_PATH];
} ShellFilesArray;

// Show a File Dialog (Filter applyed in code) 
bool Shell_SingleFileOpenDialog(HWND hOwnerHandle, LPWSTR lpwsPath);

// Show a Multiple Files Dialog (after use, call free to pFilesArray pointer)
bool Shell_MultipleFilesOpenDialog(HWND hOwnerHandle, ShellFilesArray** pFilesArray, DWORD * dwCount);

#endif
