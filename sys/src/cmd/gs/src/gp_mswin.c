/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_mswin.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/*
 * Microsoft Windows 3.n platform support for Ghostscript.
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
#include <stdlib.h>
#include <stdarg.h>
#include "ctype_.h"
#include "dos_.h"
#include <io.h>
#include "malloc_.h"
#include <fcntl.h>
#include <signal.h>
#include "gx.h"
#include "gp.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsexit.h"

#include "windows_.h"
#include <shellapi.h>
#ifdef __WIN32__
#include <winspool.h>
#endif
#include "gp_mswin.h"
#include "gsdll.h"
/* use longjmp instead of exit when using DLL */
#include <setjmp.h>
extern jmp_buf gsdll_env;

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));

/* ------ from gnuplot winmain.c plus new stuff ------ */

/* limits */
#define MAXSTR 255

/* public handles */
HINSTANCE phInstance;
HWND hwndtext = HWND_DESKTOP;	/* would be better to be a real window */

const LPSTR szAppName = "Ghostscript";
BOOL is_win32s = FALSE;
char FAR win_prntmp[MAXSTR];	/* filename of PRN temporary file */
private int is_printer(const char *name);
int win_init = 0;		/* flag to know if gp_exit has been called */
int win_exit_status;

BOOL CALLBACK _export AbortProc(HDC, int);

#ifdef __WIN32__
/* DLL entry point for Borland C++ */
BOOL WINAPI _export
DllEntryPoint(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    /* Win32s: HIWORD bit 15 is 1 and bit 14 is 0 */
    /* Win95:  HIWORD bit 15 is 1 and bit 14 is 1 */
    /* WinNT:  HIWORD bit 15 is 0 and bit 14 is 0 */
    /* WinNT Shell Update Release is WinNT && LOBYTE(LOWORD) >= 4 */
    DWORD version = GetVersion();

    if (((HIWORD(version) & 0x8000) != 0) && ((HIWORD(version) & 0x4000) == 0))
	is_win32s = TRUE;

    phInstance = hInst;
    return TRUE;
}

/* DLL entry point for Microsoft Visual C++ */
BOOL WINAPI _export
DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    return DllEntryPoint(hInst, fdwReason, lpReserved);
}


#else
int WINAPI _export
LibMain(HINSTANCE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine)
{
    phInstance = hInstance;
    return 1;
}

int WINAPI _export
WEP(int nParam)
{
    return 1;
}
#endif


BOOL CALLBACK _export
AbortProc(HDC hdcPrn, int code)
{
    process_interrupts();
    if (code == SP_OUTOFDISK)
	return (FALSE);		/* cancel job */
    return (TRUE);
}

/* ------ Process message loop ------ */
/*
 * Check messages and interrupts; return true if interrupted.
 * This is called frequently - it must be quick!
 */
int
gp_check_interrupts(void)
{
    return (*pgsdll_callback) (GSDLL_POLL, NULL, 0);
}

/* ====== Generic platform procedures ====== */

/* ------ Initialization/termination (from gp_itbc.c) ------ */

/* Do platform-dependent initialization. */
void
gp_init(void)
{
    win_init = 1;
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
    win_init = 0;
    win_exit_status = exit_status;
}

/* Exit the program. */
void
gp_do_exit(int exit_status)
{
    /* Use longjmp since exit would terminate caller */
    /* setjmp code will check gs_exit_status */
    longjmp(gsdll_env, gs_exit_status);
}

/* ------ Printer accessing ------ */

/* Forward references */
private int gp_printfile(P2(const char *, const char *));

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
    } else
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

/* Printer abort procedure and progress/cancel dialog box */
/* Used by Win32 and mswinprn device */

HWND hDlgModeless;

BOOL CALLBACK _export
PrintAbortProc(HDC hdcPrn, int code)
{
    MSG msg;

    while (hDlgModeless && PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	if (hDlgModeless || !IsDialogMessage(hDlgModeless, &msg)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
    return (hDlgModeless != 0);
}

/* Modeless dialog box - Cancel printing */
BOOL CALLBACK _export
CancelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
	case WM_INITDIALOG:
	    SetWindowText(hDlg, szAppName);
	    return TRUE;
	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case IDCANCEL:
		    DestroyWindow(hDlg);
		    hDlgModeless = 0;
		    EndDialog(hDlg, 0);
		    return TRUE;
	    }
    }
    return FALSE;
}

#ifndef __WIN32__

/* Windows does not provide API's in the SDK for writing directly to a */
/* printer.  Instead you are supposed to use the Windows printer drivers. */
/* Ghostscript has its own printer drivers, so we need to use some API's */
/* that are documented only in the Device Driver Adaptation Guide */
/* that comes with the DDK.  Prototypes taken from DDK <print.h> */
DECLARE_HANDLE(HPJOB);

HPJOB WINAPI OpenJob(LPSTR, LPSTR, HPJOB);
int WINAPI StartSpoolPage(HPJOB);
int WINAPI EndSpoolPage(HPJOB);
int WINAPI WriteSpool(HPJOB, LPSTR, int);
int WINAPI CloseJob(HPJOB);
int WINAPI DeleteJob(HPJOB, int);
int WINAPI WriteDialog(HPJOB, LPSTR, int);
int WINAPI DeleteSpoolPage(HPJOB);

#endif /* WIN32 */

/* Dialog box to select printer port */
BOOL CALLBACK _export
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
#ifdef __WIN32__
		    if (HIWORD(wParam)
#else
		    if (HIWORD(lParam)
#endif
			== LBN_DBLCLK)
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

#ifdef __WIN32__		/* ******** WIN32 ******** */

/******************************************************************/
/* Print File to port or queue */
/* port==NULL means prompt for port or queue with dialog box */

/* This is messy because Microsoft changed the spooler interface */
/* between Window 3.1 and Windows 95/NT */
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

#ifdef __WIN32__
    if (!is_win32s)
	return get_queues();
#endif

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
	gp_check_interrupts();
    }

    return 0;
}



#else /* ******** !WIN32 ******** */

/* Print File to port */
private int
gp_printfile(const char *filename, const char *pmport)
{
#define PRINT_BUF_SIZE 16384u
    char *buffer;
    char *portname;
    int i, port;
    FILE *f;
    DLGPROC lpfnSpoolProc;
    WORD count;
    DLGPROC lpfnCancelProc;
    int error = FALSE;
    long lsize;
    long ldone;
    char pcdone[20];
    MSG msg;
    HPJOB hJob;

    if (is_spool(pmport) && (strlen(pmport) >= 8)) {
	/* translate from printer name to port name */
	char driverbuf[256];

	GetProfileString("Devices", pmport + 8, "", driverbuf, sizeof(driverbuf));
	strtok(driverbuf, ",");
	pmport = strtok(NULL, ",");
    }
    /* get list of ports */
    if ((buffer = malloc(PRINT_BUF_SIZE)) == (char *)NULL)
	return FALSE;

    if ((strlen(pmport) == 0) || (strcmp(pmport, "PRN") == 0)) {
	GetProfileString("ports", NULL, "", buffer, PRINT_BUF_SIZE);
	/* select a port */
#ifdef __WIN32__
	lpfnSpoolProc = (DLGPROC) SpoolDlgProc;
#else
#ifdef __DLL__
	lpfnSpoolProc = (DLGPROC) GetProcAddress(phInstance, "SpoolDlgProc");
#else
	lpfnSpoolProc = (DLGPROC) MakeProcInstance((FARPROC) SpoolDlgProc, phInstance);
#endif
#endif
	port = DialogBoxParam(phInstance, "SpoolDlgBox", (HWND) NULL, lpfnSpoolProc, (LPARAM) buffer);
#if !defined(__WIN32__) && !defined(__DLL__)
	FreeProcInstance((FARPROC) lpfnSpoolProc);
#endif
	if (!port) {
	    free(buffer);
	    return FALSE;
	}
	portname = buffer;
	for (i = 1; i < port && strlen(portname) != 0; i++)
	    portname += lstrlen(portname) + 1;
    } else
	portname = (char *)pmport;	/* Print Manager port name already supplied */

    if ((f = fopen(filename, "rb")) == (FILE *) NULL) {
	free(buffer);
	return FALSE;
    }
    fseek(f, 0L, SEEK_END);
    lsize = ftell(f);
    if (lsize <= 0)
	lsize = 1;
    fseek(f, 0L, SEEK_SET);

    hJob = OpenJob(portname, filename, (HPJOB) NULL);
    switch ((int)hJob) {
	case SP_APPABORT:
	case SP_ERROR:
	case SP_OUTOFDISK:
	case SP_OUTOFMEMORY:
	case SP_USERABORT:
	    fclose(f);
	    free(buffer);
	    return FALSE;
    }
    if (StartSpoolPage(hJob) < 0)
	error = TRUE;

#ifdef __WIN32__
    lpfnCancelProc = (DLGPROC) CancelDlgProc;
#else
#ifdef __DLL__
    lpfnCancelProc = (DLGPROC) GetProcAddress(phInstance, "CancelDlgProc");
#else
    lpfnCancelProc = (DLGPROC) MakeProcInstance((FARPROC) CancelDlgProc, phInstance);
#endif
#endif
    hDlgModeless = CreateDialog(phInstance, "CancelDlgBox", (HWND) NULL, lpfnCancelProc);
    ldone = 0;

    while (!error && hDlgModeless
	   && (count = fread(buffer, 1, PRINT_BUF_SIZE, f)) != 0) {
	if (WriteSpool(hJob, buffer, count) < 0)
	    error = TRUE;
	ldone += count;
	sprintf(pcdone, "%d %%done", (int)(ldone * 100 / lsize));
	SetWindowText(GetDlgItem(hDlgModeless, CANCEL_PCDONE), pcdone);
	while (PeekMessage(&msg, hDlgModeless, 0, 0, PM_REMOVE)) {
	    if ((hDlgModeless == 0) || !IsDialogMessage(hDlgModeless, &msg)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}
    }
    free(buffer);
    fclose(f);

    if (!hDlgModeless)
	error = TRUE;
    DestroyWindow(hDlgModeless);
    hDlgModeless = 0;
#if !defined(__WIN32__) && !defined(__DLL__)
    FreeProcInstance((FARPROC) lpfnCancelProc);
#endif
    EndSpoolPage(hJob);
    if (error)
	DeleteJob(hJob, 0);
    else
	CloseJob(hJob);
    return !error;
}

#endif /* ******** (!)WIN32 ******** */

/* ------ File naming and accessing ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{				/* The -7 is for XXXXXX plus a possible final \. */
    int len = gp_file_name_sizeof - strlen(prefix) - 7;

    if (gp_getenv("TEMP", fname, &len) != 0)
	*fname = 0;
    else {
	char *temp;

	/* Prevent X's in path from being converted by mktemp. */
	for (temp = fname; *temp; temp++)
	    *temp = tolower(*temp);
	if (strlen(fname) && (fname[strlen(fname) - 1] != '\\'))
	    strcat(fname, "\\");
    }
    strcat(fname, prefix);
    strcat(fname, "XXXXXX");
    mktemp(fname);
    return fopen(fname, mode);
}

/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
    return fopen(fname, mode);
}
