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

/* $Id: idosave.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Supporting procedures for 'save' recording. */

#ifndef idosave_INCLUDED
#  define idosave_INCLUDED

/*
 * Save a change that must be undone by restore.  We have to pass the
 * pointer to the containing object to alloc_save_change for two reasons:
 *
 *      - We need to know which VM the containing object is in, so we can
 * know on which chain of saved changes to put the new change.
 *
 *      - We need to know whether the object is an array of refs (which
 * includes dictionaries) or a struct, so we can properly trace and
 * relocate the pointer to it from the change record during garbage
 * collection.
 */
int alloc_save_change(gs_dual_memory_t *dmem, const ref *pcont,
		      ref_packed *ptr, client_name_t cname);
int alloc_save_change_in(gs_ref_memory_t *mem, const ref *pcont,
			 ref_packed *ptr, client_name_t cname);

#endif /* idosave_INCLUDED */
