/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: icontext.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Externally visible context state */
/* Requires iref.h, stdio_.h */

#ifndef icontext_INCLUDED
#  define icontext_INCLUDED

#include "gsstype.h"		/* for extern_st */
#include "icstate.h"

/* Declare the GC descriptor for context states. */
extern_st(st_context_state);

/*
 * Define the procedure for resetting user parameters when switching
 * contexts. This is defined in either zusparam.c or inouparm.c.
 */
extern int set_user_params(P2(i_ctx_t *i_ctx_p, const ref * paramdict));

/* Allocate the state of a context, always in local VM. */
/* If *ppcst == 0, allocate the state object as well. */
int context_state_alloc(P3(gs_context_state_t ** ppcst,
			   const ref *psystem_dict,
			   const gs_dual_memory_t * dmem));

/* Load the state of the interpreter from a context. */
/* The argument is not const because caches may be updated. */
int context_state_load(P1(gs_context_state_t *));

/* Store the state of the interpreter into a context. */
int context_state_store(P1(gs_context_state_t *));

/* Free the contents of the state of a context, always to its local VM. */
/* Return a mask of which of its VMs, if any, we freed. */
int context_state_free(P1(gs_context_state_t *));

#endif /* icontext_INCLUDED */
