/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_nsync.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Dummy thread / semaphore / monitor implementation */
#include "std.h"
#include "gserror.h"
#include "gserrors.h"
#include "gpsync.h"

/* ------- Synchronization primitives -------- */

/* Semaphores */

uint
gp_semaphore_sizeof(void)
{
    return sizeof(gp_semaphore);
}

int
gp_semaphore_open(gp_semaphore * sema)
{
    if (sema)
	*(int *)sema = 0;
    return 0;
}

int
gp_semaphore_close(gp_semaphore * sema)
{
    return 0;
}

int
gp_semaphore_wait(gp_semaphore * sema)
{
    if (*(int *)sema == 0)
	return_error(gs_error_unknownerror); /* no real waiting */
    --(*(int *)sema);
    return 0;
}

int
gp_semaphore_signal(gp_semaphore * sema)
{
    ++(*(int *)sema);
    return 0;
}

/* Monitors */

uint
gp_monitor_sizeof(void)
{
    return sizeof(gp_monitor);
}

int
gp_monitor_open(gp_monitor * mon)
{
    if (mon)
	mon->dummy_ = 0;
    return 0;
}

int
gp_monitor_close(gp_monitor * mon)
{
    return 0;
}

int
gp_monitor_enter(gp_monitor * mon)
{
    if (mon->dummy_ != 0)
	return_error(gs_error_unknownerror);
    mon->dummy_ = mon;
    return 0;
}

int
gp_monitor_leave(gp_monitor * mon)
{
    if (mon->dummy_ != mon)
	return_error(gs_error_unknownerror);
    mon->dummy_ = 0;
    return 0;
}

/* Thread creation */

int
gp_create_thread(gp_thread_creation_callback_t proc, void *proc_data)
{
    return_error(gs_error_unknownerror);
}
