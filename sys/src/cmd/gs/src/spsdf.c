/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: spsdf.c,v 1.7 2002/02/21 22:24:54 giles Exp $ */
/* Common utilities for PostScript and PDF format printing */
#include "stdio_.h"		/* for stream.h */
#include "string_.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gserror.h"
#include "gserrors.h"
#include "spprint.h"
#include "spsdf.h"
#include "stream.h"
#include "strimpl.h"
#include "sa85x.h"
#include "sstring.h"
#include "scanchar.h"

/*
 * Write a string in its shortest form ( () or <> ).  Note that
 * this form is different depending on whether binary data are allowed.
 * Currently we don't support ASCII85 strings ( <~ ~> ).
 */
void
s_write_ps_string(stream * s, const byte * str, uint size, int print_ok)
{
    uint added = 0;
    uint i;
    const stream_template *template;
    stream_AXE_state state;
    stream_state *st = NULL;

    if (print_ok & PRINT_BINARY_OK) {
	/* Only need to escape (, ), \, CR, EOL. */
	stream_putc(s, '(');
	for (i = 0; i < size; ++i) {
	    byte ch = str[i];

	    switch (ch) {
		case char_CR:
		    stream_puts(s, "\\r");
		    continue;
		case char_EOL:
		    stream_puts(s, "\\n");
		    continue;
		case '(':
		case ')':
		case '\\':
		    stream_putc(s, '\\');
	    }
	    stream_putc(s, ch);
	}
	stream_putc(s, ')');
	return;
    }
    for (i = 0; i < size; ++i) {
	byte ch = str[i];

	if (ch == 0 || ch >= 127)
	    added += 3;
	else if (strchr("()\\\n\r\t\b\f", ch) != 0)
	    ++added;
	else if (ch < 32)
	    added += 3;
    }

    if (added < size || (print_ok & PRINT_HEX_NOT_OK)) {
	/* More efficient, or mandatory, to represent as PostScript string. */
	template = &s_PSSE_template;
	stream_putc(s, '(');
    } else {
	/* More efficient, and permitted, to represent as hex string. */
	template = &s_AXE_template;
	st = (stream_state *) & state;
	s_AXE_init_inline(&state);
	stream_putc(s, '<');
    }

    {
	byte buf[100];		/* size is arbitrary */
	stream_cursor_read r;
	stream_cursor_write w;
	int status;

	r.ptr = str - 1;
	r.limit = r.ptr + size;
	w.limit = buf + sizeof(buf) - 1;
	do {
	    /* One picky compiler complains if we initialize to buf - 1. */
	    w.ptr = buf;  w.ptr--;
	    status = (*template->process) (st, &r, &w, true);
	    stream_write(s, buf, (uint) (w.ptr + 1 - buf));
	}
	while (status == 1);
    }
}

/* Set up a write stream that just keeps track of the position. */
int
s_alloc_position_stream(stream ** ps, gs_memory_t * mem)
{
    stream *s = *ps = s_alloc(mem, "s_alloc_position_stream");

    if (s == 0)
	return_error(gs_error_VMerror);
    swrite_position_only(s);
    return 0;
}

/* ---------------- Parameter printing ---------------- */

private_st_printer_param_list();
const param_printer_params_t param_printer_params_default = {
    param_printer_params_default_values
};

/* We'll implement the other printers later if we have to. */
private param_proc_xmit_typed(param_print_typed);
/*private param_proc_begin_xmit_collection(param_print_begin_collection); */
/*private param_proc_end_xmit_collection(param_print_end_collection); */
private const gs_param_list_procs printer_param_list_procs = {
    param_print_typed,
    NULL /* begin_collection */ ,
    NULL /* end_collection */ ,
    NULL /* get_next_key */ ,
    gs_param_request_default,
    gs_param_requested_default
};

int
s_init_param_printer(printer_param_list_t *prlist,
		     const param_printer_params_t * ppp, stream * s)
{
    gs_param_list_init((gs_param_list *)prlist, &printer_param_list_procs,
		       NULL);
    prlist->strm = s;
    prlist->params = *ppp;
    prlist->any = false;
    return 0;
}
int
s_alloc_param_printer(gs_param_list ** pplist,
		      const param_printer_params_t * ppp, stream * s,
		      gs_memory_t * mem)
{
    printer_param_list_t *prlist =
	gs_alloc_struct(mem, printer_param_list_t, &st_printer_param_list,
			"s_alloc_param_printer");
    int code;

    *pplist = (gs_param_list *)prlist;
    if (prlist == 0)
	return_error(gs_error_VMerror);
    code = s_init_param_printer(prlist, ppp, s);
    prlist->memory = mem;
    return code;
}

void
s_release_param_printer(printer_param_list_t *prlist)
{
    if (prlist) {
	if (prlist->any && prlist->params.suffix)
	    stream_puts(prlist->strm, prlist->params.suffix);
    }
}
void
s_free_param_printer(gs_param_list * plist)
{
    if (plist) {
	printer_param_list_t *const prlist = (printer_param_list_t *) plist;

	s_release_param_printer(prlist);
	gs_free_object(prlist->memory, plist, "s_free_param_printer");
    }
}

private int
param_print_typed(gs_param_list * plist, gs_param_name pkey,
		  gs_param_typed_value * pvalue)
{
    printer_param_list_t *const prlist = (printer_param_list_t *)plist;
    stream *s = prlist->strm;

    if (!prlist->any) {
	if (prlist->params.prefix)
	    stream_puts(s, prlist->params.prefix);
	prlist->any = true;
    }
    if (prlist->params.item_prefix)
	stream_puts(s, prlist->params.item_prefix);
    pprints1(s, "/%s", pkey);
    switch (pvalue->type) {
	case gs_param_type_null:
	    stream_puts(s, " null");
	    break;
	case gs_param_type_bool:
	    stream_puts(s, (pvalue->value.b ? " true" : " false"));
	    break;
	case gs_param_type_int:
	    pprintd1(s, " %d", pvalue->value.i);
	    break;
	case gs_param_type_long:
	    pprintld1(s, " %l", pvalue->value.l);
	    break;
	case gs_param_type_float:
	    pprintg1(s, " %g", pvalue->value.f);
	    break;
	case gs_param_type_string:
	    s_write_ps_string(s, pvalue->value.s.data, pvalue->value.s.size,
			      prlist->params.print_ok);
	    break;
	case gs_param_type_name:
	    /****** SHOULD USE #-ESCAPES FOR PDF ******/
	    stream_putc(s, '/');
	    stream_write(s, pvalue->value.n.data, pvalue->value.n.size);
	    break;
	case gs_param_type_int_array:
	    {
		uint i;
		char sepr = (pvalue->value.ia.size <= 10 ? ' ' : '\n');

		stream_putc(s, '[');
		for (i = 0; i < pvalue->value.ia.size; ++i) {
		    pprintd1(s, "%d", pvalue->value.ia.data[i]);
		    stream_putc(s, sepr);
		}
		stream_putc(s, ']');
	    }
	    break;
	case gs_param_type_float_array:
	    {
		uint i;
		char sepr = (pvalue->value.fa.size <= 10 ? ' ' : '\n');

		stream_putc(s, '[');
		for (i = 0; i < pvalue->value.fa.size; ++i) {
		    pprintg1(s, "%g", pvalue->value.fa.data[i]);
		    stream_putc(s, sepr);
		}
		stream_putc(s, ']');
	    }
	    break;
	    /*case gs_param_type_string_array: */
	    /*case gs_param_type_name_array: */
	default:
	    return_error(gs_error_typecheck);
    }
    if (prlist->params.item_suffix)
	stream_puts(s, prlist->params.item_suffix);
    return 0;
}
