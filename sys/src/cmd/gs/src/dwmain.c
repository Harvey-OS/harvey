/* Copyright (C) 1996-2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* $Id: dwmain.c,v 1.21 2004/09/15 19:41:01 ray Exp $ */
/* Ghostscript DLL loader for Windows */

#include "windows_.h"
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gscdefs.h"
#define GSREVISION gs_revision
#include "ierrors.h"
#include "iapi.h"
#include "vdtrace.h"

#include "dwmain.h"
#include "dwdll.h"
#include "dwtext.h"
#include "dwimg.h"
#include "dwtrace.h"
#include "dwreg.h"
#include "gdevdsp.h"

/* public handles */
HINSTANCE ghInstance;

/* redirected stdio */
TW *tw;

static const LPSTR szAppName = "Ghostscript";

const LPSTR szIniName = "gswin32.ini";
const char *szDllName = "gsdll32.dll";
const LPSTR szIniSection = "Text";


GSDLL gsdll;
void *instance;
HWND hwndtext;

char start_string[] = "systemdict /start get exec\n";

static int poll(void)
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    /* If text window closing then abort Ghostscript */
    if (tw->quitnow)
	return e_Fatal;
    return 0;
}

/*********************************************************************/
/* stdio functions */
static int GSDLLCALL
gsdll_stdin(void *instance, char *buf, int len)
{
    return text_read_line(tw, buf, len);
}

static int GSDLLCALL
gsdll_stdout(void *instance, const char *str, int len)
{
    text_write_buf(tw, str, len);
    return len;
}

static int GSDLLCALL
gsdll_stderr(void *instance, const char *str, int len)
{
    text_write_buf(tw, str, len);
    return len;
}

/* Poll the caller for cooperative multitasking. */
/* If this function is NULL, polling is not needed */
static int GSDLLCALL gsdll_poll(void *handle)
{
    return poll();
}
/*********************************************************************/

/* new dll display device */
/* 
#define DISPLAY_DEBUG
 */

/* New device has been opened */
/* This is the first event from this device. */
static int display_open(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_open(0x%x, 0x%x)\n", handle, device);
    text_puts(tw, buf);
#endif
    img = image_new(handle, device);	/* create and add to list */
    if (img)
        image_open(img);
    return 0;
}

/* Device is about to be closed. */
/* Device will not be closed until this function returns. */
static int display_preclose(void *handle, void *device)
{
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_preclose(0x%x, 0x$x)\n", handle, device);
    text_puts(tw, buf);
#endif
    /* do nothing - no thread synchonisation needed */
    return 0;
}

/* Device has been closed. */
/* This is the last event from this device. */
static int display_close(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_close(0x%x, 0x$x)\n", handle, device);
    text_puts(tw, buf);
#endif
    img = image_find(handle, device);
    if (img) {
	image_delete(img);	/* remove from list but don't free */
	image_close(img);
    }
    return 0;
}

/* Device is about to be resized. */
/* Resize will only occur if this function returns 0. */
static int display_presize(void *handle, void *device, int width, int height, 
	int raster, unsigned int format)
{
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_presize(0x%x, 0x%x, width=%d height=%d raster=%d\n\
  format=%d)\n", 
       handle, device, width, height, raster, format);
    text_puts(tw, buf);
#endif
    return 0;
}
   
/* Device has been resized. */
/* New pointer to raster returned in pimage */
static int display_size(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_size(0x%x, 0x%x, width=%d height=%d raster=%d\n\
  format=%d image=0x%x)\n", 
       handle, device, width, height, raster, format, pimage);
    text_puts(tw, buf);
#endif
    img = image_find(handle, device);
    image_size(img, width, height, raster, format, pimage);
    image_updatesize(img);
    return 0;
}
   
/* flushpage */
static int display_sync(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_sync(0x%x, 0x%x)\n", handle, device);
    text_puts(tw, buf);
#endif
    img = image_find(handle, device);
    image_sync(img);
    return 0;
}

/* showpage */
/* If you want to pause on showpage, then don't return immediately */
static int display_page(void *handle, void *device, int copies, int flush)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    char buf[256];
    sprintf(buf, "display_page(0x%x, 0x%x, copies=%d flush=%d)\n", 
	handle, device, copies, flush);
    text_puts(tw, buf);
#endif
    img = image_find(handle, device);
    image_page(img);
    return 0;
}

/* Poll the caller for cooperative multitasking. */
/* If this function is NULL, polling is not needed */
static int display_update(void *handle, void *device, 
    int x, int y, int w, int h)
{
    IMAGE *img;
    img = image_find(handle, device);
    image_poll(img);	/* redraw the window periodically */
    return poll();
}

int display_separation(void *handle, void *device, 
   int comp_num, const char *name,
   unsigned short c, unsigned short m,
   unsigned short y, unsigned short k)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_separation(0x%x, 0x%x, %d '%s' %d,%d,%d,%d)\n", 
	handle, device, comp_num, name, (int)c, (int)m, (int)y, (int)k);
#endif
    img = image_find(handle, device);
    if (img)
        image_separation(img, comp_num, name, c, m, y, k);
    return 0;
}

display_callback display = { 
    sizeof(display_callback),
    DISPLAY_VERSION_MAJOR,
    DISPLAY_VERSION_MINOR,
    display_open,
    display_preclose,
    display_close,
    display_presize,
    display_size,
    display_sync,
    display_page,
    display_update,
    NULL,	/* memalloc */
    NULL,	/* memfree */
    display_separation
};


/*********************************************************************/

/* program really starts at WinMain */
int new_main(int argc, char *argv[])
{
    int code, code1;
    int exit_status;
    int exit_code;
    int nargc;
    char **nargv;
    char dformat[64];
    char ddpi[64];
    char buf[256];

    memset(buf, 0, sizeof(buf));
    if (load_dll(&gsdll, buf, sizeof(buf))) {
	text_puts(tw, "Can't load Ghostscript DLL\n");
	text_puts(tw, buf);
	text_puts(tw, "\n");
	return 1;
    }

    if (gsdll.new_instance(&instance, NULL) < 0) {
	text_puts(tw, "Can't create Ghostscript instance\n");
	return 1;
    }

#ifdef DEBUG
    visual_tracer_init();
    gsdll.set_visual_tracer(&visual_tracer);
#endif

    gsdll.set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
    gsdll.set_poll(instance, gsdll_poll);
    gsdll.set_display_callback(instance, &display);

    /* insert display device defaults as first arguments */
    {   int format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_1 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	HDC hdc = GetDC(NULL);	/* get hdc for desktop */
	int depth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
	sprintf(ddpi, "-dDisplayResolution=%d", GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(NULL, hdc);
	if (depth == 32)
 	    format = DISPLAY_COLORS_RGB | DISPLAY_UNUSED_LAST | 
		DISPLAY_DEPTH_8 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	else if (depth == 16)
 	    format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_16 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST |
		DISPLAY_NATIVE_555;
	else if (depth > 8)
 	    format = DISPLAY_COLORS_RGB | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_8 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	else if (depth >= 8)
 	    format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_8 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	else if (depth >= 4)
 	    format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_4 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
        sprintf(dformat, "-dDisplayFormat=%d", format);
    }
    nargc = argc + 2;
    nargv = (char **)malloc((nargc + 1) * sizeof(char *));
    nargv[0] = argv[0];
    nargv[1] = dformat;
    nargv[2] = ddpi;
    memcpy(&nargv[3], &argv[1], argc * sizeof(char *));

#if defined(_MSC_VER) || defined(__BORLANDC__)
    __try {
#endif
    code = gsdll.init_with_args(instance, nargc, nargv);
    if (code == 0)
	code = gsdll.run_string(instance, start_string, 0, &exit_code);
    code1 = gsdll.exit(instance);
    if (code == 0 || (code == e_Quit && code1 != 0))
	code = code1;
#if defined(_MSC_VER) || defined(__BORLANDC__)
    } __except(exception_code() == EXCEPTION_STACK_OVERFLOW) {
        code = e_Fatal;
        text_puts(tw, "*** C stack overflow. Quiting...\n");
    }
#endif

    gsdll.delete_instance(instance);

#ifdef DEBUG
    visual_tracer_close();
#endif

    unload_dll(&gsdll);

    free(nargv);

    exit_status = 0;
    switch (code) {
	case 0:
	case e_Quit:
	    break;
	case e_Fatal:
	    exit_status = 1;
	    break;
	case e_Info:
	default:
	    exit_status = 255;
    }
    
    return exit_status;
}



void
set_font(void)
{
    int fontsize;
    char fontname[256];
    char buf[32];

    /* read ini file */
    GetPrivateProfileString(szIniSection, "FontName", "Courier New", fontname, sizeof(fontname), szIniName);
    fontsize = GetPrivateProfileInt(szIniSection, "FontSize", 10, szIniName);

    /* set font */
    text_font(tw, fontname, fontsize); 

    /* write ini file */
    WritePrivateProfileString(szIniSection, "FontName", fontname, szIniName);
    sprintf(buf, "%d", fontsize);
    WritePrivateProfileString(szIniSection, "FontSize", buf, szIniName);
}

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
    int dll_exit_status;
#define MAXCMDTOKENS 128
    /* BC++ 4.5 will give us _argc and _argv[], but they will be */
    /* incorrect if there is a space in the program name. */
    /* Provide our own parsing code to create argc and argv[]. */
    int argc;
    LPSTR argv[MAXCMDTOKENS];
    LPSTR p;
    char command[256];
    char *args;
    char *d, *e;
    char winposbuf[256];
    int len = sizeof(winposbuf);
    int x, y, cx, cy;
 
    /* copy the hInstance into a variable so it can be used */
    ghInstance = hInstance;

    if (hPrevInstance) {
	MessageBox((HWND)NULL,"Can't run twice", szAppName, 
		MB_ICONHAND | MB_OK);
	return FALSE;
    }

    /* If called with "gswin32c.exe arg1 arg2"
     * lpszCmdLine returns:
     *    "arg1 arg2" if called with CreateProcess(NULL, command, ...)
     *    "arg2"      if called with CreateProcess(command, args, ...)
     * GetCommandLine() returns
     *    ""gswin32c.exe" arg1 arg2" 
     *          if called with CreateProcess(NULL, command, ...)
     *    "  arg1 arg2"      
     *          if called with CreateProcess(command, args, ...)
     * Consequently we must use GetCommandLine() 
     */
    p = GetCommandLine();

    argc = 0;
    args = (char *)malloc(lstrlen(p)+1);
    if (args == (char *)NULL) {
	fprintf(stdout, "Insufficient memory in WinMain()\n");
	return 1;
    }
   
    /* Parse command line handling quotes. */
    d = args;
    while (*p) {
	/* for each argument */

	if (argc >= MAXCMDTOKENS - 1)
	    break;

        e = d;
        while ((*p) && (*p != ' ')) {
	    if (*p == '\042') {
		/* Remove quotes, skipping over embedded spaces. */
		/* Doesn't handle embedded quotes. */
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
	    p++;	/* Skip over trailing spaces */
    }
    argv[argc] = NULL;

    if (strlen(argv[0]) == 0) {
	GetModuleFileName(hInstance, command, sizeof(command)-1);
	argv[0] = command;
    }


    tw = text_new();
    if (tw == NULL) {
	MessageBox((HWND)NULL, "Can't create text window", 
		szAppName, MB_OK | MB_ICONSTOP);
 	return 1;
    }

    /* start up the text window */
    if (!hPrevInstance) {
	HICON hicon = LoadIcon(hInstance, (LPSTR)MAKEINTRESOURCE(GSTEXT_ICON));
	text_register_class(tw, hicon);
    }
    set_font();
    text_size(tw, 80, 80);
    text_drag(tw, "(", ") run\r");
    if (win_get_reg_value("Text", winposbuf, &len) == 0) {
	if (sscanf(winposbuf, "%d %d %d %d", &x, &y, &cx, &cy) == 4)
	    text_setpos(tw, x, y, cx, cy);
    }

    /* create the text window */
    if (text_create(tw, szAppName, cmdShow))
	exit(1);

    hwndtext = text_get_handle(tw);

    dll_exit_status = new_main(argc, argv);
    
    if (dll_exit_status && !tw->quitnow) {
	/* display error message in text window */
	MSG msg;
	text_puts(tw, "\nClose this window with the close button on the title bar or the system menu.\n");
	if (IsIconic(text_get_handle(tw)))
	    ShowWindow(text_get_handle(tw), SW_SHOWNORMAL);
	BringWindowToTop(text_get_handle(tw));  /* make text window visible */
	FlashWindow(text_get_handle(tw), TRUE);
	/* Wait until error message is read */
	while (!tw->quitnow && GetMessage(&msg, (HWND)NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }

    /* Save the text window size */
    if (text_getpos(tw, &x, &y, &cx, &cy) == 0) {
	sprintf(winposbuf, "%d %d %d %d", x, y, cx, cy);
	win_set_reg_value("Text", winposbuf);
    }

    text_destroy(tw);
    tw = NULL;

    return dll_exit_status;
}



