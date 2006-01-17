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

/* $Id: iddstack.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Dictionary stack API subset needed by idict.h */

#ifndef iddstack_INCLUDED
#  define iddstack_INCLUDED

#ifndef dict_stack_DEFINED
#  define dict_stack_DEFINED
typedef struct dict_stack_s dict_stack_t;
#endif

/*
 * Reset the cached top values.  Every routine that alters the
 * dictionary stack (including changing the protection or size of the
 * top dictionary on the stack) must call this.
 */
void dstack_set_top(dict_stack_t *);

/* Check whether a dictionary is one of the permanent ones on the d-stack. */
bool dstack_dict_is_permanent(const dict_stack_t *, const ref *);

#endif /* iddstack_INCLUDED */
