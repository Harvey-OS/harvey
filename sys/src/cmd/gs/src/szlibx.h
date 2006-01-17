/* Copyright (C) 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: szlibx.h,v 1.4 2002/02/21 22:24:54 giles Exp $ */
/* zlib filter state definition */

#ifndef szlibx_INCLUDED
#  define szlibx_INCLUDED

/* Define an opaque type for the dynamic part of the state. */
typedef struct zlib_dynamic_state_s zlib_dynamic_state_t;

/* Define the stream state structure. */
typedef struct stream_zlib_state_s {
    stream_state_common;
    /* Parameters - compression and decompression */
    int windowBits;
    bool no_wrapper;		/* omit wrapper and checksum */
    /* Parameters - compression only */
    int level;			/* effort level */
    int method;
    int memLevel;
    int strategy;
    /* Dynamic state */
    zlib_dynamic_state_t *dynamic;
} stream_zlib_state;

/*
 * The state descriptor is public only to allow us to split up
 * the encoding and decoding filters.
 */
extern_st(st_zlib_state);
#define public_st_zlib_state()	/* in szlibc.c */\
  gs_public_st_ptrs1(st_zlib_state, stream_zlib_state,\
    "zlibEncode/Decode state", zlib_state_enum_ptrs, zlib_state_reloc_ptrs,\
    dynamic)
extern const stream_template s_zlibD_template;
extern const stream_template s_zlibE_template;

/* Shared procedures */
stream_proc_set_defaults(s_zlib_set_defaults);

#endif /* szlibx_INCLUDED */
