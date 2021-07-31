/* Copyright (C) 1996-2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/* $Id: dwmainc.c,v 1.8.2.1 2002/01/16 21:07:16 igorm Exp $ */
/* dwmainc.c */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include "errors.h"
#include "iapi.h"
#include "gdevdsp.h"
#include "dwdll.h"
#include "dwimg.h"

GSDLL gsdll;
void *instance;
BOOL quitnow = FALSE;
HANDLE hthread;
DWORD thread_id;
HWND hwndtext = NULL;	/* for dwimg.c, but not used */
HWND hwndforeground;	/* our best guess for our console window handle */

char start_string[] = "systemdict /start get exec\n";

/*********************************************************************/
/* stdio functions */

static int GSDLLCALL
gsdll_stdin(void *instance, char *buf, int len)
{
    return _read(fileno(stdin), buf, len);
}

static int GSDLLCALL
gsdll_stdout(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stdout);
    fflush(stdout);
    return len;
}

static int GSDLLCALL
gsdll_stderr(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stderr);
    fflush(stderr);
    return len;
}

/*********************************************************************/
/* dll device */

/* We must run windows from another thread, since main thread */
/* is running Ghostscript and blocks on stdin. */

/* We notify second thread of events using PostThreadMessage()
 * with the following messages. Apparently Japanese Windows sends
 * WM_USER+1 with lParam == 0 and crashes. So we use WM_USER+101.
 * Fix from Akira Kakuto
 */
#define DISPLAY_OPEN WM_USER+101
#define DISPLAY_CLOSE WM_USER+102
#define DISPLAY_SIZE WM_USER+103
#define DISPLAY_SYNC WM_USER+104
#define DISPLAY_PAGE WM_USER+105

/*
#define DISPLAY_DEBUG
*/

/* The second thread is the message loop */
static void winthread(void *arg)
{
    MSG msg;
    thread_id = GetCurrentThreadId();
    hthread = GetCurrentThread();

    while (!quitnow && GetMessage(&msg, (HWND)NULL, 0, 0)) {
	switch (msg.message) {
	    case DISPLAY_OPEN:
		image_open((IMAGE *)msg.lParam);
		break;
	    case DISPLAY_CLOSE:
		{
		    IMAGE *img = (IMAGE *)msg.lParam; 
		    HANDLE hmutex = img->hmutex;
		    image_close(img);
		    CloseHandle(hmutex);
		}
		break;
	    case DISPLAY_SIZE:
		image_updatesize((IMAGE *)msg.lParam);
		break;
	    case DISPLAY_SYNC:
		image_sync((IMAGE *)msg.lParam);
		break;
	    case DISPLAY_PAGE:
		image_page((IMAGE *)msg.lParam);
		break;
	    default:
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    }
}


/* New device has been opened */
/* Tell user to use another device */
int display_open(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_open(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_new(handle, device);	/* create and add to list */
    img->hmutex = CreateMutex(NULL, FALSE, NULL);
    if (img)
	PostThreadMessage(thread_id, DISPLAY_OPEN, 0, (LPARAM)img);
    return 0;
}

int display_preclose(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_preclose(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img) {
	/* grab mutex to stop other thread using bitmap */
	WaitForSingleObject(img->hmutex, 120000);
    }
    return 0;
}

int display_close(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_close(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img) {
	/* This is a hack to pass focus from image window to console */
	if (GetForegroundWindow() == img->hwnd)
	    SetForegroundWindow(hwndforeground);

	image_delete(img);	/* remove from list, but don't free */
	PostThreadMessage(thread_id, DISPLAY_CLOSE, 0, (LPARAM)img);
    }
    return 0;
}

int display_presize(void *handle, void *device, int width, int height, 
	int raster, unsigned int format)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_presize(0x%x 0x%x, %d, %d, %d, %d, %ld)\n",
	handle, device, width, height, raster, format);
#endif
    img = image_find(handle, device);
    if (img) {
	/* grab mutex to stop other thread using bitmap */
	WaitForSingleObject(img->hmutex, 120000);
    }
    return 0;
}
   
int display_size(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_size(0x%x 0x%x, %d, %d, %d, %d, %ld, 0x%x)\n",
	handle, device, width, height, raster, format, pimage);
#endif
    img = image_find(handle, device);
    if (img) {
	image_size(img, width, height, raster, format, pimage);
	/* release mutex to allow other thread to use bitmap */
	ReleaseMutex(img->hmutex);
	PostThreadMessage(thread_id, DISPLAY_SIZE, 0, (LPARAM)img);
    }
    return 0;
}
   
int display_sync(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_sync(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img)
	PostThreadMessage(thread_id, DISPLAY_SYNC, 0, (LPARAM)img);
    return 0;
}

int display_page(void *handle, void *device, int copies, int flush)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_page(0x%x, 0x%x, copies=%d, flush=%d)\n", 
	handle, device, copies, flush);
#endif
    img = image_find(handle, device);
    if (img)
	PostThreadMessage(thread_id, DISPLAY_PAGE, 0, (LPARAM)img);
    return 0;
}

int display_update(void *handle, void *device, 
    int x, int y, int w, int h)
{
    /* Unneeded for polling - we are running Windows on another thread. */
    /* Eventually we will add code here which provides progressive 
     * update of the display during rendering.
     */
    return 0;
}

/*
#define DISPLAY_DEBUG_USE_ALLOC
*/
#ifdef DISPLAY_DEBUG_USE_ALLOC
/* This code isn't used, but shows how to use this function */
void *display_memalloc(void *handle, void *device, unsigned long size)
{
    void *mem;
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_memalloc(0x%x 0x%x %d)\n", 
	handle, device, size);
#endif
    mem = malloc(size);
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "  returning 0x%x\n", (int)mem);
#endif
    return mem;
}

int display_memfree(void *handle, void *device, void *mem)
{
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_memfree(0x%x, 0x%x, 0x%x)\n", 
	handle, device, mem);
#endif
    free(mem);
    return 0;
}
#endif



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
#ifdef DISPLAY_DEBUG_USE_ALLOC
    display_memalloc,	/* memalloc */
    display_memfree	/* memfree */
#else
    NULL,	/* memalloc */
    NULL	/* memfree */
#endif
};


/*********************************************************************/

int main(int argc, char *argv[])
{
    int code;
    int exit_code;
    int exit_status;
    int nargc;
    char **nargv;
    char buf[256];
    char dformat[64];

    if (!_isatty(fileno(stdin)))
        _setmode(fileno(stdin), _O_BINARY);
    _setmode(fileno(stdout), _O_BINARY);
    _setmode(fileno(stderr), _O_BINARY);

    hwndforeground = GetForegroundWindow();	/* assume this is ours */
    memset(buf, 0, sizeof(buf));
    if (load_dll(&gsdll, buf, sizeof(buf))) {
	fprintf(stderr, "Can't load Ghostscript DLL\n");
	fprintf(stderr, "%s\n", buf);
	return 1;
    }

    if (gsdll.new_instance(&instance, NULL) < 0) {
	fprintf(stderr, "Can't create Ghostscript instance\n");
	return 1;
    }

    if (_beginthread(winthread, 65535, NULL) == -1) {
	fprintf(stderr, "GUI thread creation failed\n");
    }
    else {
	int n = 30;
	/* wait for thread to start */
	Sleep(0);
	while (n && (hthread == INVALID_HANDLE_VALUE)) {
	    n--;
	    Sleep(100);
        }
	while (n && (PostThreadMessage(thread_id, WM_USER, 0, 0) == 0)) {
	    n--;
	    Sleep(100);
	}
	if (n == 0)
	    fprintf(stderr, "Can't post message to GUI thread\n");
    }

    gsdll.set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
    gsdll.set_display_callback(instance, &display);

    {   int format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_1 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	HDC hdc = GetDC(NULL);	/* get hdc for desktop */
	int depth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
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
    nargc = argc + 1;
    nargv = (char **)malloc((nargc + 1) * sizeof(char *));
    nargv[0] = argv[0];
    nargv[1] = dformat;
    memcpy(&nargv[2], &argv[1], argc * sizeof(char *));

    code = gsdll.init_with_args(instance, nargc, nargv);
    if (code == 0)
	code = gsdll.run_string(instance, start_string, 0, &exit_code);
    gsdll.exit(instance);

    gsdll.delete_instance(instance);

    unload_dll(&gsdll);

    free(nargv);

    /* close other thread */
    quitnow = TRUE; 
    PostThreadMessage(thread_id, WM_QUIT, 0, (LPARAM)0);
    Sleep(0);

    exit_status = 0;
    switch (code) {
	case 0:
	case e_Info:
	case e_Quit:
	    break;
	case e_Fatal:
	    exit_status = 1;
	    break;
	default:
	    exit_status = 255;
    }


    return exit_status;
}

