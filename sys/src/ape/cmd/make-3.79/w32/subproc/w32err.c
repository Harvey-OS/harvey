#include <windows.h>
#include "w32err.h"

/*
 * Description: the windows32 version of perror()
 *
 * Returns:  a pointer to a static error
 *
 * Notes/Dependencies:  I got this from 
 *      comp.os.ms-windows.programmer.win32
 */
char * 
map_windows32_error_to_string (DWORD ercode) {
/* __declspec (thread) necessary if you will use multiple threads */
__declspec (thread) static char szMessageBuffer[128];
 
	/* Fill message buffer with a default message in 
	 * case FormatMessage fails 
	 */
    wsprintf (szMessageBuffer, "Error %ld", ercode);

	/*
	 *  Special code for winsock error handling.
	 */
	if (ercode > WSABASEERR) {
		HMODULE hModule = GetModuleHandle("wsock32");
		if (hModule != NULL) {
			FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
				hModule,
				ercode,
				LANG_NEUTRAL,
				szMessageBuffer,
				sizeof(szMessageBuffer),
				NULL);
			FreeLibrary(hModule);
		} 
	} else {
		/*
		 *  Default system message handling
		 */
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  ercode,
                  LANG_NEUTRAL,
                  szMessageBuffer,
                  sizeof(szMessageBuffer),
                  NULL);
	}
    return szMessageBuffer;
}
 
