/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_wsync.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* MS Windows (Win32) thread / semaphore / monitor implementation */
/* original multi-threading code by John Desrosiers */
#include "malloc_.h"
#include "gserror.h"
#include "gserrors.h"
#include "gpsync.h"
#include "windows_.h"
#include <process.h>

/* ------- Synchronization primitives -------- */

/* Semaphore supports wait/signal semantics */

typedef struct win32_semaphore_s {
    HANDLE handle;		/* returned from CreateSemaphore */
} win32_semaphore;

uint
gp_semaphore_sizeof(void)
{
    return sizeof(win32_semaphore);
}

int	/* if sema <> 0 rets -ve error, 0 ok; if sema == 0, 0 movable, 1 fixed */
gp_semaphore_open(
		  gp_semaphore * sema	/* create semaphore here */
)
{
    win32_semaphore *const winSema = (win32_semaphore *)sema;

    if (winSema) {
	winSema->handle = CreateSemaphore(NULL, 0, max_int, NULL);
	return
	    (winSema->handle != NULL ? 0 :
	     gs_note_error(gs_error_unknownerror));
    } else
	return 0;		/* Win32 semaphores handles may be moved */
}

int
gp_semaphore_close(
		   gp_semaphore * sema	/* semaphore to affect */
)
{
    win32_semaphore *const winSema = (win32_semaphore *)sema;

    if (winSema->handle != NULL)
	CloseHandle(winSema->handle);
    winSema->handle = NULL;
    return 0;
}

int				/* rets 0 ok, -ve error */
gp_semaphore_wait(
		  gp_semaphore * sema	/* semaphore to affect */
)
{
    win32_semaphore *const winSema = (win32_semaphore *)sema;

    return
	(WaitForSingleObject(winSema->handle, INFINITE) == WAIT_OBJECT_0
	 ? 0 : gs_error_unknownerror);
}

int				/* rets 0 ok, -ve error */
gp_semaphore_signal(
		    gp_semaphore * sema	/* semaphore to affect */
)
{
    win32_semaphore *const winSema = (win32_semaphore *)sema;

    return
	(ReleaseSemaphore(winSema->handle, 1, NULL) ? 0 :
	 gs_error_unknownerror);
}


/* Monitor supports enter/leave semantics */

typedef struct win32_monitor_s {
    CRITICAL_SECTION lock;	/* critical section lock */
} win32_monitor;

uint
gp_monitor_sizeof(void)
{
    return sizeof(win32_monitor);
}

int	/* if sema <> 0 rets -ve error, 0 ok; if sema == 0, 0 movable, 1 fixed */
gp_monitor_open(
		gp_monitor * mon	/* create monitor here */
)
{
    win32_monitor *const winMon = (win32_monitor *)mon;

    if (mon) {
	InitializeCriticalSection(&winMon->lock);	/* returns no status */
	return 0;
    } else
	return 1;		/* Win32 critical sections mutsn't be moved */
}

int
gp_monitor_close(
		 gp_monitor * mon	/* monitor to affect */
)
{
    win32_monitor *const winMon = (win32_monitor *)mon;

    DeleteCriticalSection(&winMon->lock);	/* rets no status */
    return 0;
}

int				/* rets 0 ok, -ve error */
gp_monitor_enter(
		 gp_monitor * mon	/* monitor to affect */
)
{
    win32_monitor *const winMon = (win32_monitor *)mon;

    EnterCriticalSection(&winMon->lock);	/* rets no status */
    return 0;
}

int				/* rets 0 ok, -ve error */
gp_monitor_leave(
		 gp_monitor * mon	/* monitor to affect */
)
{
    win32_monitor *const winMon = (win32_monitor *)mon;

    LeaveCriticalSection(&winMon->lock);	/* rets no status */
    return 0;
}

/* --------- Thread primitives ---------- */

typedef struct gp_thread_creation_closure_s {
    gp_thread_creation_callback_t function;	/* function to start */
    void *data;			/* magic data to pass to thread */
} gp_thread_creation_closure;

/* Origin of new threads started by gp_create_thread */
private void
gp_thread_begin_wrapper(
			void *thread_data	/* gp_thread_creation_closure passed as magic data */
)
{
    gp_thread_creation_closure closure;

    closure = *(gp_thread_creation_closure *)thread_data;
    free(thread_data);
    (*closure.function)(closure.data);
    _endthread();
}

/* Call a function on a brand new thread */
int				/* 0 ok, -ve error */
gp_create_thread(
		 gp_thread_creation_callback_t function,	/* function to start */
		 void *data	/* magic data to pass to thread fn */
)
{
    /* Create the magic closure that thread_wrapper gets passed */
    gp_thread_creation_closure *closure =
	(gp_thread_creation_closure *)malloc(sizeof(*closure));

    if (!closure)
	return gs_error_VMerror;
    closure->function = function;
    closure->data = data;

    /*
     * Start thread_wrapper.  The Watcom _beginthread returns (int)(-1) if
     * the call fails.  The Microsoft _beginthread returns -1 (according to
     * the doc, even though the return type is "unsigned long" !!!) if the
     * call fails; we aren't sure what the Borland _beginthread returns.
     * The hack with ~ avoids a source code commitment as to whether the
     * return type is [u]int or [u]long.
     *
     * BEGIN_THREAD is a macro (defined in windows_.h) because _beginthread
     * takes different arguments in Watcom C.
     */
    if (~BEGIN_THREAD(gp_thread_begin_wrapper, 0, closure) != 0)
	return 0;
    return_error(gs_error_unknownerror);
}

