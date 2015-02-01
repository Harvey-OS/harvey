/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1990, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: btoken.h,v 1.7 2002/06/16 04:47:10 lpd Exp $ */
/* Definitions for Level 2 binary tokens */

#ifndef btoken_INCLUDED
#  define btoken_INCLUDED

/*
 * Define accessors for pointers to the system and user name tables
 * (arrays).  Note that these refer implicitly to i_ctx_p.  Note also
 * that these pointers may be NULL: clients must check this.
 */
#define system_names_p (gs_imemory.space_global->names_array)
#define user_names_p (gs_imemory.space_local->names_array)

/* Create a system or user name table (in the stable memory of mem). */
int create_names_array(ref **ppnames, gs_memory_t *mem,
		       client_name_t cname); /* in zbseq.c */

/* Convert an object to its representation in a binary object sequence. */
int encode_binary_token(i_ctx_t *i_ctx_p, const ref *obj, long *ref_offset,
			long *char_offset, byte *str); /* in iscanbin.c */

/* Define the current binary object format for operators. */
/* This is a ref so that it can be managed properly by save/restore. */
#define ref_binary_object_format_container i_ctx_p
#define ref_binary_object_format\
  (ref_binary_object_format_container->binary_object_format)

#endif /* btoken_INCLUDED */
