/* Copyright (C) 1992, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ccfont.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Header for fonts compiled into C. */

#ifndef ccfont_INCLUDED
#  define ccfont_INCLUDED

/* Include all the things a compiled font needs. */
#include "stdpre.h"
#include "gsmemory.h"
#include "iref.h"
#include "ivmspace.h"		/* for avm_foreign */
#include "store.h"

/* Define type-specific refs for initializing arrays. */
#define ref_(t) struct { struct tas_s tas; t value; }
#define boolean_v(b) { {t_boolean<<r_type_shift}, (ushort)(b) }
#define integer_v(i) { {t_integer<<r_type_shift}, (long)(i) }
#define null_v() { {t_null<<r_type_shift} }
#define real_v(v) { {t_real<<r_type_shift}, (float)(v) }

/* Define other initialization structures. */
typedef struct {
    byte encx, charx;
} charindex;

/*
 * We represent mostly-string arrays by byte strings.  Each element
 * starts with length bytes.  If the first length byte is not 255,
 * it and the following byte define a big-endian length of a string or name.
 * If the first two bytes are (255,255), this element is null.
 * Otherwise, the initial 255 is followed by a 2-byte big-endian length
 * of a string that must be scanned as a token.
 */
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

/*
 * We pass a procedure vector to the font initialization routine
 * to avoid having externs, which compromise sharability.
 */
typedef struct cfont_procs_s {
    int (*ref_dict_create) (P5(i_ctx_t *, ref *, const cfont_dict_keys *,
			       cfont_string_array, const ref *));
    int (*string_dict_create) (P5(i_ctx_t *, ref *, const cfont_dict_keys *,
				  cfont_string_array,
				  cfont_string_array));
    int (*num_dict_create) (P6(i_ctx_t *, ref *, const cfont_dict_keys *,
			       cfont_string_array, const ref *,
			       const char *));
    int (*name_array_create) (P4(i_ctx_t *, ref *, cfont_string_array, int));
    int (*string_array_create) (P5(i_ctx_t *, ref *, cfont_string_array,
				   int /*size */ , uint /*protection */ ));
    int (*scalar_array_create) (P5(i_ctx_t *, ref *, const ref *,
				   int /*size */ , uint /*protection */ ));
    int (*name_create) (P3(i_ctx_t *, ref *, const char *));
    int (*ref_from_string) (P4(i_ctx_t *, ref *, const char *, uint));
} cfont_procs;

/*
 * In order to make it possible for third parties to compile fonts (into
 * a shared library, on systems that support such things), we define
 * a tiny procedural interface for getting access to the compiled font table.
 */
#define ccfont_proc(proc)\
  int proc(P3(i_ctx_t *, const cfont_procs *, ref *))
typedef ccfont_proc((*ccfont_fproc));

/*
 * There should be some consts in the *** below, but a number of
 * C compilers don't handle const properly in such situations.
 */
extern int ccfont_fprocs(P2(int *, const ccfont_fproc **));

#define ccfont_version 19	/* for checking against libraries */

#endif /* ccfont_INCLUDED */
