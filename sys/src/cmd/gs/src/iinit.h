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

/* $Id: iinit.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* (Internal) interface to iinit.c */

#ifndef iinit_INCLUDED
#  define iinit_INCLUDED

/*
 * Declare initialization procedures exported by iinit.c for imain.c.
 * These must be executed in the order they are declared below.
 */
int obj_init(i_ctx_t **, gs_dual_memory_t *);
int zop_init(i_ctx_t *);
int op_init(i_ctx_t *);

/*
 * Test whether there are any Level 2 operators in the executable.
 * (This is different from the language level in which the interpreter is
 * actually running: it is only tested during initialization.)
 */
bool gs_have_level2(void);

#endif /* iinit_INCLUDED */
