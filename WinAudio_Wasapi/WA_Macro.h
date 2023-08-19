#ifndef WA_MACRO_H
#define WA_MACRO_H

// Error codes
#define WA_WASAPI_OK					0x0000
#define WA_WASAPI_ENDOFFILE				0x0001
#define WA_WASAPI_INPUTERROR			0x0002
#define WA_WASAPI_MALLOCERROR			0x0003
#define WA_WASAPI_NOBYTESTOWRITE		0x0004
#define WA_WASAPI_DEVICEWRITEERROR		0x0005

// Use in WaitSingleObject or Multiple Object Timeout
#define WA_WASAPI_MAX_WAIT_TIME_MS		1000

// Use High Resolution time to read playing position (100ns order)
#define WA_WASAPI_USE_HIGH_RES_TIMER 0

// Output buffer length in Ms
#define WA_WASAPI_DEFAULT_LATENCY_MS 400  

#endif
