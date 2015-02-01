/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

/* $Id: sbtx.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Definitions for ByteTranslate filters */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sbtx_INCLUDED
#  define sbtx_INCLUDED

/* ByteTranslateEncode/Decode */
typedef struct stream_BT_state_s {
    stream_state_common;
    byte table[256];
} stream_BT_state;
typedef stream_BT_state stream_BTE_state;
typedef stream_BT_state stream_BTD_state;

#define private_st_BT_state()	/* in sfilter1.c */\
  gs_private_st_simple(st_BT_state, stream_BT_state,\
    "ByteTranslateEncode/Decode state")
extern const stream_template s_BTE_template;
extern const stream_template s_BTD_template;

#endif /* sbtx_INCLUDED */
