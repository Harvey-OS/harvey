/* Copyright (C) 1996, 2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
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


/* $Id: dpmain.c,v 1.12 2004/08/19 21:52:20 ghostgum Exp $ */
/* Ghostscript DLL loader for OS/2 */
/* For WINDOWCOMPAT (console mode) application */

/* Russell Lang  1996-06-05 */

/* Updated 2001-03-10 by rjl
 *  New DLL interface, uses display device.
 *  Uses same interface to gspmdrv.c as os2pm device.
 */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN	/* to get bits/pixel of display */
#define INCL_GPI	/* to get bits/pixel of display */
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include "gscdefs.h"
#define GS_REVISION gs_revision
#include "ierrors.h"
#include "iapi.h"
#include "gdevdsp.h"

#define MAXSTR 256
#define BITMAPINFO2_SIZE 40
const char *szDllName = "GSDLL2.DLL";
char start_string[] = "systemdict /start get exec\n";
int debug = TRUE /* FALSE */;


#define MIN_COMMIT 4096		/* memory is committed in these size chunks */
#define ID_NAME "GSPMDRV_%u_%u"
#define SHARED_NAME "\\SHAREMEM\\%s"
#define SYNC_NAME   "\\SEM32\\SYNC_%s"
#define MUTEX_NAME  "\\SEM32\\MUTEX_%s"

LONG display_planes;
LONG display_bitcount;
LONG display_hasPalMan;
ULONG os_version;

/* main structure with info about the GS DLL */
typedef struct tagGSDLL {
	HMODULE hmodule;	/* DLL module handle */
	PFN_gsapi_revision revision;
	PFN_gsapi_new_instance new_instance;
	PFN_gsapi_delete_instance delete_instance;
	PFN_gsapi_set_stdio set_stdio;
	PFN_gsapi_set_poll set_poll;
	PFN_gsapi_set_display_callback set_display_callback;
	PFN_gsapi_init_with_args init_with_args;
	PFN_gsapi_run_string run_string;
	PFN_gsapi_exit exit;
} GSDLL;

GSDLL gsdll;
void *instance;
TID tid;

void
gs_addmess(char *str)
{
    fputs(str, stdout);
    fflush(stdout);
}

/*********************************************************************/
/* load and unload the Ghostscript DLL */

/* free GS DLL */
/* This should only be called when gsdll_execute has returned */
/* TRUE means no error */
BOOL
gs_free_dll(void)
{
    char buf[MAXSTR];
    APIRET rc;

    if (gsdll.hmodule == (HMODULE) NULL)
	return TRUE;
    rc = DosFreeModule(gsdll.hmodule);
    if (rc) {
	sprintf(buf, "DosFreeModule returns %d\n", rc);
	gs_addmess(buf);
	sprintf(buf, "Unloaded GSDLL\n\n");
	gs_addmess(buf);
    }
    return !rc;
}

void
gs_load_dll_cleanup(void)
{
    char buf[MAXSTR];

    gs_free_dll();
}

/* load GS DLL if not already loaded */
/* return TRUE if OK */
BOOL
gs_load_dll(void)
{
    char buf[MAXSTR + 40];
    APIRET rc;
    char *p;
    int i;
    const char *dllname;
    PTIB pptib;
    PPIB pppib;
    char szExePath[MAXSTR];
    char fullname[1024];
    const char *shortname;
    gsapi_revision_t rv;

    if ((rc = DosGetInfoBlocks(&pptib, &pppib)) != 0) {
	fprintf(stdout, "Couldn't get pid, rc = \n", rc);
	return FALSE;
    }
    /* get path to EXE */
    if ((rc = DosQueryModuleName(pppib->pib_hmte, sizeof(szExePath), 
	szExePath)) != 0) {
	fprintf(stdout, "Couldn't get module name, rc = %d\n", rc);
	return FALSE;
    }
    if ((p = strrchr(szExePath, '\\')) != (char *)NULL) {
	p++;
	*p = '\0';
    }
    dllname = szDllName;
#ifdef DEBUG
    if (debug) {
	sprintf(buf, "Trying to load %s\n", dllname);
	gs_addmess(buf);
    }
#endif
    memset(buf, 0, sizeof(buf));
    rc = DosLoadModule(buf, sizeof(buf), dllname, &gsdll.hmodule);
    if (rc) {
	/* failed */
	/* try again, with path of EXE */
	if ((shortname = strrchr((char *)szDllName, '\\')) 
	    == (const char *)NULL)
	    shortname = szDllName;
	strcpy(fullname, szExePath);
	if ((p = strrchr(fullname, '\\')) != (char *)NULL)
	    p++;
	else
	    p = fullname;
	*p = '\0';
	strcat(fullname, shortname);
	dllname = fullname;
#ifdef DEBUG
	if (debug) {
	    sprintf(buf, "Trying to load %s\n", dllname);
	    gs_addmess(buf);
	}
#endif
	rc = DosLoadModule(buf, sizeof(buf), dllname, &gsdll.hmodule);
	if (rc) {
	    /* failed again */
	    /* try once more, this time on system search path */
	    dllname = shortname;
#ifdef DEBUG
	    if (debug) {
		sprintf(buf, "Trying to load %s\n", dllname);
		gs_addmess(buf);
	    }
#endif
	    rc = DosLoadModule(buf, sizeof(buf), dllname, &gsdll.hmodule);
	}
    }
    if (rc == 0) {
#ifdef DEBUG
	if (debug)
	    gs_addmess("Loaded Ghostscript DLL\n");
#endif
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_REVISION", 
		(PFN *) (&gsdll.revision))) != 0) {
	    sprintf(buf, "Can't find GSAPI_REVISION, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	/* check DLL version */
	if (gsdll.revision(&rv, sizeof(rv)) != 0) {
	    sprintf(buf, "Unable to identify Ghostscript DLL revision - it must be newer than needed.\n");
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}

	if (rv.revision != GS_REVISION) {
	    sprintf(buf, "Wrong version of DLL found.\n  Found version %ld\n  Need version  %ld\n", rv.revision, (long)GS_REVISION);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}

	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_NEW_INSTANCE", 
		(PFN *) (&gsdll.new_instance))) != 0) {
	    sprintf(buf, "Can't find GSAPI_NEW_INSTANCE, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_DELETE_INSTANCE", 
		(PFN *) (&gsdll.delete_instance))) != 0) {
	    sprintf(buf, "Can't find GSAPI_DELETE_INSTANCE, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_SET_STDIO", 
		(PFN *) (&gsdll.set_stdio))) != 0) {
	    sprintf(buf, "Can't find GSAPI_SET_STDIO, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_SET_DISPLAY_CALLBACK", 
		(PFN *) (&gsdll.set_display_callback))) != 0) {
	    sprintf(buf, "Can't find GSAPI_SET_DISPLAY_CALLBACK, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_SET_POLL", 
		(PFN *) (&gsdll.set_poll))) != 0) {
	    sprintf(buf, "Can't find GSAPI_SET_POLL, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, 
		"GSAPI_INIT_WITH_ARGS", 
		(PFN *) (&gsdll.init_with_args))) != 0) {
	    sprintf(buf, "Can't find GSAPI_INIT_WITH_ARGS, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_RUN_STRING", 
		(PFN *) (&gsdll.run_string))) != 0) {
	    sprintf(buf, "Can't find GSAPI_RUN_STRING, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
	if ((rc = DosQueryProcAddr(gsdll.hmodule, 0, "GSAPI_EXIT", 
		(PFN *) (&gsdll.exit))) != 0) {
	    sprintf(buf, "Can't find GSAPI_EXIT, rc = %d\n", rc);
	    gs_addmess(buf);
	    gs_load_dll_cleanup();
	    return FALSE;
	}
    } else {
	sprintf(buf, "Can't load Ghostscript DLL %s \nDosLoadModule rc = %d\n",
	    szDllName, rc);
	gs_addmess(buf);
	gs_load_dll_cleanup();
	return FALSE;
    }
    return TRUE;
}


/*********************************************************************/
/* stdio functions */

static int 
gsdll_stdin(void *instance, char *buf, int len)
{
    return read(fileno(stdin), buf, len);
}

static int 
gsdll_stdout(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stdout);
    fflush(stdout);
    return len;
}

static int 
gsdll_stderr(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stderr);
    fflush(stderr);
    return len;
}

/*********************************************************************/
/* display device */

/*
#define DISPLAY_DEBUG
*/

typedef struct IMAGE_S IMAGE;
struct IMAGE_S {
    void *handle;
    void *device;
    PID pid;		/* PID of our window (CMD.EXE) */
    HEV sync_event;	/* tell gspmdrv to redraw window */
    HMTX bmp_mutex;	/* protects access to bitmap */
    HQUEUE term_queue;	/* notification that gspmdrv has finished */
    ULONG session_id;	/* id of gspmdrv */
    PID process_id;	/* of gspmdrv */

    int width;
    int height;
    int raster;
    int format;

    BOOL format_known;

    unsigned char *bitmap;
    ULONG committed;
    IMAGE *next;
};

IMAGE *first_image = NULL;
static IMAGE *image_find(void *handle, void *device);

static IMAGE *
image_find(void *handle, void *device)
{
    IMAGE *img;
    for (img = first_image; img!=0; img=img->next) {
	if ((img->handle == handle) && (img->device == device))
	    return img;
    }
    return NULL;
}


/* start gspmdrv.exe */
static int run_gspmdrv(IMAGE *img)
{
    int ccode;
    PCHAR pdrvname = "gspmdrv.exe";
    CHAR error_message[256];
    CHAR term_queue_name[128];
    CHAR id[128];
    CHAR arg[1024];
    STARTDATA sdata;
    APIRET rc;
    PTIB pptib;
    PPIB pppib;
    CHAR progname[256];
    PCHAR tail;

#ifdef DEBUG
    if (debug)
	fprintf(stdout, "run_gspmdrv: starting\n");
#endif
    sprintf(id, ID_NAME, img->pid, (ULONG)img->device);

    /* Create termination queue - used to find out when gspmdrv terminates */
    sprintf(term_queue_name, "\\QUEUES\\TERMQ_%s", id);
    if (DosCreateQueue(&(img->term_queue), QUE_FIFO, term_queue_name)) {
	fprintf(stdout, "run_gspmdrv: failed to create termination queue\n");
	return e_limitcheck;
    }
    /* get full path to gsos2.exe and hence path to gspmdrv.exe */
    if ((rc = DosGetInfoBlocks(&pptib, &pppib)) != 0) {
	fprintf(stdout, "run_gspmdrv: Couldn't get module handle, rc = %d\n", 
	    rc);
	return e_limitcheck;
    }
    if ((rc = DosQueryModuleName(pppib->pib_hmte, sizeof(progname) - 1, 
	progname)) != 0) {
	fprintf(stdout, "run_gspmdrv: Couldn't get module name, rc = %d\n", 
	    rc);
	return e_limitcheck;
    }
    if ((tail = strrchr(progname, '\\')) != (PCHAR) NULL) {
	tail++;
	*tail = '\0';
    } else
	tail = progname;
    strcat(progname, pdrvname);

    /* Open the PM driver session gspmdrv.exe */
    /* arguments are: */
    /*  (1) -d (display) option */
    /*  (2) id string */
    sprintf(arg, "-d %s", id);

    /* because gspmdrv.exe is a different EXE type to gs.exe, 
     * we must use start session not DosExecPgm() */
    sdata.Length = sizeof(sdata);
    sdata.Related = SSF_RELATED_CHILD;	/* to be a child  */
    sdata.FgBg = SSF_FGBG_BACK;	/* start in background */
    sdata.TraceOpt = 0;
    sdata.PgmTitle = "Ghostscript PM driver session";
    sdata.PgmName = progname;
    sdata.PgmInputs = arg;
    sdata.TermQ = term_queue_name;
    sdata.Environment = pppib->pib_pchenv;	/* use Parent's environment */
    sdata.InheritOpt = 0;	/* Can't inherit from parent because */
				/* different sesison type */
    sdata.SessionType = SSF_TYPE_DEFAULT;	/* default is PM */
    sdata.IconFile = NULL;
    sdata.PgmHandle = 0;
    sdata.PgmControl = 0;
    sdata.InitXPos = 0;
    sdata.InitYPos = 0;
    sdata.InitXSize = 0;
    sdata.InitYSize = 0;
    sdata.ObjectBuffer = error_message;
    sdata.ObjectBuffLen = sizeof(error_message);

    rc = DosStartSession(&sdata, &img->session_id, &img->process_id);
    if (rc == ERROR_FILE_NOT_FOUND) {
	sdata.PgmName = pdrvname;
	rc = DosStartSession(&sdata, &img->session_id, &img->process_id);
    }
    if (rc) {
	fprintf(stdout, "run_gspmdrv: failed to run %s, rc = %d\n", 
	    sdata.PgmName, rc);
	fprintf(stdout, "run_gspmdrv: error_message: %s\n", error_message);
	return e_limitcheck;
    }
#ifdef DEBUG
    if (debug)
        fprintf(stdout, "run_gspmdrv: returning\n");
#endif
    return 0;
}

void
image_color(unsigned int format, int index, 
    unsigned char *r, unsigned char *g, unsigned char *b)
{
    switch (format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    *r = *g = *b = (index ? 0 : 255);
		    break;
		case DISPLAY_DEPTH_4:
		    if (index == 7)
			*r = *g = *b = 170;
		    else if (index == 8)
			*r = *g = *b = 85;
		    else {
			int one = index & 8 ? 255 : 128;
			*r = (index & 4 ? one : 0);
			*g = (index & 2 ? one : 0);
			*b = (index & 1 ? one : 0);
		    }
		    break;
		case DISPLAY_DEPTH_8:
		    /* palette of 96 colors */
		    /* 0->63 = 00RRGGBB, 64->95 = 010YYYYY */
		    if (index < 64) {
			int one = 255 / 3;
			*r = ((index & 0x30) >> 4) * one;
			*g = ((index & 0x0c) >> 2) * one;
			*b =  (index & 0x03) * one;
		    }
		    else {
			int val = index & 0x1f;
			*r = *g = *b = (val << 3) + (val >> 2);
		    }
		    break;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    *r = *g = *b = (index ? 255 : 0);
		    break;
		case DISPLAY_DEPTH_4:
		    *r = *g = *b = (unsigned char)((index<<4) + index);
		    break;
		case DISPLAY_DEPTH_8:
		    *r = *g = *b = (unsigned char)index;
		    break;
	    }
	    break;
    }
}


static int image_palette_size(int format)
{
    int palsize = 0;
    switch (format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    palsize = 2;
		    break;
		case DISPLAY_DEPTH_4:
		    palsize = 16;
		    break;
		case DISPLAY_DEPTH_8:
		    palsize = 96;
		    break;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    palsize = 2;
		    break;
		case DISPLAY_DEPTH_4:
		    palsize = 16;
		    break;
		case DISPLAY_DEPTH_8:
		    palsize = 256;
		    break;
	    }
	    break;
    }
    return palsize;
}

/* New device has been opened */
/* Tell user to use another device */
int display_open(void *handle, void *device)
{
    APIRET rc;
    IMAGE *img;
    PTIB pptib;
    PPIB pppib;
    CHAR id[128];
    CHAR name[128];
    PBITMAPINFO2 bmi;

#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('o', stdout);
    fprintf(stdout, "display_open(0x%x, 0x%x)\n", handle, device);
#endif

    if (first_image) {
	/* gsos2.exe is a console application, and displays using
	 * gspmdrv.exe which is a PM application.  To start 
	 * gspmdrv.exe, DosStartSession is used with SSF_RELATED_CHILD.  
	 * A process can have only one child session marked SSF_RELATED_CHILD.  
	 * When we call DosStopSession for the second session, it will 
	 * close, but it will not write to the termination queue.  
	 * When we wait for the session to end by reading the 
	 * termination queue, we wait forever.
	 * For this reason, multiple image windows are disabled
	 * for OS/2.
	 * To get around this, we would need to replace the current
	 * method of one gspmdrv.exe session per window, to having
	 * a new PM application which can display multiple windows
	 * within a single session.
	 */
	return e_limitcheck;
    }

    img = (IMAGE *)malloc(sizeof(IMAGE));
    if (img == NULL)
	return e_limitcheck;
    memset(img, 0, sizeof(IMAGE));

    /* add to list */
    img->next = first_image;
    first_image = img;

    /* remember device and handle */
    img->handle = handle;
    img->device = device;

    /* Derive ID from process ID */
    if (DosGetInfoBlocks(&pptib, &pppib)) {
	fprintf(stdout, "\ndisplay_open: Couldn't get pid\n");
	return e_limitcheck;
    }
    img->pid = pppib->pib_ulppid;	/* use parent (CMD.EXE) pid */
    sprintf(id, ID_NAME, img->pid, (ULONG) img->device);

    /* Create update event semaphore */
    sprintf(name, SYNC_NAME, id);
    if (DosCreateEventSem(name, &(img->sync_event), 0, FALSE)) {
	fprintf(stdout, "display_open: failed to create event semaphore %s\n", name);
	return e_limitcheck;
    }
    /* Create mutex - used for preventing gspmdrv from accessing */
    /* bitmap while we are changing the bitmap size. Initially unowned. */
    sprintf(name, MUTEX_NAME, id);
    if (DosCreateMutexSem(name, &(img->bmp_mutex), 0, FALSE)) {
	DosCloseEventSem(img->sync_event);
	fprintf(stdout, "display_open: failed to create mutex semaphore %s\n", name);
	return e_limitcheck;
    }

    /* Shared memory is common to all processes so we don't want to 
     * allocate too much.
     */
    sprintf(name, SHARED_NAME, id);
    if (DosAllocSharedMem((PPVOID) & img->bitmap, name,
		      13 * 1024 * 1024, PAG_READ | PAG_WRITE)) {
	fprintf(stdout, "display_open: failed allocating shared BMP memory %s\n", name);
	return e_limitcheck;
    }

    /* commit one page so there is enough storage for a */
    /* bitmap header and palette */
    if (DosSetMem(img->bitmap, MIN_COMMIT, PAG_COMMIT | PAG_DEFAULT)) {
	DosFreeMem(img->bitmap);
	fprintf(stdout, "display: failed committing BMP memory\n");
	return e_limitcheck;
    }
    img->committed = MIN_COMMIT;

    /* write a zero pixel BMP */
    bmi = (PBITMAPINFO2) img->bitmap;
    bmi->cbFix = BITMAPINFO2_SIZE; /* OS/2 2.0 and Windows 3.0 compatible */
    bmi->cx = 0;
    bmi->cy = 0;
    bmi->cPlanes = 1;
    bmi->cBitCount = 24;
    bmi->ulCompression = BCA_UNCOMP;
    bmi->cbImage = 0;
    bmi->cxResolution = 0;
    bmi->cyResolution = 0;
    bmi->cclrUsed = 0;
    bmi->cclrImportant = 0;

    /* delay start of gspmdrv until size is known */

#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('O', stdout);
#endif
    return 0;
}

int display_preclose(void *handle, void *device)
{
    IMAGE *img;
    REQUESTDATA Request;
    ULONG DataLength;
    PVOID DataAddress;
    PULONG QueueEntry;
    BYTE ElemPriority;
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('l', stdout);
    fprintf(stdout, "display_preclose(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img) {
 	if (img->session_id) {
	    /* Close gspmdrv driver */
	    DosStopSession(STOP_SESSION_SPECIFIED, img->session_id);
	    Request.pid = img->pid;
	    Request.ulData = 0;
	    /* wait for termination queue, queue is then closed */
	    /* by session manager */
	    DosReadQueue(img->term_queue, &Request, &DataLength,
		     &DataAddress, 0, DCWW_WAIT, &ElemPriority, (HEV) NULL);
	    /* queue needs to be closed by us */
	    DosCloseQueue(img->term_queue);
	}
	img->session_id = 0;
	img->term_queue = 0;

	DosCloseEventSem(img->sync_event);
	DosCloseMutexSem(img->bmp_mutex);
    }
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('L', stdout);
#endif
    return 0;
}

int display_close(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('c', stdout);
    fprintf(stdout, "display_close(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img) {
	/* gspmdrv was closed by display_preclose */
	/* release memory */
	DosFreeMem(img->bitmap);
	img->bitmap = (unsigned char *)NULL;
	img->committed = 0;
    }
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('C', stdout);
#endif
    return 0;
}

int display_presize(void *handle, void *device, int width, int height, 
	int raster, unsigned int format)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('r', stdout);
    fprintf(stdout, "display_presize(0x%x 0x%x, %d, %d, %d, %d)\n",
	handle, device, width, height, raster, format);
#endif
    img = image_find(handle, device);
    if (img) {
	int color = format & DISPLAY_COLORS_MASK;
	int depth = format & DISPLAY_DEPTH_MASK;
	int alpha = format & DISPLAY_ALPHA_MASK;
	img->format_known = FALSE;
	if ( ((color == DISPLAY_COLORS_NATIVE) || 
	      (color == DISPLAY_COLORS_GRAY))
		 &&
	     ((depth == DISPLAY_DEPTH_1) ||
	      (depth == DISPLAY_DEPTH_4) ||
	      (depth == DISPLAY_DEPTH_8)) )
	    img->format_known = TRUE;
	if ((color == DISPLAY_COLORS_RGB) && (depth == DISPLAY_DEPTH_8) &&
	    (alpha == DISPLAY_ALPHA_NONE))
	    img->format_known = TRUE;
	if (!img->format_known) {
	    fprintf(stdout, "display_presize: format %d = 0x%x is unsupported\n", format, format);
	    return e_limitcheck;
	}
	/* grab mutex to stop other thread using bitmap */
	DosRequestMutexSem(img->bmp_mutex, 120000);
	/* remember parameters so we can figure out where to allocate bitmap */
	img->width = width;
	img->height = height;
	img->raster = raster;
	img->format = format;
    }
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('R', stdout);
#endif
    return 0;
}
   
int display_size(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage)
{
    IMAGE *img;
    PBITMAPINFO2 bmi;
    int nColors;
    int i;
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('z', stdout);
    fprintf(stdout, "display_size(0x%x 0x%x, %d, %d, %d, %d, %d, 0x%x)\n",
	handle, device, width, height, raster, format, pimage);
#endif
    img = image_find(handle, device);
    if (img) {
	if (!img->format_known)
	    return e_limitcheck;

	img->width = width;
	img->height = height;
	img->raster = raster;
	img->format = format;
	/* write BMP header including palette */
	bmi = (PBITMAPINFO2) img->bitmap;
	bmi->cbFix = BITMAPINFO2_SIZE;
	bmi->cx = img->width;
	bmi->cy = img->height;
	bmi->cPlanes = 1;
	bmi->cBitCount = 24;
	bmi->ulCompression = BCA_UNCOMP;
	bmi->cbImage = 0;
	bmi->cxResolution = 0;
	bmi->cyResolution = 0;
	bmi->cclrUsed = bmi->cclrImportant = image_palette_size(format);

	switch (img->format & DISPLAY_DEPTH_MASK) {
	    default:
	    case DISPLAY_DEPTH_1:
		bmi->cBitCount = 1;
		break;
	    case DISPLAY_DEPTH_4:
		bmi->cBitCount = 4;
		break;
	    case DISPLAY_DEPTH_8:
		if ((img->format & DISPLAY_COLORS_MASK) == DISPLAY_COLORS_NATIVE)
		    bmi->cBitCount = 8;
		else if ((img->format & DISPLAY_COLORS_MASK) == DISPLAY_COLORS_GRAY)
		    bmi->cBitCount = 8;
		else
		    bmi->cBitCount = 24;
		break;
	}

	/* add palette if needed */
	nColors = bmi->cclrUsed;
	if (nColors) {
	    unsigned char *p;
	    p = img->bitmap + BITMAPINFO2_SIZE;
	    for (i = 0; i < nColors; i++) {
		image_color(img->format, i, p+2, p+1, p);
		*(p+3) = 0;
		p += 4;
	    }
	}

	/* release mutex to allow other thread to use bitmap */
	DosReleaseMutexSem(img->bmp_mutex);
    }
#ifdef DISPLAY_DEBUG
    if (debug) {
    	fprintf(stdout, "\nBMP dump\n");
    	fprintf(stdout, " bitmap=%lx\n", img->bitmap);
    	fprintf(stdout, " committed=%lx\n", img->committed);
    	fprintf(stdout, " cx=%d\n", bmi->cx);
    	fprintf(stdout, " cy=%d\n", bmi->cy);
    	fprintf(stdout, " cPlanes=%d\n", bmi->cPlanes);
    	fprintf(stdout, " cBitCount=%d\n", bmi->cBitCount);
    	fprintf(stdout, " ulCompression=%d\n", bmi->ulCompression);
    	fprintf(stdout, " cbImage=%d\n", bmi->cbImage);
    	fprintf(stdout, " cxResolution=%d\n", bmi->cxResolution);
    	fprintf(stdout, " cyResolution=%d\n", bmi->cyResolution);
    	fprintf(stdout, " cclrUsed=%d\n", bmi->cclrUsed);
    	fprintf(stdout, " cclrImportant=%d\n", bmi->cclrImportant);
    }
    if (debug)
	fputc('Z', stdout);
#endif
    return 0;
}
   
int display_sync(void *handle, void *device)
{
    IMAGE *img;
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('s', stdout);
    fprintf(stdout, "display_sync(0x%x, 0x%x)\n", handle, device);
#endif
    img = image_find(handle, device);
    if (img) {
	if (!img->format_known)
	    return e_limitcheck;
	/* delay starting gspmdrv until display_size has been called */
	if (!img->session_id && (img->width != 0) && (img->height != 0))
	   run_gspmdrv(img);
	DosPostEventSem(img->sync_event);
    }
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('S', stdout);
#endif
    return 0;
}

int display_page(void *handle, void *device, int copies, int flush)
{
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('p', stdout);
    fprintf(stdout, "display_page(0x%x, 0x%x, copies=%d, flush=%d)\n", 
	handle, device, copies, flush);
#endif
    display_sync(handle, device);
#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('P', stdout);
#endif
    return 0;
}

void *display_memalloc(void *handle, void *device, unsigned long size)
{
    IMAGE *img;
    unsigned long needed;
    unsigned long header;
    APIRET rc;
    void *mem = NULL;

#ifdef DISPLAY_DEBUG
    if (debug)
	fputc('m', stdout);
    fprintf(stdout, "display_memalloc(0x%x 0x%x %d)\n", 
	handle, device, size);
#endif
    img = image_find(handle, device);
    if (img) {
	/* we don't actually allocate memory here, we only commit
	 * preallocated shared memory.
	 * First work out size of header + palette.
	 * We allocate space for the header and tell Ghostscript
	 * that the memory starts just after the header.
	 * We rely on the Ghostscript memory device placing the
	 * raster at the start of this memory and having a
	 * raster length the same as the length of a BMP row.
         */
	header = BITMAPINFO2_SIZE + image_palette_size(img->format) * 4;

	/* Work out if we need to commit more */
	needed = (size + header + MIN_COMMIT - 1) & (~(MIN_COMMIT - 1));
	if (needed > img->committed) {
	    /* commit more memory */
	    if (rc = DosSetMem(img->bitmap + img->committed,
			   needed - img->committed,
			   PAG_COMMIT | PAG_DEFAULT)) {
		fprintf(stdout, "No memory in display_memalloc rc = %d\n", rc);
		return NULL;
	    }
	    img->committed = needed;
	}
        mem = img->bitmap + header;
    }
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "  returning 0x%x\n", (int)mem);
    if (debug)
	fputc('M', stdout);
#endif
    return mem;
}

int display_memfree(void *handle, void *device, void *mem)
{
    /* we can't uncommit shared memory, so do nothing */
    /* memory will be released when device is closed */
#ifdef DISPLAY_DEBUG
    fprintf(stdout, "display_memfree(0x%x, 0x%x, 0x%x)\n", 
	handle, device, mem);
#endif
}

int display_update(void *handle, void *device, 
    int x, int y, int w, int h)
{
    /* unneeded - we are running image window in a separate process */
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
    display_memalloc,
    display_memfree
};


/*********************************************************************/


int
main(int argc, char *argv[])
{
    int code, code1;
    int exit_code;
    int exit_status;
    int nargc;
    char **nargv;
    char dformat[64];
    ULONG version[3];
    void *instance;

    if (DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_REVISION, 
	    &version, sizeof(version)))
	os_version = 201000;	/* a guess */
    else
	os_version = version[0] * 10000 + version[1] * 100 + version[2];

    if (!gs_load_dll()) {
	fprintf(stdout, "Can't load %s\n", szDllName);
	return -1;
    }

    /* insert -dDisplayFormat=XXXXX as first argument */
    {   int format = DISPLAY_COLORS_NATIVE | DISPLAY_ALPHA_NONE | 
		DISPLAY_DEPTH_1 | DISPLAY_LITTLEENDIAN | DISPLAY_BOTTOMFIRST;
	int depth;
	HPS ps = WinGetPS(HWND_DESKTOP);
	HDC hdc = GpiQueryDevice(ps);
	DevQueryCaps(hdc, CAPS_COLOR_PLANES, 1, &display_planes);
	DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1, &display_bitcount);
	DevQueryCaps(hdc, CAPS_ADDITIONAL_GRAPHICS, 1, &display_hasPalMan);
	display_hasPalMan &= CAPS_PALETTE_MANAGER;
  	depth = display_planes * display_bitcount;
	if ((depth <= 8) && !display_hasPalMan)
	    depth = 24;		/* disaster: limited colours and no palette */ 
	WinReleasePS(ps);

	if (depth > 8)
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


#ifdef DEBUG
    if (debug) 
	fprintf(stdout, "%s\n", dformat);
#endif
    nargc = argc + 1;
    nargv = (char **)malloc((nargc + 1) * sizeof(char *));
    nargv[0] = argv[0];
    nargv[1] = dformat;
    memcpy(&nargv[2], &argv[1], argc * sizeof(char *));

    if ( (code = gsdll.new_instance(&instance, NULL)) == 0) {
	gsdll.set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
	gsdll.set_display_callback(instance, &display);

	code = gsdll.init_with_args(instance, nargc, nargv);
	if (code == 0) 
	    code = gsdll.run_string(instance, start_string, 0, &exit_code);
	code1 = gsdll.exit(instance);
	if (code == 0 || (code == e_Quit && code1 != 0))
	    code = code1;

	gsdll.delete_instance(instance);
    }

    gs_free_dll();

    free(nargv);

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
