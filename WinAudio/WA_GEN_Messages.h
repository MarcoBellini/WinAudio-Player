#ifndef WA_GEN_MESSAGES_H
#define WA_GEN_MESSAGES_H


// Define lParam of SendMessage Function
#define WM_WA_MSG	(WM_APP + 0x0001)


/*
	+++ Get or Set Current Status +++


	dwStatus = SendMessage(MainWindowHwnd, WM_WA_MSG, MSG_STATUS, MSG_STATUS_xxx);

	MSG_STATUS_PLAY = Send Play Command (Start to Play Current Opened File or Unpause)
	MSG_STATUS_PAUSE = Send Pause Command (Pause Media)
	MSG_STATUS_STOP = Send Stop Command (Stop and Close Input and Output)
	MSG_STATUS_GET = Get Current Status

	This function returns always the current status
*/
#define MSG_STATUS						0x0001
#define MSG_STATUS_PLAY					0x000A
#define MSG_STATUS_PAUSE				0x000B
#define MSG_STATUS_STOP					0x000C
#define MSG_STATUS_GET					0x000D



/*

	+++ Open a new file. Clear Playlist, add to it and prepare to a MSG_STATUS_PLAY command +++

	wchar_t MyFilePath[MAX_PATH];

	...
	Fill MyFilePath with valid Path file
	...

	lpCpyStruct.dwData = MSG_OPENFILE;
	lpCpyStruct.lpData = MyFilePath;
	lpCpyStruct.cbData = SizeOf(MyFilePath);

	dwResult = SendMessage(MainWindowHwnd, WM_COPYDATA, MainWindowHwnd, lpCpyStruct);

	dwResult = WA_OK No errors
	dwResult > 0 See Error Codes below
*/
#define MSG_OPENFILE					0x0002




/*

	+++ Enqueue a new file to playlist +++

	wchar_t MyFilePath[MAX_PATH];

	...
	Fill MyFilePath with valid Path file
	...

	lpCpyStruct.dwData = MSG_ENQUEUEFILE;
	lpCpyStruct.lpData = MyFilePath;
	lpCpyStruct.cbData = SizeOf(MyFilePath);

	dwResult = SendMessage(MainWindowHwnd, WM_COPYDATA, MainWindowHwnd, lpCpyStruct);

	dwResult = WA_OK No errors
	dwResult > 0 See Error Codes below
*/
#define MSG_ENQUEUEFILE					0x0003


/*
	+++ Notify We Reched End Of Stream +++

	dwResult = SendMessage(MainWindowHwnd, WM_WA_MSG, MSG_NOTIFYENDOFSTREAM, 0);

	dwResult = WA_OK No errors
	dwResult > 0 See Error Codes below


	Used by Output plugins to notify main window when End Of Stream Occours.
	Main Window doesn't know when Input is EOF and output stops.

	Output must always send this Message!


*/
#define MSG_NOTIFYENDOFSTREAM			0x0004




/*
	+++ Set or Get Volume +++

	Volume Min Value = 0
	Volume Max Value = 255

	## Set Volume ##

	dwResult  = SendMessage(MainWindowHwnd, WM_WA_MSG, MSG_SETVOLUME, (LPARAM) uNewVolumeValue) 

	dwResult = WA_OK No errors
	dwResult > 0 See Error Codes below

	## Get Volume ##

	dwResult  = SendMessage(MainWindowHwnd, WM_WA_MSG, MSG_GETVOLUME, 0)

	dwResult = Current Volume Value
*/
#define MSG_SETVOLUME					0x0005
#define MSG_GETVOLUME					0x0006



// Define Current Status
#define WA_STATUS_PLAY					0x0001
#define WA_STATUS_PAUSE					0x0002
#define WA_STATUS_STOP					0x0003

// Define Errors
#define WA_OK							0
#define WA_ERROR_FILENOTFOUND			0x0001
#define WA_ERROR_OUTPUTNOTREADY			0x0002
#define WA_ERROR_MALLOCERROR			0x0003
#define WA_ERROR_BADPTR					0x0004
#define WA_ERROR_FILENOTSUPPORTED		0x0005
#define WA_ERROR_STREAMNOTSEEKABLE		0x0006
#define WA_ERROR_ENDOFFILE				0x0007
#define WA_ERROR_BUFFEROVERFLOW			0x0008
#define WA_ERROR_BADFORMAT				0x0009
#define WA_ERROR_INPUTOUTPUTNOTFOUND	0x000A
#define WA_ERROR_TOOMUCHPLUGINS			0x000B
#define WA_ERROR_BADPARAM				0x000C
#define WA_ERROR_BADMSG					0x000D
#define WA_ERROR_FAIL					0x000E
#define WA_ERROR_OUTPUTTHREADNOTREADY	0x000F


// Define FFT Size
#define WA_FFT_512						512
#define WA_FFT_1024						1024
#define WA_FFT_2048						2048
#define WA_FFT_4096						4096

#endif
