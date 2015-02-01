/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iesdata.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Generic execution stack structure definition */

#ifndef iesdata_INCLUDED
#  define iesdata_INCLUDED

#include "isdata.h"

/* Define the execution stack structure. */
typedef struct exec_stack_s {

    ref_stack_t stack;		/* the actual execution stack */

/*
 * To improve performance, we cache the currentfile pointer
 * (i.e., `shallow-bind' it in Lisp terminology).  The invariant is as
 * follows: either esfile points to the currentfile slot on the estack
 * (i.e., the topmost slot with an executable file), or it is 0.
 * To maintain the invariant, it is sufficient that whenever a routine
 * pushes or pops anything on the estack, if the object *might* be
 * an executable file, invoke esfile_clear_cache(); alternatively,
 * immediately after pushing an object, invoke esfile_check_cache().
 */
    ref *current_file;

} exec_stack_t;

/*
 * current_file is cleared by garbage collection, so we don't declare it
 * as a pointer.
 */
#define public_st_exec_stack()	/* in interp.c */\
  gs_public_st_suffix_add0(st_exec_stack, exec_stack_t, "exec_stack_t",\
    exec_stack_enum_ptrs, exec_stack_reloc_ptrs, st_ref_stack)
#define st_exec_stack_num_ptrs st_ref_stack_num_ptrs

#endif /* iesdata_INCLUDED */
