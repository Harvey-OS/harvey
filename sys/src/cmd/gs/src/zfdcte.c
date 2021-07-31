/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zfdcte.c,v 1.3 2000/09/19 19:00:53 lpd Exp $ */
/* DCTEncode filter creation */
#include "memory_.h"
#include "stdio_.h"		/* for jpeglib.h */
#include "jpeglib_.h"
#include "ghost.h"
#include "oper.h"
#include "gsmalloc.h"		/* for gs_memory_default */
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"
#include "ifilter.h"
#include "iparam.h"

/*#define TEST*/

/* Import the parameter processing procedure from sdeparam.c */
stream_state_proc_put_params(s_DCTE_put_params, stream_DCT_state);
#ifdef TEST
stream_state_proc_get_params(s_DCTE_get_params, stream_DCT_state);
#endif

/* <target> <dict> DCTEncode/filter <file> */
private int
zDCTE(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_memory_t *mem = &gs_memory_default;
    stream_DCT_state state;
    dict_param_list list;
    jpeg_compress_data *jcdp;
    int code;
    const ref *dop;
    uint dspace;

    /* First allocate space for IJG parameters. */
    jcdp = (jpeg_compress_data *)
	gs_alloc_bytes_immovable(mem, sizeof(*jcdp), "zDCTE");
    if (jcdp == 0)
	return_error(e_VMerror);
    if (s_DCTE_template.set_defaults)
	(*s_DCTE_template.set_defaults) ((stream_state *) & state);
    state.data.compress = jcdp;
    jcdp->memory = state.jpeg_memory = mem;	/* set now for allocation */
    state.report_error = filter_report_error;	/* in case create fails */
    if ((code = gs_jpeg_create_compress(&state)) < 0)
	goto fail;		/* correct to do jpeg_destroy here */
    /* Read parameters from dictionary */
    if (r_has_type(op, t_dictionary))
	dop = op, dspace = r_space(op);
    else
	dop = 0, dspace = 0;
    if ((code = dict_param_list_read(&list, dop, NULL, false, iimemory)) < 0)
	goto fail;
    if ((code = s_DCTE_put_params((gs_param_list *) & list, &state)) < 0)
	goto rel;
    /* Create the filter. */
    jcdp->template = s_DCTE_template;
    /* Make sure we get at least a full scan line of input. */
    state.scan_line_size = jcdp->cinfo.input_components *
	jcdp->cinfo.image_width;
    jcdp->template.min_in_size =
	max(s_DCTE_template.min_in_size, state.scan_line_size);
    /* Make sure we can write the user markers in a single go. */
    jcdp->template.min_out_size =
	max(s_DCTE_template.min_out_size, state.Markers.size);
    code = filter_write(i_ctx_p, 0, &jcdp->template,
			(stream_state *) & state, dspace);
    if (code >= 0)		/* Success! */
	return code;
    /* We assume that if filter_write fails, the stream has not been
     * registered for closing, so s_DCTE_release will never be called.
     * Therefore we free the allocated memory before failing.
     */
rel:
    iparam_list_release(&list);
fail:
    gs_jpeg_destroy(&state);
    gs_free_object(mem, jcdp, "zDCTE fail");
    return code;
}

#ifdef TEST
#include "stream.h"
#include "files.h"
/* <dict> <filter> <bool> .dcteparams <dict> */
private int
zdcteparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    dict_param_list list;
    int code;

    check_type(*op, t_boolean);
    check_write_file(s, op - 1);
    check_type(op[-2], t_dictionary);
    /* The DCT filters copy the template.... */
    if (s->state->template->process != s_DCTE_template.process)
	return_error(e_rangecheck);
    code = dict_param_list_write(&list, op - 2, NULL, iimemory);
    if (code < 0)
	return code;
    code = s_DCTE_get_params((gs_param_list *) & list,
			     (stream_DCT_state *) s->state,
			     op->value.boolval);
    iparam_list_release(&list);
    if (code >= 0)
	pop(2);
    return code;
}
#endif

/* ------ Initialization procedure ------ */

const op_def zfdcte_op_defs[] =
{
#ifdef TEST
    {"3.dcteparams", zdcteparams},
#endif
    op_def_begin_filter(),
    {"2DCTEncode", zDCTE},
    op_def_end(0)
};
