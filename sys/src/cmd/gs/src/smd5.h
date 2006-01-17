/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: smd5.h,v 1.5 2004/01/13 14:03:30 igor Exp $ */
/* Definitions for MD5Encode filter */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef smd5_INCLUDED
#  define smd5_INCLUDED

#include "md5.h"

/*
 * The MD5Encode filter accepts an arbitrary amount of input data, and then,
 * when closed, emits the 16-byte MD5 digest.
 */
typedef struct stream_MD5E_state_s {
    stream_state_common;
    md5_state_t md5;
} stream_MD5E_state;

#define private_st_MD5E_state()	/* in smd5.c */\
  gs_private_st_simple(st_MD5E_state, stream_MD5E_state,\
    "MD5Encode state")
extern const stream_template s_MD5E_template;

stream *s_MD5E_make_stream(gs_memory_t *mem, byte *digest, int digest_size);

#endif /* smd5_INCLUDED */
