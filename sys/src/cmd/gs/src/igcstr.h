/* Copyright (C) 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: igcstr.h,v 1.6 2005/09/05 13:58:55 leonardo Exp $ */
/* Internal interface to string garbage collector */

#ifndef igcstr_INCLUDED
#  define igcstr_INCLUDED

/* Exported by ilocate.c for igcstr.c */
chunk_t *gc_locate(const void *, gc_state_t *);

/* Exported by igcstr.c for igc.c */
void gc_strings_set_marks(chunk_t *, bool);
bool gc_string_mark(const byte *, uint, bool, gc_state_t *);
void gc_strings_clear_reloc(chunk_t *);
void gc_strings_set_reloc(chunk_t *);
void gc_strings_compact(chunk_t *);
string_proc_reloc(igc_reloc_string);
const_string_proc_reloc(igc_reloc_const_string);
param_string_proc_reloc(igc_reloc_param_string);

#endif /* igcstr_INCLUDED */
