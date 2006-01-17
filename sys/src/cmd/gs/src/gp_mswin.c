/* Copyright (C) 1992, 2000-2003 artofcode LLC.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gp_mswin.c,v 1.25 2005/03/04 21:58:55 ghostgum Exp $ */
/*
 * Microsoft Windows platform support for Ghostscript.
 *
 * Original version by Russell Lang and Maurice Castro with help from
 * Programming Windows, 2nd Ed., Charles Petzold, Microsoft Press;
 * initially created from gp_dosfb.c and gp_itbc.c 5th June 1992.
 */

/* Modified for Win32 & Microsoft C/C++ 8.0 32-Bit, 26.Okt.1994 */
/* by Friedrich Nowak                                           */

/* Original EXE and GSview specific code removed */
/* DLL version must now be used under MS-Windows */
/* Russell Lang 16 March 1996 */

#include "stdio_.h"
#include "string_.h"
#include "memory_.h"
#include "pipe_.h"
#include <stdlib.h>
#include <stdarg.h>
#include "ctype_.h"
#include <io.h>
#include "malloc_.h"
#include <fcntl.h>
#include <signal.h>
#include "gx.h"
#include "gp.h"
#include "gpcheck.h"
#include "gpmisc.h"
#include "gserrors.h"
#include "gsexit.h"

#include "windows_.h"
#include <shellapi.h>
#include <winspool.h>
#include "gp_mswin.h"

/* Library routines not declared in a standard header */
extern char *getenv(const char *);

/* limits */
#define MAXSTR 255

/* GLOBAL VARIABLE that needs to be removed */
char win_prntmp[MAXSTR];	/* filename of PRN temporary file */

/* GLOBAL VARIABLES - initialised at DLL load time */
HINSTANCE phInstance;
BOOL is_win32s = FALSE;

const LPSTR szAppName = "Ghostscript";
private int is_printer(const char *name);


/* ====== Generic platform procedures ====== */

/* ------ Initialization/termination (from gp_itbc.c) ------ */

/* Do platform-dependent initialization. */
void
gp_init(void)
{
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

/* Exit the program. */
void
gp_do_exit(int exit_status)
{
    exit(exit_status);
}

/* ------ Persistent data cache ------*/

/* insert a buffer under a (type, key) pair */
int gp_cache_insert(int type, byte *key, int keylen, void *buffer, int buflen)
{
    /* not yet implemented */
    return 0;
}

/* look up a (type, key) in the cache */
int gp_cache_query(int type, byte* key, int keylen, void **buffer,
    gp_cache_alloc alloc, void *userdata)
{
    /* not yet implemented */
    return -1;
}

/* ------ Printer accessing ------ */

/* Forward references */
private int gp_printfile(const char *, const char *);

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char fname[gp_file_name_sizeof], int binary_mode)
{
    if (is_printer(fname)) {
	FILE *pfile;

	/* Open a scratch file, which we will send to the */
	/* actual printer in gp_close_printer. */
	pfile = gp_open_scratch_file(gp_scratch_file_name_prefix,
				     win_prntmp, "wb");
	return pfile;
    } else if (fname[0] == '|') 	/* pipe */
	return popen(fname + 1, (binary_mode ? "wb" : "w"));
    else
	return fopen(fname, (binary_mode ? "wb" : "w"));
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE * pfile, const char *fname)
{
    fclose(pfile);
    if (!is_printer(fname))
	return;			/* a file, not a printer */

    gp_printfile(win_prntmp, fname);
    unlink(win_prntmp);
}


/* Dialog box to select printer port */
DLGRETURN CALLBACK
SpoolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSTR entry;

    switch (message) {
	case WM_INITDIALOG:
	    entry = (LPSTR) lParam;
	    while (*entry) {
		SendDlgItemMessage(hDlg, SPOOL_PORT, LB_ADDSTRING, 0, (LPARAM) entry);
		entry += lstrlen(entry) + 1;
	    }
	    SendDlgItemMessage(hDlg, SPOOL_PORT, LB_SETCURSEL, 0, (LPARAM) 0);
	    return TRUE;
	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case SPOOL_PORT:
		    if (HIWORD(wParam) == LBN_DBLCLK)
			PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
		    return FALSE;
		case IDOK:
		    EndDialog(hDlg, 1 + (int)SendDlgItemMessage(hDlg, SPOOL_PORT, LB_GETCURSEL, 0, 0L));
		    return TRUE;
		case IDCANCEL:
		    EndDialog(hDlg, 0);
		    return TRUE;
	    }
    }
    return FALSE;
}

/* return TRUE if queue looks like a valid printer name */
int
is_spool(const char *queue)
{
    char *prefix = "\\\\spool";	/* 7 characters long */
    int i;

    for (i = 0; i < 7; i++) {
	if (prefix[i] == '\\') {
	    if ((*queue != '\\') && (*queue != '/'))
		return FALSE;
	} else if (tolower(*queue) != prefix[i])
	    return FALSE;
	queue++;
    }
    if (*queue && (*queue != '\\') && (*queue != '/'))
	return FALSE;
    return TRUE;
}


private int
is_printer(const char *name)
{
    char buf[128];

    /* is printer if no name given */
    if (strlen(name) == 0)
	return TRUE;

    /*  is printer if name appears in win.ini [ports] section */
    GetProfileString("ports", name, "XYZ", buf, sizeof(buf));
    if (strlen(name) == 0 || strcmp(buf, "XYZ"))
	return TRUE;

    /* is printer if name prefixed by \\spool\ */
    if (is_spool(name))
	return TRUE;

    return FALSE;
}


/******************************************************************/
/* Print File to port or queue */
/* port==NULL means prompt for port or queue with dialog box */

/* This is messy because Microsoft changed the spooler interface */
/* between Windows 3.1 and Windows 95/NT */
/* and didn't provide the spooler interface in Win32s */

/* Win95, WinNT: Use OpenPrinter, WritePrinter etc. */
private int gp_printfile_win32(const char *filename, char *port);

/* Win32s: Pass to Win16 spooler via gs16spl.exe */
private int gp_printfile_gs16spl(const char *filename, const char *port);

/*
 * Valid values for pmport are:
 *   ""
 *      action: WinNT and Win95 use default queue, Win32s prompts for port
 *   "LPT1:" (or other port that appears in win.ini [ports]
 *      action: start gs16spl.exe to print to the port
 *   "\\spool\printer name"
 *      action: send to printer using WritePrinter (WinNT and Win95).
 *      action: translate to port name using win.ini [Devices]
 *              then use gs16spl.exe (Win32s).
 *   "\\spool"
 *      action: prompt for queue name then send to printer using 
 *              WritePrinter (WinNT and Win95).
 *      action: prompt for port then use gs16spl.exe (Win32s).
 */
/* Print File */
private int
gp_printfile(const char *filename, const char *pmport)
{
    /* treat WinNT and Win95 differently to Win32s */
    if (!is_win32s) {
	if (strlen(pmport) == 0) {	/* get default printer */
	    char buf[256];
	    char *p;

	    /* WinNT stores default printer in registry and win.ini */
	    /* Win95 stores default printer in win.ini */
	    GetProfileString("windows", "device", "", buf, sizeof(buf));
	    if ((p = strchr(buf, ',')) != NULL)
		*p = '\0';
	    return gp_printfile_win32(filename, buf);
	} else if (is_spool(pmport)) {
	    if (strlen(pmport) >= 8)
		return gp_printfile_win32(filename, (char *)pmport + 8);
	    else
		return gp_printfile_win32(filename, (char *)NULL);
	} else
	    return gp_printfile_gs16spl(filename, pmport);
    } else {
	/* Win32s */
	if (is_spool(pmport)) {
	    if (strlen(pmport) >= 8) {
		/* extract port name from win.ini */
		char driverbuf[256];
		char *output;

		GetProfileString("Devices", pmport + 8, "", driverbuf, sizeof(driverbuf));
		strtok(driverbuf, ",");
		output = strtok(NULL, ",");
		return gp_printfile_gs16spl(filename, output);
	    } else
		return gp_printfile_gs16spl(filename, (char *)NULL);
	} else
	    return gp_printfile_gs16spl(filename, pmport);
    }
}

#define PRINT_BUF_SIZE 16384u
#define PORT_BUF_SIZE 4096

char *
get_queues(void)
{
    int i;
    DWORD count, needed;
    PRINTER_INFO_1 *prinfo;
    char *enumbuffer;
    char *buffer;
    char *p;

    /* enumerate all available printers */
    EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL, NULL, 1, NULL, 0, &needed, &count);
    if (needed == 0) {
	/* no printers */
	enumbuffer = malloc(4);
	if (enumbuffer == (char *)NULL)
	    return NULL;
	memset(enumbuffer, 0, 4);
	return enumbuffer;
    }
    enumbuffer = malloc(needed);
    if (enumbuffer == (char *)NULL)
	return NULL;
    if (!EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL, NULL, 1, (LPBYTE) enumbuffer, needed, &needed, &count)) {
	char buf[256];

	free(enumbuffer);
	sprintf(buf, "EnumPrinters() failed, error code = %d", GetLastError());
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	return NULL;
    }
    prinfo = (PRINTER_INFO_1 *) enumbuffer;
    if ((buffer = malloc(PORT_BUF_SIZE)) == (char *)NULL) {
	free(enumbuffer);
	return NULL;
    }
    /* copy printer names to single buffer */
    p = buffer;
    for (i = 0; i < count; i++) {
	if (strlen(prinfo[i].pName) + 1 < (PORT_BUF_SIZE - (p - buffer))) {
	    strcpy(p, prinfo[i].pName);
	    p += strlen(p) + 1;
	}
    }
    *p = '\0';			/* double null at end */
    free(enumbuffer);
    return buffer;
}


char *
get_ports(void)
{
    char *buffer;

    if (!is_win32s)
	return get_queues();

    if ((buffer = malloc(PORT_BUF_SIZE)) == (char *)NULL)
	return NULL;
    GetProfileString("ports", NULL, "", buffer, PORT_BUF_SIZE);
    return buffer;
}

/* return TRUE if queuename available */
/* return FALSE if cancelled or error */
/* if queue non-NULL, use as suggested queue */
BOOL
get_queuename(char *portname, const char *queue)
{
    char *buffer;
    char *p;
    int i, iport;

    buffer = get_queues();
    if (buffer == NULL)
	return FALSE;
    if ((queue == (char *)NULL) || (strlen(queue) == 0)) {
	/* select a queue */
	iport = DialogBoxParam(phInstance, "QueueDlgBox", (HWND) NULL, SpoolDlgProc, (LPARAM) buffer);
	if (!iport) {
	    free(buffer);
	    return FALSE;
	}
	p = buffer;
	for (i = 1; i < iport && strlen(p) != 0; i++)
	    p += lstrlen(p) + 1;
	/* prepend \\spool\ which is used by Ghostscript to distinguish */
	/* real files from queues */
	strcpy(portname, "\\\\spool\\");
	strcat(portname, p);
    } else {
	strcpy(portname, "\\\\spool\\");
	strcat(portname, queue);
    }

    free(buffer);
    return TRUE;
}

/* return TRUE if portname available */
/* return FALSE if cancelled or error */
/* if port non-NULL, use as suggested port */
BOOL
get_portname(char *portname, const char *port)
{
    char *buffer;
    char *p;
    int i, iport;
    char filename[MAXSTR];

    buffer = get_ports();
    if (buffer == NULL)
	return FALSE;
    if ((port == (char *)NULL) || (strlen(port) == 0)) {
	if (buffer == (char *)NULL)
	    return FALSE;
	/* select a port */
	iport = DialogBoxParam(phInstance, "SpoolDlgBox", (HWND) NULL, SpoolDlgProc, (LPARAM) buffer);
	if (!iport) {
	    free(buffer);
	    return FALSE;
	}
	p = buffer;
	for (i = 1; i < iport && strlen(p) != 0; i++)
	    p += lstrlen(p) + 1;
	strcpy(portname, p);
    } else
	strcpy(portname, port);

    if (strlen(portname) == 0)
	return FALSE;
    if (strcmp(portname, "FILE:") == 0) {
	OPENFILENAME ofn;

	filename[0] = '\0';
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = (HWND) NULL;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.Flags = OFN_PATHMUSTEXIST;
	if (!GetSaveFileName(&ofn)) {
	    free(buffer);
	    return FALSE;
	}
	strcpy(portname, filename);
    }
    free(buffer);
    return TRUE;
}


/* True Win32 method, using OpenPrinter, WritePrinter etc. */
private int
gp_printfile_win32(const char *filename, char *port)
{
    DWORD count;
    char *buffer;
    char portname[MAXSTR];
    FILE *f;
    HANDLE printer;
    DOC_INFO_1 di;
    DWORD written;

    if (!get_queuename(portname, port))
	return FALSE;
    port = portname + 8;	/* skip over \\spool\ */

    if ((buffer = malloc(PRINT_BUF_SIZE)) == (char *)NULL)
	return FALSE;

    if ((f = fopen(filename, "rb")) == (FILE *) NULL) {
	free(buffer);
	return FALSE;
    }
    /* open a printer */
    if (!OpenPrinter(port, &printer, NULL)) {
	char buf[256];

	sprintf(buf, "OpenPrinter() failed for \042%s\042, error code = %d", port, GetLastError());
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	free(buffer);
	return FALSE;
    }
    /* from here until ClosePrinter, should AbortPrinter on error */

    di.pDocName = szAppName;
    di.pOutputFile = NULL;
    di.pDatatype = "RAW";	/* for available types see EnumPrintProcessorDatatypes */
    if (!StartDocPrinter(printer, 1, (LPBYTE) & di)) {
	char buf[256];

	sprintf(buf, "StartDocPrinter() failed, error code = %d", GetLastError());
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	AbortPrinter(printer);
	free(buffer);
	return FALSE;
    }
    /* copy file to printer */
    while ((count = fread(buffer, 1, PRINT_BUF_SIZE, f)) != 0) {
	if (!WritePrinter(printer, (LPVOID) buffer, count, &written)) {
	    free(buffer);
	    fclose(f);
	    AbortPrinter(printer);
	    return FALSE;
	}
    }
    fclose(f);
    free(buffer);

    if (!EndDocPrinter(printer)) {
	char buf[256];

	sprintf(buf, "EndDocPrinter() failed, error code = %d", GetLastError());
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	AbortPrinter(printer);
	return FALSE;
    }
    if (!ClosePrinter(printer)) {
	char buf[256];

	sprintf(buf, "ClosePrinter() failed, error code = %d", GetLastError());
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	return FALSE;
    }
    return TRUE;
}


/* Start a 16-bit application gs16spl.exe to print a file */
/* Intended for Win32s where 16-bit spooler functions are not available */
/* and Win32 spooler functions are not implemented. */
int
gp_printfile_gs16spl(const char *filename, const char *port)
{
/* Get printer port list from win.ini */
    char portname[MAXSTR];
    HINSTANCE hinst;
    char command[MAXSTR];
    char *p;
    HWND hwndspl;

    if (!get_portname(portname, port))
	return FALSE;

    /* get path to EXE - same as DLL */
    GetModuleFileName(phInstance, command, sizeof(command));
    if ((p = strrchr(command, '\\')) != (char *)NULL)
	p++;
    else
	p = command;
    *p = '\0';
    sprintf(command + strlen(command), "gs16spl.exe %s %s",
	    portname, filename);

    hinst = (HINSTANCE) WinExec(command, SW_SHOWNORMAL);
    if (hinst < (HINSTANCE) HINSTANCE_ERROR) {
	char buf[MAXSTR];

	sprintf(buf, "Can't run: %s", command);
	MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	return FALSE;
    }
    hwndspl = FindWindow(NULL, "GS Win32s/Win16 spooler");

    while (IsWindow(hwndspl)) {
	gp_check_interrupts(NULL);
    }

    return 0;
}


/******************************************************************/
/* MS Windows has popen and pclose in stdio.h, but under different names.
 * Unfortunately MSVC5 and 6 have a broken implementation of _popen, 
 * so we use own.  Our implementation only supports mode "wb".
 */
FILE *mswin_popen(const char *cmd, const char *mode)
{
    SECURITY_ATTRIBUTES saAttr;
    STARTUPINFO siStartInfo;
    PROCESS_INFORMATION piProcInfo;
    HANDLE hPipeTemp = INVALID_HANDLE_VALUE;
    HANDLE hChildStdinRd = INVALID_HANDLE_VALUE;
    HANDLE hChildStdinWr = INVALID_HANDLE_VALUE;
    HANDLE hChildStdoutWr = INVALID_HANDLE_VALUE;
    HANDLE hChildStderrWr = INVALID_HANDLE_VALUE;
    HANDLE hProcess = GetCurrentProcess();
    int handle = 0;
    char *command = NULL;
    FILE *pipe = NULL;

    if (strcmp(mode, "wb") != 0)
	return NULL;

    /* Set the bInheritHandle flag so pipe handles are inherited. */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    /* Create anonymous inheritable pipes for STDIN for child. 
     * First create a noninheritable duplicate handle of our end of 
     * the pipe, then close the inheritable handle.
     */
    if (handle == 0)
        if (!CreatePipe(&hChildStdinRd, &hPipeTemp, &saAttr, 0))
	    handle = -1;
    if (handle == 0) {
	if (!DuplicateHandle(hProcess, hPipeTemp, 
	    hProcess, &hChildStdinWr, 0, FALSE /* not inherited */,
	    DUPLICATE_SAME_ACCESS))
	    handle = -1;
        CloseHandle(hPipeTemp);
    }
    /* Create inheritable duplicate handles for our stdout/err */
    if (handle == 0)
	if (!DuplicateHandle(hProcess, GetStdHandle(STD_OUTPUT_HANDLE),
	    hProcess, &hChildStdoutWr, 0, TRUE /* inherited */,
	    DUPLICATE_SAME_ACCESS))
	    handle = -1;
    if (handle == 0)
        if (!DuplicateHandle(hProcess, GetStdHandle(STD_ERROR_HANDLE),
            hProcess, &hChildStderrWr, 0, TRUE /* inherited */, 
	    DUPLICATE_SAME_ACCESS))
	    handle = -1;

    /* Set up members of STARTUPINFO structure. */
    memset(&siStartInfo, 0, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.dwFlags = STARTF_USESTDHANDLES;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdError = hChildStderrWr;

    if (handle == 0) {
	command = (char *)malloc(strlen(cmd)+1);
	if (command)
	    strcpy(command, cmd);
	else
	    handle = -1;
    }

    if (handle == 0)
	if (!CreateProcess(NULL,
	    command,  	   /* command line                       */
	    NULL,          /* process security attributes        */
	    NULL,          /* primary thread security attributes */
	    TRUE,          /* handles are inherited              */
	    0,             /* creation flags                     */
	    NULL,          /* environment                        */
	    NULL,          /* use parent's current directory     */
	    &siStartInfo,  /* STARTUPINFO pointer                */
	    &piProcInfo))  /* receives PROCESS_INFORMATION  */ 
	{
	    handle = -1;
	}
	else {
	    CloseHandle(piProcInfo.hProcess);
	    CloseHandle(piProcInfo.hThread);
	    handle = _open_osfhandle((long)hChildStdinWr, 0);
	}

    if (hChildStdinRd != INVALID_HANDLE_VALUE)
	CloseHandle(hChildStdinRd);	/* close our copy */
    if (hChildStdoutWr != INVALID_HANDLE_VALUE)
	CloseHandle(hChildStdoutWr);	/* close our copy */
    if (hChildStderrWr != INVALID_HANDLE_VALUE)
	CloseHandle(hChildStderrWr);	/* close our copy */
    if (command)
	free(command);

    if (handle < 0) {
	if (hChildStdinWr != INVALID_HANDLE_VALUE)
	    CloseHandle(hChildStdinWr);
    }
    else {
	pipe = _fdopen(handle, "wb");
	if (pipe == NULL)
	    _close(handle);
    }
    return pipe;
}


/* ------ File naming and accessing ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{
    UINT n;
    DWORD l;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    int fd = -1;
    FILE *f = NULL;
    char sTempDir[_MAX_PATH];
    char sTempFileName[_MAX_PATH];

    memset(fname, 0, gp_file_name_sizeof);
    if (!gp_file_name_is_absolute(prefix, strlen(prefix))) {
	int plen = sizeof(sTempDir);

	if (gp_gettmpdir(sTempDir, &plen) != 0)
	    l = GetTempPath(sizeof(sTempDir), sTempDir);
	else
	    l = strlen(sTempDir);
    } else {
	strncpy(sTempDir, prefix, sizeof(sTempDir));
	prefix = "";
	l = strlen(sTempDir);
    }
    /* Fix the trailing terminator so GetTempFileName doesn't get confused */
    if (sTempDir[l-1] == '/')
	sTempDir[l-1] = '\\';		/* What Windoze prefers */

    if (l <= sizeof(sTempDir)) {
	n = GetTempFileName(sTempDir, prefix, 0, sTempFileName);
	if (n == 0) {
	    /* If 'prefix' is not a directory, it is a path prefix. */
	    int l = strlen(sTempDir), i;

	    for (i = l - 1; i > 0; i--) {
		uint slen = gs_file_name_check_separator(sTempDir + i, l, sTempDir + l);

		if (slen > 0) {
		    sTempDir[i] = 0;   
		    i += slen;
		    break;
		}
	    }
	    if (i > 0)
		n = GetTempFileName(sTempDir, sTempDir + i, 0, sTempFileName);
	}
	if (n != 0) {
	    hfile = CreateFile(sTempFileName, 
		GENERIC_READ | GENERIC_WRITE | DELETE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL /* | FILE_FLAG_DELETE_ON_CLOSE */, 
		NULL);
	    /*
	     * Can't apply FILE_FLAG_DELETE_ON_CLOSE due to 
	     * the logics of clist_fclose. Also note that
	     * gdev_prn_render_pages requires multiple temporary files
	     * to exist simultaneousely, so that keeping all them opened
	     * may exceed available CRTL file handles.
	     */
	}
    }
    if (hfile != INVALID_HANDLE_VALUE) {
	/* Associate a C file handle with an OS file handle. */
	fd = _open_osfhandle((long)hfile, 0);
	if (fd == -1)
	    CloseHandle(hfile);
	else {
	    /* Associate a C file stream with C file handle. */
	    f = fdopen(fd, mode);
	    if (f == NULL)
		_close(fd);
	}
    }
    if (f != NULL) {
	if ((strlen(sTempFileName) < gp_file_name_sizeof))
	    strncpy(fname, sTempFileName, gp_file_name_sizeof - 1);
	else {
	    /* The file name is too long. */
	    fclose(f);
	    f = NULL;
	}
    }
    if (f == NULL)
	eprintf1("**** Could not open temporary file '%s'\n", fname);
    return f;
}

/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
    return fopen(fname, mode);
}

/* ------ Font enumeration ------ */
 
 /* This is used to query the native os for a list of font names and
  * corresponding paths. The general idea is to save the hassle of
  * building a custom fontmap file.
  */
 
void *gp_enumerate_fonts_init(gs_memory_t *mem)
{
    return NULL;
}
         
int gp_enumerate_fonts_next(void *enum_state, char **fontname, char **path)
{
    return 0;
}
                         
void gp_enumerate_fonts_free(void *enum_state)
{
}           
