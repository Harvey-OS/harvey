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

/* $Id: istkparm.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Parameter structure for expandable stacks of refs */

#ifndef istkparm_INCLUDED
#  define istkparm_INCLUDED

/*
 * Define the structure for stack parameters set at initialization.
 */
/*typedef struct ref_stack_params_s ref_stack_params_t;*/ /* in istack.h */
struct ref_stack_params_s {
    uint bot_guard;		/* # of guard elements below bot */
    uint top_guard;		/* # of guard elements above top */
    uint block_size;		/* size of each block */
    uint data_size;		/* # of data slots in each block */
    ref guard_value;		/* t__invalid or t_operator, */
				/* bottom guard value */
    int underflow_error;	/* error code for underflow */
    int overflow_error;		/* error code for overflow */
    bool allow_expansion;	/* if false, don't expand */
};
#define private_st_ref_stack_params() /* in istack.c */\
  gs_private_st_simple(st_ref_stack_params, ref_stack_params_t,\
    "ref_stack_params_t")

#endif /* istkparm_INCLUDED */
