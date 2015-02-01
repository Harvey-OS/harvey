/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sa85x.h,v 1.6 2002/11/13 08:23:13 raph Exp $ */
/* ASCII85 filter interface */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sa85x_INCLUDED
#  define sa85x_INCLUDED

#include "sa85d.h"

/* ASCII85Encode */
typedef struct stream_A85E_state_s {
    stream_state_common;
    /* The following change dynamically. */
    int count;			/* # of digits since last EOL */
    int last_char;		/* last character written */
} stream_A85E_state;

#define private_st_A85E_state()	/* in sfilter2.c */\
  gs_private_st_simple(st_A85E_state, stream_A85E_state,\
    "ASCII85Encode state")
#define s_A85E_init_inline(ss)\
  ((ss)->count = 0, (ss)->last_char = '\n', 0)
extern const stream_template s_A85E_template;

#endif /* sa85x_INCLUDED */
