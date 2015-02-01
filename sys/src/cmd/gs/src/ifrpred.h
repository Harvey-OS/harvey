/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

/* $Id: ifrpred.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* filter_read_predictor prototype */

#ifndef ifrpred_INCLUDED
#  define ifrpred_INCLUDED

/* Exported by zfdecode.c for zfzlib.c */
int filter_read_predictor(i_ctx_t *i_ctx_p, int npop,
			  const stream_template * template,
			  stream_state * st);

#endif /* ifrpred_INCLUDED */
