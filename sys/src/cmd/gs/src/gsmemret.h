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

/*$Id: gsmemret.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Interface to retrying memory allocator */

#if !defined(gsmemret_INCLUDED)
#  define gsmemret_INCLUDED

#include "gsmemory.h"

/*
 * This allocator encapsulates another allocator with a closure that is
 * called to attempt to free up memory if an allocation fails.
 * Note that it does not keep track of memory that it acquires:
 * thus free_all with FREE_ALL_DATA is a no-op.
 */
typedef struct gs_memory_retrying_s gs_memory_retrying_t;

/*
 * Define the procedure type for the recovery closure.
 */
typedef enum {
    RECOVER_STATUS_NO_RETRY,
    RECOVER_STATUS_RETRY_OK
} gs_memory_recover_status_t;
typedef gs_memory_recover_status_t (*gs_memory_recover_proc_t)
     (P2(gs_memory_retrying_t *rmem, void *proc_data));

struct gs_memory_retrying_s {
    gs_memory_common;		/* interface outside world sees */
    gs_memory_t *target;	/* allocator to front */
    gs_memory_recover_proc_t recover_proc;
    void *recover_proc_data;
};

/* ---------- Public constructors/destructors ---------- */

/* Initialize a retrying memory manager. */
int gs_memory_retrying_init(P2(
			gs_memory_retrying_t * rmem,	/* allocator to init */
			gs_memory_t * target	/* allocator to wrap */
			));

/* Release a retrying memory manager. */
/* Note that this has no effect on the target. */
void gs_memory_retrying_release(P1(gs_memory_retrying_t *rmem));

/* Set the recovery closure of a retrying memory manager. */
void gs_memory_retrying_set_recover(P3(gs_memory_retrying_t *rmem,
				       gs_memory_recover_proc_t recover_proc,
				       void *recover_proc_data));

/* Get the target of a retrying memory manager. */
gs_memory_t * gs_memory_retrying_target(P1(const gs_memory_retrying_t *rmem));

#endif /*!defined(gsmemret_INCLUDED) */
