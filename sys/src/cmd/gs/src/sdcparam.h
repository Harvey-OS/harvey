/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sdcparam.h,v 1.5 2002/06/16 05:00:54 lpd Exp $ */
/* DCT filter parameter setting and reading interface */

#ifndef sdcparam_INCLUDED
#  define sdcparam_INCLUDED

/*
 * All of these procedures are defined in sdcparam.c and are only for
 * internal use (by sddparam.c and sdeparam.c), so they are not
 * documented here.
 */

int s_DCT_get_params(gs_param_list * plist, const stream_DCT_state * ss,
		     const stream_DCT_state * defaults);
int s_DCT_get_quantization_tables(gs_param_list * plist,
				  const stream_DCT_state * pdct,
				  const stream_DCT_state * defaults,
				  bool is_encode);
int s_DCT_get_huffman_tables(gs_param_list * plist,
			     const stream_DCT_state * pdct,
			     const stream_DCT_state * defaults,
			     bool is_encode);

int s_DCT_byte_params(gs_param_list * plist, gs_param_name key, int start,
		      int count, UINT8 * pvals);
int s_DCT_put_params(gs_param_list * plist, stream_DCT_state * pdct);
int s_DCT_put_quantization_tables(gs_param_list * plist,
				  stream_DCT_state * pdct,
				  bool is_encode);
int s_DCT_put_huffman_tables(gs_param_list * plist, stream_DCT_state * pdct,
			     bool is_encode);

#endif /* sdcparam_INCLUDED */
