/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sdcparam.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* DCT filter parameter setting and reading interface */

#ifndef sdcparam_INCLUDED
#  define sdcparam_INCLUDED

/*
 * All of these procedures are defined in sdcparam.c and are only for
 * internal use (by sddparam.c and sdeparam.c), so they are not
 * documented here.
 */

int s_DCT_get_params(P3(gs_param_list * plist, const stream_DCT_state * ss,
			const stream_DCT_state * defaults));
int s_DCT_get_quantization_tables(P4(gs_param_list * plist,
				     const stream_DCT_state * pdct,
				     const stream_DCT_state * defaults,
				     bool is_encode));
int s_DCT_get_huffman_tables(P4(gs_param_list * plist,
				const stream_DCT_state * pdct,
				const stream_DCT_state * defaults,
				bool is_encode));

int s_DCT_byte_params(P5(gs_param_list * plist, gs_param_name key, int start,
			 int count, UINT8 * pvals));
int s_DCT_put_params(P2(gs_param_list * plist, stream_DCT_state * pdct));
int s_DCT_put_quantization_tables(P3(gs_param_list * plist,
				     stream_DCT_state * pdct,
				     bool is_encode));
int s_DCT_put_huffman_tables(P3(gs_param_list * plist, stream_DCT_state * pdct,
				bool is_encode));

#endif /* sdcparam_INCLUDED */
