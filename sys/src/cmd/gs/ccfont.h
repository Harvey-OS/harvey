/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* ccfont.h */
/* Header for fonts compiled into C. */

#ifndef ccfont_INCLUDED
#  define ccfont_INCLUDED

/* Include all the things a compiled font needs. */
#include "std.h"
#include "gsmemory.h"
#include "iref.h"
#include "store.h"

/* Define type-specific refs for initializing arrays. */
#define ref_(t) struct { struct tas_s tas; t value; }
#define boolean_v(b) { {t_boolean<<r_type_shift}, (ushort)(b) }
#define integer_v(i) { {t_integer<<r_type_shift}, (long)(i) }
#define null_v() { {t_null<<r_type_shift} }
#define real_v(v) { {t_real<<r_type_shift}, (float)(v) }

/* Define other initialization structures. */
typedef struct { byte encx, charx; } charindex;
/* Arrays of strings/names/nulls are represented by byte strings, */
/* containing two bytes of big-endian string length (255,255=null) */
/* followed by the string characters. */
typedef const char *cfont_string_array;

/* Support routines in iccfont.c */
typedef struct {
	const charindex *enc_keys;	/* keys from encoding vectors */
	uint num_enc_keys;
	uint num_str_keys;
	uint extra_slots;		/* (need extra for fonts) */
	uint dict_attrs;		/* protection for dictionary */
	uint value_attrs;		/* protection for values */
					/* (only used for string dicts) */
} cfont_dict_keys;
/* We pass a procedure vector to the font initialization routine */
/* to avoid having externs, which compromise sharability. */
/* On MS-DOS, each compiled font has its own data segment, */
/* so all of these procedures must be declared 'huge' for Borland C. */
typedef struct cfont_procs_s {
	int huge (*ref_dict_create)(P4(ref *, const cfont_dict_keys *,
				       cfont_string_array, const ref *));
	int huge (*string_dict_create)(P4(ref *, const cfont_dict_keys *,
					  cfont_string_array,
					  cfont_string_array));
	int huge (*num_dict_create)(P5(ref *, const cfont_dict_keys *,
				       cfont_string_array, const ref *,
				       const char *));
	int huge (*name_array_create)(P3(ref *, cfont_string_array, int));
	int huge (*string_array_create)(P4(ref *, cfont_string_array,
					   int /*size*/,
					   uint	/*protection*/));
	int huge (*name_create)(P2(ref *, const char *));
	int huge (*ref_from_string)(P3(ref *, const char *, uint));
} cfont_procs;


#endif					/* ccfont_INCLUDED */
