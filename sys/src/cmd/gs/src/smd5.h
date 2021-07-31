/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: smd5.h,v 1.2 2000/09/19 19:00:50 lpd Exp $ */
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

#endif /* smd5_INCLUDED */
