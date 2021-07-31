/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_mswin.c */
/*
 * Microsoft Windows 3.n platform support for Ghostscript.
 * Original version by Russell Lang and Maurice Castro with help from
 * Programming Windows, 2nd Ed., Charles Petzold, Microsoft Press;
 * initially created from gp_dosfb.c and gp_itbc.c 5th June 1992.
 */

/* Modified for Win32 & Microsoft C/C++ 8.0 32-Bit, 26.Okt.1994 */
/* by Friedrich Nowak                                           */

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
#include "gxdevice.h"

#include "windows_.h"
#if WINVER >= 0x030a
#include <shellapi.h>
#endif
#include "gp_mswin.h"
#include "gp_mswtx.h"
#ifdef __DLL__
#include "gsdll.h"
#endif

/* for imitation pipes */
#include "stream.h"
#include "gxiodev.h"			/* must come after stream.h */
extern stream *gs_stream_stdin;		/* from ziodev.c */
extern stream *gs_stream_stdout;	/* from ziodev.c */
extern stream *gs_stream_stderr;	/* from ziodev.c */

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));

/* Imported from gp_msdos.c */
int gp_file_is_console(P1(FILE *));

/* ------ from gnuplot winmain.c plus new stuff ------ */

/* limits */
#define MAXSTR 255

/* public handles */
HWND hwndtext;
HINSTANCE phInstance;

const LPSTR szAppName = "Ghostscript";
const LPSTR szImgName = "Ghostscript Image";
#ifdef __WIN32__
const LPSTR szIniName = "gswin32.ini";
#else
const LPSTR szIniName = "gswin.ini";
#endif
const LPSTR szIniSection = "Text";
BOOL is_win31 = FALSE;
BOOL is_winnt = FALSE;
char FAR win_prntmp[MAXSTR];	/* filename of PRN temporary file */
int win_init = 0;		/* flag to know if gp_exit has been called */
int win_exit_status;

/* gsview.exe */
BOOL gsview = FALSE;
HWND gsview_hwnd = NULL;
BOOL gsview_next = FALSE;
LPSTR gsview_option = "-sGSVIEW=";

/* redirected stdio */
TW textwin;	/* text window structure */

/* imitation pipes */
HGLOBAL pipe_hglobal = NULL;
LPBYTE pipe_lpbyte = NULL;
UINT pipe_count = 0;
#ifdef __WIN32__
HANDLE pipe_hmapfile;
LPBYTE pipe_mapptr;
BOOL pipe_eof;
#endif

BOOL CALLBACK _export AbortProc(HDC, int);

#ifdef __DLL__
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
#else
int main(int argc, char *argv[]);

/* our exit handler */
/* also called from Text Window WM_CLOSE */
void win_exit(void)
{
#if WINVER >= 0x030a
	/* disable Drag Drop */
	if (is_win31)
		DragAcceptFiles(hwndtext, FALSE);
#endif
	/* if we didn't exit through gs_exit() then do so now */
	if (win_init)
		gs_exit(0);

	fcloseall();
	if (win_exit_status) {
		/* display message box so error messages in hwndtext can be read */
		char buf[20];
		if (IsIconic(textwin.hWndText))
		    ShowWindow(textwin.hWndText, SW_SHOWNORMAL);
		BringWindowToTop(textwin.hWndText);  /* make hwndtext visible */
		sprintf(buf, "Exit code %d\nSee text window for details",win_exit_status);
		MessageBox((HWND)NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
	}
	
	/* tell gsview that we are closing */
	if (gsview)
		SendMessage(gsview_hwnd, WM_GSVIEW, GSWIN_CLOSE, (LPARAM)NULL);

	TextClose(&textwin);
#if !defined(_MSC_VER)
	exit(win_exit_status);
#endif
}

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
	int i;
#if defined(_MSC_VER)    /* MSC doesn't give us _argc and _argv[] so ...   */
#define MAXCMDTOKENS 128
	int	_argc=0;
	LPSTR	_argv[MAXCMDTOKENS];
#ifdef __WIN32__
	_argv[_argc] = "gswin32.exe";
#else
	_argv[_argc] = "gswin.exe";
#endif
	_argv[++_argc] = _fstrtok( lpszCmdLine, " ");
	while (_argv[_argc] != NULL)
		_argv[++_argc] = _fstrtok( NULL, " ");
#endif
	is_win31 = FALSE;
	is_winnt = FALSE;
	{
	DWORD version = GetVersion();
	if (((LOBYTE(LOWORD(version))<<8) | HIBYTE(LOWORD(version))) >= 0x30a)
		is_win31 = TRUE;
#ifdef __WIN32__
	if ((HIWORD(version) & 0x8000) == 0)
		is_winnt = TRUE;
	/* treat Windows 95 (Windows 4.0) the same as Windows NT */
	if (((LOBYTE(LOWORD(version))<<8) | HIBYTE(LOWORD(version))) >= 0x400)
		is_winnt = TRUE;
#endif
	}

	if (hPrevInstance) {
		MessageBox((HWND)NULL,"Can't run twice", szAppName, MB_ICONHAND | MB_OK);
		return FALSE;
	}

	/* copy the hInstance into a variable so it can be used */
	phInstance = hInstance;

	/* start up the text window */
	textwin.hInstance = hInstance;
	textwin.hPrevInstance = hPrevInstance;
	textwin.nCmdShow = cmdShow;
	textwin.Title = szAppName;
	textwin.hIcon = LoadIcon(hInstance, (LPSTR)MAKEINTRESOURCE(TEXT_ICON));
	textwin.DragPre = "(";
	textwin.DragPost = ") run\r";
	textwin.ScreenSize.x = 80;
	textwin.ScreenSize.y = 80;
	textwin.KeyBufSize = 2048;
	textwin.CursorFlag = 1;	/* scroll to cursor after \n & \r */
	textwin.shutdown = win_exit;
	GetPrivateProfileString(szIniSection, "FontName", 
	    "", textwin.fontname, sizeof(textwin.fontname), szIniName);
	textwin.fontsize = GetPrivateProfileInt(szIniSection, "FontSize", 
	    0, szIniName);
	if ( (textwin.fontname[0] == '\0') || (textwin.fontsize < 8) ) {
	  char buf[16];
#ifdef __WIN32__
	  if (is_winnt) {
	    /* NT 3.5 doesn't have Terminal font so set a different default */
	    strcpy(textwin.fontname, "Courier New");
	    textwin.fontsize = 10;
	  }
	  else
#endif
	  {
	    strcpy(textwin.fontname, "Terminal");
	    textwin.fontsize = 9;
	  }
	  /* save text size */
          WritePrivateProfileString(szIniSection, "FontName", textwin.fontname,
	    szIniName);
	  sprintf(buf, "%d", textwin.fontsize);
	  WritePrivateProfileString(szIniSection, "FontSize", buf, szIniName);
	}

	if (TextInit(&textwin))
		exit(1);
	hwndtext = textwin.hWndText;
#ifndef _MSC_VER
	(void) atexit((atexit_t)win_exit); /* setup exit handler */
#else
	(void) atexit(win_exit);
#endif
	/* check if we are to use gsview.exe */
	for (i=0; i<_argc; i++) {
	    if (!strncmp(_argv[i], gsview_option, strlen(gsview_option))) {
		gsview_hwnd = (HWND)atoi(_argv[i]+strlen(gsview_option));
		if (gsview_hwnd != (HWND)NULL) {
			if (!IsWindow(gsview_hwnd)) {
				char buf[80];
				sprintf(buf,"GSVIEW window handle %u is invalid",(int)gsview_hwnd);
				MessageBox(hwndtext, buf, szAppName, MB_OK);
				return 0;
			}
			gsview = TRUE;
			/* give gsview the handle to our text window */
			SendMessage(gsview_hwnd, WM_GSVIEW, HWND_TEXT, (LPARAM)hwndtext);
		}
	    }
	}

	main(_argc, _argv);

	/* never reached */
	win_exit(); 
	return win_exit_status;
}

#endif /* !__DLL__ */

BOOL CALLBACK _export
AbortProc(HDC hdcPrn, int code)
{
    process_interrupts();
    return(TRUE);
}
  
/* ------ Process message loop ------ */
/*
 * Check messages and interrupts; return true if interrupted.
 * This is called frequently - it must be quick!
 */
int
gp_check_interrupts(void)
{
	TextMessage();
	return 0;
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

/* ------ Printer accessing ------ */
  
/* Forward references */
private int gp_printfile(P2(char *, const char *));

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char *fname, int binary_mode)
{
char buf[128];
	/* if fname appears in win.ini ports section, then treat as printer */
	GetProfileString("ports", fname, "XYZ", buf, sizeof(buf));
	if ( strlen(fname) == 0 || !strcmp(fname, "PRN") || strcmp(buf,"XYZ") )
	{	FILE *pfile;
		pfile = gp_open_scratch_file(gp_scratch_file_name_prefix, 
			win_prntmp, "wb");
		return pfile;
	}
	else
		return fopen(fname, (binary_mode ? "wb" : "w"));
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE *pfile, const char *fname)
{
char buf[128];
	fclose(pfile);
	GetProfileString("ports", fname, "XYZ", buf, sizeof(buf));
	if (strlen(fname) && strcmp(fname,"PRN") && !strcmp(buf,"XYZ"))
	    return;		/* a file, not a printer */

	gp_printfile(win_prntmp, fname);
	unlink(win_prntmp);
}

#ifndef __WIN32__

/* Windows does not provide API's in the SDK for writing directly to a */
/* printer.  Instead you are supposed to use the Windows printer drivers. */
/* Ghostscript has its own printer drivers, so we need to use some API's */
/* that are documented only in the Device Driver Adaptation Guide */
/* that comes with the DDK.  Prototypes taken from DDK <print.h> */
DECLARE_HANDLE(HPJOB);

HPJOB	WINAPI OpenJob(LPSTR, LPSTR, HPJOB);
int	WINAPI StartSpoolPage(HPJOB);
int	WINAPI EndSpoolPage(HPJOB);
int	WINAPI WriteSpool(HPJOB, LPSTR, int);
int	WINAPI CloseJob(HPJOB);
int	WINAPI DeleteJob(HPJOB, int);
int	WINAPI WriteDialog(HPJOB, LPSTR, int);
int	WINAPI DeleteSpoolPage(HPJOB);

#endif		/* WIN32 */

HWND hDlgModeless;

/* Modeless dialog box - Cancel printing */
BOOL CALLBACK _export
CancelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
	case WM_INITDIALOG:
	    SetWindowText(hDlg, szAppName);
	    return TRUE;
	case WM_COMMAND:
	    switch(LOWORD(wParam)) {
		case IDCANCEL:
		    DestroyWindow(hDlg);
		    hDlgModeless = 0;
		    EndDialog(hDlg, 0);
		    return TRUE;
	    }
    }
    return FALSE;
}

/* Dialog box to select printer port */
BOOL CALLBACK _export
SpoolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
LPSTR entry;
    switch(message) {
	case WM_INITDIALOG:
	    entry = (LPSTR)lParam;
	    while (*entry) {
		SendDlgItemMessage(hDlg, SPOOL_PORT, LB_ADDSTRING, 0, (LPARAM)entry);
		entry += lstrlen(entry)+1;
	    }
	    SendDlgItemMessage(hDlg, SPOOL_PORT, LB_SETCURSEL, 0, (LPARAM)0);
	    return TRUE;
	case WM_COMMAND:
	    switch(LOWORD(wParam)) {
		case SPOOL_PORT:
#ifdef WIN32
		    if (HIWORD(wParam)
#else
		    if (HIWORD(lParam)
#endif
			               == LBN_DBLCLK)
			PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
		    return FALSE;
		case IDOK:
		    EndDialog(hDlg, 1+(int)SendDlgItemMessage(hDlg, SPOOL_PORT, LB_GETCURSEL, 0, 0L));
		    return TRUE;
		case IDCANCEL:
		    EndDialog(hDlg, 0);
		    return TRUE;
	    }
    }
    return FALSE;
}

 
#ifdef __WIN32__		/* ******** WIN32 ******** */

/* Win32s can't get access to OpenJob etc., so we try to sneak the */
/* data through the Windows printer driver unchanged */
BOOL CALLBACK _export
PrintAbortProc(HDC hdcPrn, int code)
{
    MSG msg;
    while (hDlgModeless && PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	if (hDlgModeless || !IsDialogMessage(hDlgModeless,&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
	}
    }
    return(hDlgModeless!=0);
}

/* Print File */
private int
gp_printfile(char *filename, const char *pmport)
{
HDC printer;
PRINTDLG pd;
DOCINFO di;
char *buf;
WORD *bufcount;
int count;
FILE *f;
long lsize;
long ldone;
char *fmt = "%d%% done";
char pcdone[10];
int error = TRUE;
	memset(&pd, 0, sizeof(PRINTDLG));
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hwndOwner = hwndtext;
	pd.Flags = PD_PRINTSETUP | PD_RETURNDC;

	if ((f = fopen(filename, "rb")) == (FILE *)NULL)
	    return FALSE;
	fseek(f, 0L, SEEK_END);
	lsize = ftell(f);
	if (lsize <= 0)
	    lsize = 1;
	fseek(f, 0L, SEEK_SET);
	ldone = 0;

	if (PrintDlg(&pd)) {
	printer = pd.hDC;
	if (printer != (HDC)NULL) {
	    if ( (buf = malloc(4096+2)) != (char *)NULL ) {
		bufcount = (WORD *)buf;
		EnableWindow(hwndtext,FALSE);
		hDlgModeless = CreateDialogParam(phInstance,"CancelDlgBox",hwndtext,CancelDlgProc,(LPARAM)szAppName);
		SetAbortProc(printer, PrintAbortProc);
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = szAppName;
		di.lpszOutput = NULL;
		if (StartDoc(printer, &di) > 0) {
		    while ( hDlgModeless &&  ((count = fread(buf+2, 1, 4096, f)) != 0) ) {
			*bufcount = count;
			Escape(printer, PASSTHROUGH, count+2, (LPSTR)buf, NULL);
	    		ldone += count;
	    		sprintf(pcdone, fmt, (int)(ldone * 100 / lsize));
	    		SetWindowText(GetDlgItem(hDlgModeless, CANCEL_PCDONE), pcdone);
		    }
		    if (hDlgModeless)
			EndDoc(printer);
		    else
			AbortDoc(printer);
		}
		if (hDlgModeless) {
		    DestroyWindow(hDlgModeless);
		    error = FALSE;
		    hDlgModeless = (HWND)0;
		}
	        EnableWindow(hwndtext,TRUE);
	        free(buf);
	    }
	  }
	  DeleteDC(printer);
	}
	fclose(f);
	return !error;
}

#else				/* ******** !WIN32 ******** */

/* Print File to port */
private int
gp_printfile(char *filename, const char *pmport)
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

	/* get list of ports */
	if ((buffer = malloc(PRINT_BUF_SIZE)) == (char *)NULL)
	    return FALSE;

	if ( (strlen(pmport)==0) || (strcmp(pmport, "PRN")==0) ) {
	    GetProfileString("ports", NULL, "", buffer, PRINT_BUF_SIZE);
	    /* select a port */
	    lpfnSpoolProc = (DLGPROC)MakeProcInstance((FARPROC)SpoolDlgProc, phInstance);
	    port = DialogBoxParam(phInstance, "SpoolDlgBox", hwndtext, lpfnSpoolProc, (LPARAM)buffer);
	    FreeProcInstance((FARPROC)lpfnSpoolProc);
	    if (!port) {
	        free(buffer);
	        return FALSE;
	    }
	    portname = buffer;
	    for (i=1; i<port && strlen(portname)!=0; i++)
	        portname += lstrlen(portname)+1;
	}
	else
	    portname = (char *)pmport;	/* Print Manager port name already supplied */
	
	if ((f = fopen(filename, "rb")) == (FILE *)NULL) {
	    free(buffer);
	    return FALSE;
	}
	fseek(f, 0L, SEEK_END);
	lsize = ftell(f);
	if (lsize <= 0)
	    lsize = 1;
	fseek(f, 0L, SEEK_SET);

	hJob = OpenJob(portname, filename, (HDC)NULL);
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

	lpfnCancelProc = (DLGPROC)MakeProcInstance((FARPROC)CancelDlgProc, phInstance);
	hDlgModeless = CreateDialog(phInstance, "CancelDlgBox", hwndtext, lpfnCancelProc);
	ldone = 0;

	while (!error && hDlgModeless 
	  && (count = fread(buffer, 1, PRINT_BUF_SIZE, f)) != 0 ) {
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
	    error=TRUE;
	DestroyWindow(hDlgModeless);
	hDlgModeless = 0;
	FreeProcInstance((FARPROC)lpfnCancelProc);
	EndSpoolPage(hJob);
	if (error)
	    DeleteJob(hJob, 0);
	else
	    CloseJob(hJob);
	return !error;
}

#endif				/* ******** (!)WIN32 ******** */

/* ------ File names ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{	char *temp;
	if ( (temp = getenv("TEMP")) == NULL )
		*fname = 0;
	else
	{	strcpy(fname, temp);
		/* Prevent X's in path from being converted by mktemp. */
		for ( temp = fname; *temp; temp++ )
			*temp = tolower(*temp);
		if ( strlen(fname) && (fname[strlen(fname)-1] != '\\') )
			strcat(fname, "\\");
	}
	strcat(fname, prefix);
	strcat(fname, "XXXXXX");
	mktemp(fname);
	return fopen(fname, mode);
}

/* ====== Substitute for stdio ====== */

/* Forward references */
private void win_std_init(void);
private void win_pipe_init(void);
private stream_proc_process(win_std_read_process);
private stream_proc_process(win_std_write_process);

/* Use a pseudo IODevice to get win_stdio_init called at the right time. */
/* This is bad architecture; we'll fix it later. */
private iodev_proc_init(win_stdio_init);
gx_io_device gs_iodev_wstdio = {
	"wstdio", "Special",
	{ win_stdio_init, iodev_no_open_device,
	  iodev_no_open_file, iodev_no_fopen, iodev_no_fclose,
	  iodev_no_delete_file, iodev_no_rename_file,
	  iodev_no_file_status, iodev_no_enumerate_files
	}
};

/* Do one-time initialization */
private int
win_stdio_init(gx_io_device *iodev, gs_memory_t *mem)
{
	win_std_init();		/* redefine stdin/out/err to our window routines */
#ifndef __DLL__
	if (gsview)
	    win_pipe_init();	/* redefine stdin to be a pipe */
#endif
	return 0;
}

/* reinitialize stdin/out/err to use our windows */
/* assume stream has already been initialized for the real stdin */
private void
win_std_init(void)
{
	if ( gp_file_is_console(gs_stream_stdin->file) )
	{	/* Allocate a real buffer for stdin. */
		/* The size must not exceed the size of the */
		/* lineedit buffer.  (This is a hack.) */
#define win_stdin_buf_size 160
		static const stream_procs pin =
		{	s_std_noavailable, s_std_noseek, s_std_read_reset,
			s_std_read_flush, s_std_close,
			win_std_read_process
		};
		byte *buf = gs_alloc_bytes(gs_stream_stdin->memory,
					   win_stdin_buf_size,
					   "win_stdin_init");
		s_std_init(gs_stream_stdin, buf, win_stdin_buf_size,
			   &pin, s_mode_read);
		gs_stream_stdin->file = NULL;
	}

	{	static const stream_procs pout =
		{	s_std_noavailable, s_std_noseek, s_std_write_reset,
			s_std_write_flush, s_std_close,
			win_std_write_process
		};
		if ( gp_file_is_console(gs_stream_stdout->file) )
		{	gs_stream_stdout->procs = pout;
			gs_stream_stdout->file = NULL;
		}
		if ( gp_file_is_console(gs_stream_stderr->file) )
		{	gs_stream_stderr->procs = pout;
			gs_stream_stderr->file = NULL;
		}
	}

}

#ifdef __DLL__
private int
win_std_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{
	int count = pw->limit - pw->ptr;

	if ( count == 0 ) 		/* empty buffer */
		return 1;

/* callback to get more input */
	count = (*pgsdll_callback)(GSDLL_STDIN, pw->ptr+1, count);
	if (count == 0) {
	    /* EOF */
	    /* what should we do? */
	    return EOFC;
	}

	pw->ptr += count;
	return 1;
}
#else /* !__DLL__ */
/* We should really use a private buffer for line reading, */
/* because we can't predict the size of the supplied input area.... */
private int
win_std_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	byte *buf = pw->ptr;
	/* Implement line editing here. */
#define ERASELINE 21		/* ^U */
#define ERASECHAR1 8		/* ^H */
#define ERASECHAR2 127		/* DEL */
	byte *dest = buf;
	byte *limit = pw->limit - 1;	/* always leave room for \n */
	uint ch;
	if ( dest > limit )		/* empty buffer */
		return 1;
	while ( (ch = TextGetCh(&textwin)) != '\n' )
	{	switch ( ch )
		{
		default:
			if ( dest == limit )
				MessageBeep(-1);
			else
			{	*++dest = ch;
				TextPutCh(&textwin, (char) ch);
			}
			break;
		case ERASELINE:
			while ( dest > buf )
			{	TextPutCh(&textwin, '\b');
				TextPutCh(&textwin, ' ');
				TextPutCh(&textwin, '\b');
				dest--;
			}
			break;
		case ERASECHAR1:
		case ERASECHAR2:
			if ( dest > buf )
			{	TextPutCh(&textwin, '\b');
				TextPutCh(&textwin, ' ');
				TextPutCh(&textwin, '\b');
				dest--;
			}
			break;
		}
	}
	*++dest = ch;
	TextPutCh(&textwin, (char) ch);
	pw->ptr = dest;
	return 1;
}
#endif /* !__DLL__ */


private int
win_std_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;
#ifdef __DLL__
	(*pgsdll_callback)(GSDLL_STDOUT, (char *)(pr->ptr +1), count);
	pr->ptr = pr->limit;
#else
	/* The prototype for TextWriteBuf is missing a const.... */
	TextWriteBuf(&textwin, (char *)(pr->ptr + 1), count);
	pr->ptr = pr->limit;
	(void)gp_check_interrupts();
#endif
	return 0;
}

/* This is used instead of the stdio version. */
/* The declaration must be identical to that in <stdio.h>. */
#if defined(_WIN32) && defined(_MSC_VER)
int _CRTAPI2
fprintf(FILE *file, const char *fmt, ...)
#else
int _Cdecl _FARFUNC
fprintf(FILE _FAR *file, const char *fmt, ...)
#endif
{
int count;
va_list args;
	va_start(args,fmt);
	if ( gp_file_is_console(file) ) {
		char buf[1024];
		count = vsprintf(buf,fmt,args);
#ifdef __DLL__
		(*pgsdll_callback)(GSDLL_STDOUT, buf, count);
#else
/*		MessageBox((HWND)NULL, buf, szAppName, MB_OK | MB_ICONSTOP); */
		TextWriteBuf(&textwin, buf, count);
#endif
	}
	else
		count = vfprintf(file, fmt, args);
	va_end(args);
	return count;
}

/* ------ Imitation pipes (only used with gsview) ------ */

/* Forward references */
private stream_proc_process(win_pipe_read_process);
private void win_pipe_request(void);
private int win_pipe_read(char *, unsigned int);

/* reinitialize stdin stream to read from imitation pipe */
/* assume stream has already been initialized for the windows stdin */
private void
win_pipe_init(void)
{	if ( gs_stream_stdin->procs.process == win_std_read_process )
		gs_stream_stdin->procs.process = win_pipe_read_process;
}

private int
win_pipe_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	if ( pw->ptr < pw->limit )
	{	int nread = win_pipe_read(pw->ptr + 1, 1);
		if ( nread <= 0 )
			return EOFC;
		pw->ptr++;
	}
	return 1;
}

/* get a block from gsview */
private void
win_pipe_request(void)
{
    MSG msg;
	/* request another block */
#ifdef __WIN32__
	if (is_winnt) {
	    pipe_hglobal = (HGLOBAL)NULL;
	    pipe_lpbyte = (LPBYTE)NULL;
	    if (pipe_eof)
		return;
	}
	else
#endif
	{
	    if (pipe_lpbyte != (LPBYTE)NULL)
		    GlobalUnlock(pipe_hglobal);
	    if (pipe_hglobal != (HGLOBAL)NULL)
		    GlobalFree(pipe_hglobal);
	    pipe_hglobal = (HGLOBAL)NULL;
	    pipe_lpbyte = (LPBYTE)NULL;
	}

	SendMessage(gsview_hwnd, WM_GSVIEW, PIPE_REQUEST, 0L);
	/* wait for block */
	/* don't use PeekMessage in a loop - would waste CPU cycles under WinNT */
    	while ((pipe_hglobal == (HGLOBAL)NULL)
	  && GetMessage(&msg, 0, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
#ifdef __WIN32__
	if (is_winnt) {
	    if (pipe_mapptr == NULL) {
		char buf[64];
		sprintf(buf, "gsview_%d", gsview_hwnd);
		pipe_hmapfile = OpenFileMapping(FILE_MAP_READ, FALSE, buf);
		pipe_mapptr = MapViewOfFile(pipe_hmapfile, FILE_MAP_READ, 0, 0, 0);
		if (pipe_mapptr == NULL) {
	            MessageBox((HWND)NULL, "Imitation pipe handle is zero", szAppName, MB_OK | MB_ICONSTOP);
		    pipe_lpbyte = NULL;
		    pipe_count = 0;
		    return;
		}
	    }
	    pipe_lpbyte = pipe_mapptr+sizeof(WORD);
	    pipe_count = *((WORD *)pipe_mapptr);
	}
	else
#endif
	{
	  if (pipe_hglobal == (HGLOBAL)NULL) {
	    MessageBox((HWND)NULL, "Imitation pipe handle is zero", szAppName, MB_OK | MB_ICONSTOP);
          }
	  else {
	    pipe_lpbyte = GlobalLock(pipe_hglobal);
	    if (pipe_lpbyte == (LPBYTE)NULL)  {
		char buf[256];
		sprintf(buf, "Imitation pipe handle = 0x%x, pointer = NULL", pipe_hglobal);
	        MessageBox((HWND)NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
		pipe_count = 0;
	    }
	    else {
	        pipe_count = *((WORD FAR *)pipe_lpbyte);
	        pipe_lpbyte += sizeof(WORD);
	    }
	  }
	}
}

/* pipe_read - similar to fread */
private int
win_pipe_read(char *ptr, unsigned int count)
{
	int n;
	if (!pipe_count)
	    win_pipe_request();
	if (!pipe_count)
	    return -1;
	n = min(count, pipe_count);
	memcpy(ptr, pipe_lpbyte, n);
	pipe_lpbyte += n;
	pipe_count -= n;
	return n;
}
