/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zfdctd.c */
/* DCTDecode filter creation */
#include "memory_.h"
#include "stdio_.h"			/* for jpeglib.h */
#include "jpeglib.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"
#include "ifilter.h"

/* Import the common setup routines from zfdctc.c */
int zfdct_setup(P2(const ref *op, stream_DCT_state *pdct));
int zfdct_setup_quantization_tables(P3(const ref *op, stream_DCT_state *pdct,
				       bool is_encode));
int zfdct_setup_huffman_tables(P3(const ref *op, stream_DCT_state *pdct,
				  bool is_encode));

/* <source> <dict> DCTDecode/filter <file> */
/* <source> DCTDecode/filter <file> */
int
zDCTD(os_ptr op)
{	stream_DCT_state state;
	jpeg_decompress_data *jddp;
	int code;
	int npop;
	const ref *dop;
	uint dspace;

	/* First allocate space for IJG parameters. */
	jddp = gs_malloc(1, sizeof(*jddp), "zDCTD");
	if ( jddp == 0 )
		return_error(e_VMerror);
	state.data.decompress = jddp;
	jddp->scanline_buffer = NULL; /* set this early for safe error exit */
	if ( (code = gs_jpeg_create_decompress(&state)) < 0 )
		goto fail;	/* correct to do jpeg_destroy here */
	/* Read parameters from dictionary */
	if ( (code = zfdct_setup(op, &state)) < 0 )
		goto fail;
	npop = code;
	if ( npop == 0 )
	  dop = 0, dspace = 0;
	else
	  dop = op, dspace = r_space(op);
	/* DCTDecode accepts quantization and huffman tables
	 * in case these tables have been omitted from the datastream.
	 */
	if ( (code = zfdct_setup_huffman_tables(dop, &state, false)) < 0 ||
	     (code = zfdct_setup_quantization_tables(dop, &state, false)) < 0
	   )
		goto fail;
	/* Create the filter. */
	jddp->template = s_DCTD_template;
	code = filter_read(op, npop, &jddp->template,
			   (stream_state *)&state, dspace);
	if ( code >= 0 )		/* Success! */
		return code;
	/* We assume that if filter_read fails, the stream has not been
	 * registered for closing, so s_DCTD_release will never be called.
	 * Therefore we free the allocated memory before failing.
	 */

fail:
	gs_jpeg_destroy(&state);
	gs_free(jddp, 1, sizeof(*jddp), "zDCTD fail");
	return code;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfdctd_op_defs) {
		op_def_begin_filter(),
	{"2DCTDecode", zDCTD},
END_OP_DEFS(0) }
