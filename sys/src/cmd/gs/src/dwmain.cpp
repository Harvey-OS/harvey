/* Copyright (C) 1996-1998, Russell Lang.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/


// $Id: dwmain.cpp,v 1.1 2000/03/09 08:40:40 lpd Exp $
// Ghostscript DLL loader for Windows

#define STRICT
#include <windows.h>
#include <shellapi.h>
#ifndef __WIN32__
#include <toolhelp.h>
static BOOL dllfix(void);
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
extern "C" {
#include "gscdefs.h"
#define GSREVISION gs_revision
#include "gsdll.h"
}
#include "dwmain.h"
#include "dwdll.h"
#include "dwtext.h"
#include "dwimg.h"

#if defined(_MSC_VER) && defined(__WIN32__)
#define _export
#endif

/* public handles */
HINSTANCE ghInstance;

/* redirected stdio */
TextWindow tw;

const LPSTR szAppName = "Ghostscript";

#ifdef __WIN32__
const LPSTR szIniName = "GSWIN32.INI";
const char *szDllName = "GSDLL32.DLL";
#else
const LPSTR szIniName = "GSWIN.INI";
const char *szDllName = "GSDLL16.DLL";
#endif
const LPSTR szIniSection = "Text";

int dll_exit_status;

int FAR _export gsdll_callback(int message, char FAR *str, unsigned long count);

// the Ghostscript DLL class
gsdll_class gsdll;

char start_string[] = "systemdict /start get exec\n";

// program really starts at WinMain
int
new_main(int argc, char *argv[])
{
typedef char FAR * FARARGV_PTR;
FARARGV_PTR *far_argv;
int rc;

    // load DLL
    if (gsdll.load(ghInstance, szDllName, GSREVISION)) {
	char buf[256];
	gsdll.get_last_error(buf, sizeof(buf));
	tw.puts(buf);
	return 1;
    }

    
#ifdef __WIN32__
    far_argv = argv;
#else
    far_argv = new FARARGV_PTR[argc+1];
    if (!far_argv)
	return 1;
    for (int i = 0; i<argc; i++)
	far_argv[i] = argv[i];	// convert from near to far pointers
    far_argv[i+1] = NULL;
#endif

    // initialize the interpreter
    rc = gsdll.init(gsdll_callback, tw.get_handle(), argc, far_argv);
    if (rc == GSDLL_INIT_QUIT) {
        gsdll.unload();
	return 0;
    }
    if (rc) {
	char buf[256];
	gsdll.get_last_error(buf, sizeof(buf));
	tw.puts(buf);
        gsdll.unload();
#ifndef __WIN32__
	if (rc == GSDLL_INIT_IN_USE)
	    dllfix();   // offer option to unload NASTY!
#endif
	return rc;
    }

    // if (!batch)
    gsdll.execute(start_string, strlen(start_string));

#ifdef UNUSED
    int len;
    char line[256];
    do {
	len = tw.read_line(line, sizeof(line));
    } while ( len && !gsdll.execute(line, len) );
#endif
    
    gsdll.unload();

    return 0;
}


/* our exit handler */
/* also called from Text Window WM_CLOSE */
void win_exit(void)
{
    if (dll_exit_status) {
	/* display message box so error messages in text window can be read */
	char buf[80];
	if (IsIconic(tw.get_handle()))
	    ShowWindow(tw.get_handle(), SW_SHOWNORMAL);
	BringWindowToTop(tw.get_handle());  /* make text window visible */
	sprintf(buf, "Exit code %d\nSee text window for details",dll_exit_status);
	MessageBox((HWND)NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
    }
}

void
set_font(void)
{
    int fontsize;
    char fontname[256];
    char buf[32];

    // read ini file
    GetPrivateProfileString(szIniSection, "FontName", "Courier New", fontname, sizeof(fontname), szIniName);
    fontsize = GetPrivateProfileInt(szIniSection, "FontSize", 10, szIniName);

    // set font
    tw.font(fontname, fontsize); 

    // write ini file
    WritePrivateProfileString(szIniSection, "FontName", fontname, szIniName);
    sprintf(buf, "%d", fontsize);
    WritePrivateProfileString(szIniSection, "FontSize", buf, szIniName);
}

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
#define MAXCMDTOKENS 128
    // BC++ 4.5 will give us _argc and _argv[], but they will be 
    // incorrect if there is a space in the program name.
    // Provide our own parsing code to create argc and argv[]. 
    int argc;
    LPSTR argv[MAXCMDTOKENS];
    LPSTR p;
    char command[256];
    char *args;
    char *d, *e;
 
    /* copy the hInstance into a variable so it can be used */
    ghInstance = hInstance;

    if (hPrevInstance) {
	MessageBox((HWND)NULL,"Can't run twice", szAppName, 
		MB_ICONHAND | MB_OK);
	return FALSE;
    }

    // If called with "gswin32c.exe arg1 arg2"
    // lpszCmdLine returns:
    //    "arg1 arg2" if called with CreateProcess(NULL, command, ...)
    //    "arg2"      if called with CreateProcess(command, args, ...)
    // GetCommandLine() returns
    //    ""gswin32c.exe" arg1 arg2" 
    //          if called with CreateProcess(NULL, command, ...)
    //    "  arg1 arg2"      
    //          if called with CreateProcess(command, args, ...)
    // Consequently we must use GetCommandLine() 
    p = GetCommandLine();

    argc = 0;
    args = (char *)malloc(lstrlen(p)+1);
    if (args == (char *)NULL) {
	fprintf(stdout, "Insufficient memory in WinMain()\n");
	return 1;
    }
   
    // Parse command line handling quotes.
    d = args;
    while (*p) {
	// for each argument

	if (argc >= MAXCMDTOKENS - 1)
	    break;

        e = d;
        while ((*p) && (*p != ' ')) {
	    if (*p == '\042') {
		// Remove quotes, skipping over embedded spaces.
		// Doesn't handle embedded quotes.
		p++;
		while ((*p) && (*p != '\042'))
		    *d++ =*p++;
	    }
	    else 
		*d++ = *p;
	    if (*p)
		p++;
        }
	*d++ = '\0';
	argv[argc++] = e;

	while ((*p) && (*p == ' '))
	    p++;	// Skip over trailing spaces
    }
    argv[argc] = NULL;

    if (strlen(argv[0]) == 0) {
	GetModuleFileName(hInstance, command, sizeof(command)-1);
	argv[0] = command;
    }


    /* start up the text window */
    if (!hPrevInstance) {
	HICON hicon = LoadIcon(hInstance, (LPSTR)MAKEINTRESOURCE(GSTEXT_ICON));
	tw.register_class(hInstance, hicon);
    }
    set_font();
    tw.size(80, 80);
    tw.drag("(", ") run\r");

    // create the text window
    if (tw.create(szAppName, cmdShow))
	exit(1);

    // initialize for image windows
    ImageWindow::hwndtext = tw.get_handle();
    ImageWindow::hInstance = hInstance;

    dll_exit_status = new_main(argc, argv);

    win_exit();

    tw.destroy();

    return dll_exit_status;
}


int FAR _export
gsdll_callback(int message, char FAR *str, unsigned long count)
{
char buf[256];
ImageWindow *iw;
    switch (message) {
	case GSDLL_POLL:
	    {
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		    {
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		    }
	    }
	    // If text window closed then abort Ghostscript
	    if (!IsWindow(tw.get_handle()))
		return -1;
	    break;
	case GSDLL_STDIN:
	    return tw.read_line(str, count);
	case GSDLL_STDOUT:
	    if (str != (char *)NULL)
		tw.write_buf(str, count);
	    return count;
	case GSDLL_DEVICE:
#ifdef DEBUG
	    sprintf(buf,"Callback: DEVICE %p %s\n", (char *)str,
		count ? "open" : "close");
	    tw.puts(buf);
#endif
	    if (count) {
		iw = new ImageWindow;
		iw->open(str);
	    }
	    else {
		iw = FindImageWindow(str);
		iw->close();
	    }
	    break;
	case GSDLL_SYNC:
#ifdef DEBUG
	    sprintf(buf,"Callback: SYNC %p\n", (char *)str);
	    tw.puts(buf);
#endif
	    iw = FindImageWindow(str);
	    iw->sync();
	    break;
	case GSDLL_PAGE:
#ifdef DEBUG
	    sprintf(buf,"Callback: PAGE %p\n", (char *)str);
	    tw.puts(buf);
#endif
	    iw = FindImageWindow(str);
	    iw->page();
	    break;
	case GSDLL_SIZE:
#ifdef DEBUG
	    sprintf(buf,"Callback: SIZE %p width=%d height=%d\n", (char *)str,
		(int)(count & 0xffff), (int)((count>>16) & 0xffff) );
	    tw.puts(buf);
#endif
	    iw = FindImageWindow(str);
	    iw->size( (int)(count & 0xffff), (int)((count>>16) & 0xffff) );
	    break;
	default:
	    sprintf(buf,"Callback: Unknown message=%d\n",message);
	    tw.puts(buf);
	    break;
    }
    return 0;
}


#ifndef __WIN32__
// This is a hack to cope with a GPF occuring in either Ghostscript
// or a driver loaded by Ghostscript.  If a GPF occurs, gswin16.exe
// is unloaded, but not gsdll16.dll.
// Attempts to reload gswin16.exe will fail because gsdll16.dll
// is already in use.  If this occurs, this code will unload
// gsdll16.dll, EVEN IF SOMEONE ELSE IS USING IT.

// The DLL module name as it appears in the module definition file
#define GSDLLNAME "GSDLL16"
// These are the names of EXE modules that are known users
// of the Ghostscript DLL.
// If one of these is running, it is not safe to unload the DLL.
#define GSEXE		"GSWIN16"

typedef HMODULE (WINAPI *TOOLMFN)(MODULEENTRY FAR *, LPCSTR);

static BOOL
dllfix(void)
{
HMODULE hdll;			// module handle of DLL
HMODULE hgswin16;		// module handle of Ghostscript
MODULEENTRY me_dll;		 // to contain details about DLL module
MODULEENTRY me_gswin16;  // details about Ghostscript
HINSTANCE htool;		// module handle of TOOLHELP.DLL
TOOLMFN lpfnModuleFindName = NULL;
char buf[256];
BOOL err = FALSE;

    // load TOOLHELP.DLL
    htool = LoadLibrary("TOOLHELP.DLL");
    if ( htool <= HINSTANCE_ERROR )
	err = TRUE;

    // get address of toolhelp function
    if (!err) {
	lpfnModuleFindName = (TOOLMFN)GetProcAddress(htool, "ModuleFindName");
	err = !lpfnModuleFindName;
    }

    // find handle for DLL
    if (!err) {
	memset(&me_dll, 0, sizeof(me_dll));
	me_dll.dwSize = sizeof(me_dll);
	hdll = lpfnModuleFindName(&me_dll, GSDLLNAME);
    }

    // look for Ghostscript EXE module
    // This should be found because we are it
    if (!err) {
	memset(&me_gswin16, 0, sizeof(me_gswin16));
	me_gswin16.dwSize = sizeof(me_gswin16);
	hgswin16 = lpfnModuleFindName(&me_gswin16, GSEXE);
    }

#ifdef DEBUG
    // for debugging, show what we have found
    if (hdll) {
	wsprintf(buf, "Found %s\nModule Handle=%d\nModule Usage=%d\nModule Path=%s",
	me_dll.szModule, me_dll.hModule, me_dll.wcUsage, me_dll.szExePath);
	MessageBox((HWND)NULL, buf, szAppName, MB_OK);
    }
    if (hgswin16) {
	wsprintf(buf, "Found %s\nModule Handle=%d\nModule Usage=%d\nModule Path=%s",
	me_gswin16.szModule, me_gswin16.hModule, me_gswin16.wcUsage,
	me_gswin16.szExePath);
	MessageBox((HWND)NULL, buf, szAppName, MB_OK);
    }
#endif

    // if DLL loaded and Ghostscript is not running...
    if (!err &&
	hdll && 		// Ghostscript DLL loaded
	(hgswin16 && me_gswin16.wcUsage == 1) // Only one copy of Ghostscript EXE
	) {
	wsprintf(buf, "%s is loaded but does not appear to be in use by Ghostscript. Unload it?", GSDLLNAME);
	if (MessageBox((HWND)NULL, buf, szAppName, MB_YESNO | MB_ICONHAND) == IDYES)
	    // If DLL is really in use, we about to cause a disaster
	    while (GetModuleUsage(hdll))
		FreeLibrary(hdll);
    }

    if (htool)
	FreeLibrary(htool);
    return err;
}
#endif

