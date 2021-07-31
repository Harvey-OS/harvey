/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zfdctd.c,v 1.2 2000/03/10 04:32:51 lpd Exp $ */
/* DCTDecode filter creation */
#include "memory_.h"
#include "stdio_.h"		/* for jpeglib.h */
#include "jpeglib_.h"
#include "ghost.h"
#include "oper.h"
#include "gsmalloc.h"		/* for gs_memory_default */
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"
#include "ialloc.h"
#include "ifilter.h"
#include "iparam.h"

/* Import the parameter processing procedure from sddparam.c */
stream_state_proc_put_params(s_DCTD_put_params, stream_DCT_state);

/* <source> <dict> DCTDecode/filter <file> */
/* <source> DCTDecode/filter <file> */
private int
zDCTD(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_memory_t *mem = &gs_memory_default;
    stream_DCT_state state;
    dict_param_list list;
    jpeg_decompress_data *jddp;
    int code;
    const ref *dop;
    uint dspace;

    /* First allocate space for IJG parameters. */
    jddp = (jpeg_decompress_data *)
	gs_alloc_bytes_immovable(mem, sizeof(*jddp), "zDCTD");
    if (jddp == 0)
	return_error(e_VMerror);
    if (s_DCTD_template.set_defaults)
	(*s_DCTD_template.set_defaults) ((stream_state *) & state);
    state.data.decompress = jddp;
    jddp->memory = state.jpeg_memory = mem;	/* set now for allocation */
    jddp->scanline_buffer = NULL;	/* set this early for safe error exit */
    state.report_error = filter_report_error;	/* in case create fails */
    if ((code = gs_jpeg_create_decompress(&state)) < 0)
	goto fail;		/* correct to do jpeg_destroy here */
    /* Read parameters from dictionary */
    if (r_has_type(op, t_dictionary))
	dop = op, dspace = r_space(op);
    else
	dop = 0, dspace = 0;
    if ((code = dict_param_list_read(&list, dop, NULL, false, iimemory)) < 0)
	goto fail;
    if ((code = s_DCTD_put_params((gs_param_list *) & list, &state)) < 0)
	goto rel;
    /* Create the filter. */
    jddp->template = s_DCTD_template;
    code = filter_read(i_ctx_p, 0, &jddp->template,
		       (stream_state *) & state, dspace);
    if (code >= 0)		/* Success! */
	return code;
    /*
     * We assume that if filter_read fails, the stream has not been
     * registered for closing, so s_DCTD_release will never be called.
     * Therefore we free the allocated memory before failing.
     */
rel:
    iparam_list_release(&list);
fail:
    gs_jpeg_destroy(&state);
    gs_free_object(mem, jddp, "zDCTD fail");
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zfdctd_op_defs[] =
{
    op_def_begin_filter(),
    {"2DCTDecode", zDCTD},
    op_def_end(0)
};
