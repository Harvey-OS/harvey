/* Copyright (C) 1995 Aladdin Enterprises.  All rights reserved.

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

/*$Id: igcstr.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internal interface to string garbage collector */

#ifndef igcstr_INCLUDED
#  define igcstr_INCLUDED

/* Exported by ilocate.c for igcstr.c */
chunk_t *gc_locate(P2(const void *, gc_state_t *));

/* Exported by igcstr.c for igc.c */
void gc_strings_set_marks(P2(chunk_t *, bool));
bool gc_string_mark(P4(const byte *, uint, bool, gc_state_t *));
void gc_strings_clear_reloc(P1(chunk_t *));
void gc_strings_set_reloc(P1(chunk_t *));
void gc_strings_compact(P1(chunk_t *));
string_proc_reloc(igc_reloc_string);
const_string_proc_reloc(igc_reloc_const_string);

#endif /* igcstr_INCLUDED */
