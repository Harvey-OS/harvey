/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zfrsd.c,v 1.10 2004/08/04 19:36:13 stefan Exp $ */
/* ReusableStreamDecode filter support */
#include "memory_.h"
#include "ghost.h"
#include "gsfname.h"		/* for gs_parse_file_name */
#include "gxiodev.h"
#include "oper.h"
#include "stream.h"
#include "strimpl.h"
#include "sfilter.h"		/* for SubFileDecode */
#include "files.h"
#include "idict.h"
#include "idparam.h"
#include "iname.h"
#include "store.h"

/* ---------------- Reusable streams ---------------- */

/*
 * The actual work of constructing the filter is done in PostScript code.
 * The operators in this file are internal ones that handle the dirty work.
 */

/* <dict|null> .rsdparams <filters> <decodeparms|null> */
/* filters is always an array; decodeparms is always either an array */
/* of the same length as filters, or null. */
private int
zrsdparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref *pFilter;
    ref *pDecodeParms;
    int Intent;
    bool AsyncRead;
    ref empty_array, filter1_array, parms1_array;
    uint i;
    int code;

    make_empty_array(&empty_array, a_readonly);
    if (dict_find_string(op, "Filter", &pFilter) > 0) {
	if (!r_is_array(pFilter)) {
	    if (!r_has_type(pFilter, t_name))
		return_error(e_typecheck);
	    make_array(&filter1_array, a_readonly, 1, pFilter);
	    pFilter = &filter1_array;
	}
    } else
	pFilter = &empty_array;
    /* If Filter is undefined, ignore DecodeParms. */
    if (pFilter != &empty_array &&
	dict_find_string(op, "DecodeParms", &pDecodeParms) > 0
	) {
	if (pFilter == &filter1_array) {
	    make_array(&parms1_array, a_readonly, 1, pDecodeParms);
	    pDecodeParms = &parms1_array;
	} else if (!r_is_array(pDecodeParms))
	    return_error(e_typecheck);
	else if (r_size(pFilter) != r_size(pDecodeParms))
	    return_error(e_rangecheck);
    } else
	pDecodeParms = 0;
    for (i = 0; i < r_size(pFilter); ++i) {
	ref f, fname, dp;

	array_get(imemory, pFilter, (long)i, &f);
	if (!r_has_type(&f, t_name))
	    return_error(e_typecheck);
	name_string_ref(imemory, &f, &fname);
	if (r_size(&fname) < 6 ||
	    memcmp(fname.value.bytes + r_size(&fname) - 6, "Decode", 6)
	    )
	    return_error(e_rangecheck);
	if (pDecodeParms) {
	    array_get(imemory, pDecodeParms, (long)i, &dp);
	    if (!(r_has_type(&dp, t_dictionary) || r_has_type(&dp, t_null)))
		return_error(e_typecheck);
	}
    }
    if ((code = dict_int_param(op, "Intent", 0, 3, 0, &Intent)) < 0 ||
	(code = dict_bool_param(op, "AsyncRead", false, &AsyncRead)) < 0
	)
	return code;
    push(1);
    op[-1] = *pFilter;
    if (pDecodeParms)
	*op = *pDecodeParms;
    else
	make_null(op);
    return 0;
}

/* <file|string> <CloseSource> .reusablestream <filter> */
/*
 * The file|string operand must be a "reusable source", either:
 *      - A string or bytestring;
 *      - A readable, positionable file stream;
 *      - A readable string stream;
 *      - A SubFileDecode filter with an empty EODString and a reusable
 *      source.
 * Reusable streams are also reusable sources, but they look just like
 * ordinary file or string streams.
 */
private int make_rss(i_ctx_t *i_ctx_p, os_ptr op, const byte * data,
		     uint size, int space, long offset, long length,
		     bool is_bytestring);
private int make_rfs(i_ctx_t *i_ctx_p, os_ptr op, stream *fs,
		     long offset, long length);
private int
zreusablestream(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr source_op = op - 1;
    long length = max_long;
    bool close_source;
    int code;

    check_type(*op, t_boolean);
    close_source = op->value.boolval;
    if (r_has_type(source_op, t_string)) {
	uint size = r_size(source_op);

	check_read(*source_op);
	code = make_rss(i_ctx_p, source_op, source_op->value.const_bytes,
			size, r_space(source_op), 0L, size, false);
    } else if (r_has_type(source_op, t_astruct)) {
	uint size = gs_object_size(imemory, source_op->value.pstruct);

	if (gs_object_type(imemory, source_op->value.pstruct) != &st_bytes)
	    return_error(e_rangecheck);
	check_read(*source_op);
	code = make_rss(i_ctx_p, source_op,
			(const byte *)source_op->value.pstruct, size,
			r_space(source_op), 0L, size, true);
    } else {
	long offset = 0;
	stream *source;
	stream *s;

	check_read_file(source, source_op);
	s = source;
rs:
	if (s->cbuf_string.data != 0) {	/* string stream */
	    long pos = stell(s);
	    long avail = sbufavailable(s) + pos;

	    offset += pos;
	    code = make_rss(i_ctx_p, source_op, s->cbuf_string.data,
			    s->cbuf_string.size,
			    imemory_space((const gs_ref_memory_t *)s->memory),
			    offset, min(avail, length), false);
	} else if (s->file != 0) { /* file stream */
	    if (~s->modes & (s_mode_read | s_mode_seek))
		return_error(e_ioerror);
	    code = make_rfs(i_ctx_p, source_op, s, offset + stell(s), length);
	} else if (s->state->template == &s_SFD_template) {
	    /* SubFileDecode filter */
	    const stream_SFD_state *const sfd_state =
		(const stream_SFD_state *)s->state;

	    if (sfd_state->eod.size != 0)
		return_error(e_rangecheck);
	    offset += sfd_state->skip_count - sbufavailable(s);
	    if (sfd_state->count != 0) {
		long left = max(sfd_state->count, 0) + sbufavailable(s);

		if (left < length)
		    length = left;
	    }
	    s = s->strm;
	    goto rs;
	}
	else			/* some other kind of stream */
	    return_error(e_rangecheck);
	if (close_source) {
	    stream *rs = fptr(source_op);

	    rs->strm = source;	/* only for close_source */
	    rs->close_strm = true;
	}
    }
    if (code >= 0)
	pop(1);
    return code;
}

/* Make a reusable string stream. */
private int
make_rss(i_ctx_t *i_ctx_p, os_ptr op, const byte * data, uint size,
	 int string_space, long offset, long length, bool is_bytestring)
{
    stream *s;
    long left = min(length, size - offset);

    if (icurrent_space < string_space)
	return_error(e_invalidaccess);
    s = file_alloc_stream(imemory, "make_rss");
    if (s == 0)
	return_error(e_VMerror);
    sread_string_reusable(s, data + offset, max(left, 0));
    if (is_bytestring)
	s->cbuf_string.data = 0;	/* byte array, not string */
    make_stream_file(op, s, "r");
    return 0;
}

/* Make a reusable file stream. */
private int
make_rfs(i_ctx_t *i_ctx_p, os_ptr op, stream *fs, long offset, long length)
{
    gs_const_string fname;
    gs_parsed_file_name_t pname;
    stream *s;
    int code;

    if (sfilename(fs, &fname) < 0)
	return_error(e_ioerror);
    code = gs_parse_file_name(&pname, (const char *)fname.data, fname.size);
    if (code < 0)
	return code;
    if (pname.len == 0)		/* %stdin% etc. won't have a filename */
	return_error(e_invalidfileaccess); /* can't reopen */
    if (pname.iodev == NULL)
	pname.iodev = iodev_default;
    /* Open the file again, to be independent of the source. */
    code = file_open_stream((const char *)pname.fname, pname.len, "r",
			    fs->cbsize, &s, pname.iodev,
			    pname.iodev->procs.fopen, imemory);
    if (code < 0)
	return code;
    if (sread_subfile(s, offset, length) < 0) {
	sclose(s);
	return_error(e_ioerror);
    }
    s->close_at_eod = false;
    make_stream_file(op, s, "r");
    return 0;
}

/* ---------------- Initialization procedure ---------------- */

const op_def zfrsd_op_defs[] =
{
    {"2.reusablestream", zreusablestream},
    {"2.rsdparams", zrsdparams},
    op_def_end(0)
};
