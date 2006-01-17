/* Copyright (C) 1993, 1995, 1997, 1998, 1999, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ziodev.c,v 1.13 2003/09/03 03:22:59 giles Exp $ */
/* Standard IODevice implementation */
#include "memory_.h"
#include "stdio_.h"
#include "string_.h"
#include "ghost.h"
#include "gp.h"
#include "gpcheck.h"
#include "oper.h"
#include "stream.h"
#include "istream.h"
#include "ialloc.h"
#include "iscan.h"
#include "ivmspace.h"
#include "gxiodev.h"		/* must come after stream.h */
				/* and before files.h */
#include "files.h"
#include "scanchar.h"		/* for char_EOL */
#include "store.h"
#include "ierrors.h"

/* Import the dtype of the stdio IODevices. */
extern const char iodev_dtype_stdio[];

/* Define the special devices. */
#define iodev_special(dname, init, open) {\
    dname, iodev_dtype_stdio,\
	{ init, open, iodev_no_open_file, iodev_no_fopen, iodev_no_fclose,\
	  iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,\
	  iodev_no_enumerate_files, NULL, NULL,\
	  iodev_no_get_params, iodev_no_put_params\
	}\
}

/*
 * We need the current context pointer for accessing / opening the %std
 * IODevices.  However, this is not available to the open routine.
 * Therefore, we use the hack of storing this pointer in the IODevice state
 * pointer just before calling the open routines.  We clear the pointer
 * immediately afterwards so as not to wind up with dangling references.
 */

#define LINEEDIT_BUF_SIZE 20	/* initial size, not fixed size */
/*private iodev_proc_open_device(lineedit_open);*/ /* no longer used */
const gx_io_device gs_iodev_lineedit =
    iodev_special("%lineedit%", iodev_no_init, iodev_no_open_device);

#define STATEMENTEDIT_BUF_SIZE 50	/* initial size, not fixed size */
/*private iodev_proc_open_device(statementedit_open);*/ /* no longer used */
const gx_io_device gs_iodev_statementedit =
    iodev_special("%statementedit%", iodev_no_init, iodev_no_open_device);

/* ------ Operators ------ */

/* <int> .getiodevice <string|null> */
private int
zgetiodevice(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gx_io_device *iodev;
    const byte *dname;

    check_type(*op, t_integer);
    if (op->value.intval != (int)op->value.intval)
	return_error(e_rangecheck);
    iodev = gs_getiodevice((int)(op->value.intval));
    if (iodev == 0)		/* index out of range */
	return_error(e_rangecheck);
    dname = (const byte *)iodev->dname;
    if (dname == 0)
	make_null(op);
    else
	make_const_string(op, a_readonly | avm_foreign,
			  strlen((const char *)dname), dname);
    return 0;
}

/* ------ %lineedit and %statementedit ------ */

/* <file> <bool> <int> <string> .filelineedit <file> */
/* This opens %statementedit% or %lineedit% and is also the 
 * continuation proc for callouts.
 * Input:
 *  string is the statement/line buffer, 
 *  int is the write index into string
 *  bool is true if %statementedit%
 *  file is stdin
 * Output:
 *  file is a string based stream
 * We store the line being read in a PostScript string.
 * This limits the size to max_string_size (64k).
 * This could be increased by storing the input line in something 
 * other than a PostScript string.
 */
int
zfilelineedit(i_ctx_t *i_ctx_p)
{
    uint count = 0;
    bool in_eol = false;
    int code;
    os_ptr op = osp;
    bool statement;
    stream *s;
    stream *ins;
    gs_string str;
    uint initial_buf_size;
    const char *filename;
    /*
     * buf exists only for stylistic parallelism: all occurrences of
     * buf-> could just as well be str. .
     */
    gs_string *const buf = &str;

    check_type(*op, t_string);		/* line assembled so far */
    buf->data = op->value.bytes;
    buf->size = op->tas.rsize;
    check_type(*(op-1), t_integer);	/* index */
    count = (op-1)->value.intval;
    check_type(*(op-2), t_boolean);	/* statementedit/lineedit */
    statement = (op-2)->value.boolval;
    check_read_file(ins, op - 3);	/* %stdin */

    /* extend string */
    initial_buf_size = statement ? STATEMENTEDIT_BUF_SIZE : LINEEDIT_BUF_SIZE;
    if (initial_buf_size > max_string_size)
	return_error(e_limitcheck);
    if (!buf->data || (buf->size < initial_buf_size)) {
	count = 0;
	buf->data = gs_alloc_string(imemory, initial_buf_size, 
	    "zfilelineedit(buffer)");
	if (buf->data == 0)
	    return_error(e_VMerror);
        op->value.bytes = buf->data;
	op->tas.rsize = buf->size = initial_buf_size;
    }

rd:
    code = zreadline_from(ins, buf, imemory, &count, &in_eol);
    if (buf->size > max_string_size) {
	/* zreadline_from reallocated the buffer larger than
	 * is valid for a PostScript string.
	 * Return an error, but first realloc the buffer
	 * back to a legal size.
	 */
	byte *nbuf = gs_resize_string(imemory, buf->data, buf->size, 
		max_string_size, "zfilelineedit(shrink buffer)");
	if (nbuf == 0)
	    return_error(e_VMerror);
	op->value.bytes = buf->data = nbuf;
	op->tas.rsize = buf->size = max_string_size;
	return_error(e_limitcheck);
    }

    op->value.bytes = buf->data; /* zreadline_from sometimes resizes the buffer. */
    op->tas.rsize = buf->size;

    switch (code) {
	case EOFC:
	    code = gs_note_error(e_undefinedfilename);
	    /* falls through */
	case 0:
	    break;
	default:
	    code = gs_note_error(e_ioerror);
	    break;
	case CALLC:
	    {
		ref rfile;
		(op-1)->value.intval = count;
		/* callout is for stdin */
		make_file(&rfile, a_readonly | avm_system, ins->read_id, ins);
		code = s_handle_read_exception(i_ctx_p, code, &rfile,  
		    NULL, 0, zfilelineedit);
	    }
	    break;
	case 1:		/* filled buffer */
	    {
		uint nsize = buf->size;
		byte *nbuf;

		if (nsize >= max_string_size) {
		    code = gs_note_error(e_limitcheck);
		    break;
		}
		else if (nsize >= max_string_size / 2)
		    nsize= max_string_size;
		else
		    nsize = buf->size * 2;
		nbuf = gs_resize_string(imemory, buf->data, buf->size, nsize,
					"zfilelineedit(grow buffer)");
		if (nbuf == 0) {
		    code = gs_note_error(e_VMerror);
		    break;
		}
		op->value.bytes = buf->data = nbuf;
		op->tas.rsize = buf->size = nsize;
		goto rd;
	    }
    }
    if (code != 0)
	return code;
    if (statement) {
	/* If we don't have a complete token, keep going. */
	stream st;
	stream *ts = &st;
	scanner_state state;
	ref ignore_value;
	uint depth = ref_stack_count(&o_stack);
	int code;

	/* Add a terminating EOL. */
	if (count + 1 > buf->size) {
	    uint nsize;
	    byte *nbuf;

	    nsize = buf->size + 1;
	    if (nsize > max_string_size) {
		return_error(gs_note_error(e_limitcheck));
	    }
	    else {
		nbuf = gs_resize_string(imemory, buf->data, buf->size, nsize,
					"zfilelineedit(grow buffer)");
		if (nbuf == 0) {
		    code = gs_note_error(e_VMerror);
		    return_error(code);
		}
		op->value.bytes = buf->data = nbuf;
		op->tas.rsize = buf->size = nsize;
	    }
	}
	buf->data[count++] = char_EOL;
	s_init(ts, NULL);
	sread_string(ts, buf->data, count);
sc:
	scanner_state_init_check(&state, false, true);
	code = scan_token(i_ctx_p, ts, &ignore_value, &state);
	ref_stack_pop_to(&o_stack, depth);
	if (code < 0)
	    code = scan_EOF;	/* stop on scanner error */
	switch (code) {
	    case 0:		/* read a token */
	    case scan_BOS:
		goto sc;	/* keep going until we run out of data */
	    case scan_Refill:
		goto rd;
	    case scan_EOF:
		break;
	    default:		/* error */
		return code;
	}
    }
    buf->data = gs_resize_string(imemory, buf->data, buf->size, count,
			   "zfilelineedit(resize buffer)");
    if (buf->data == 0)
	return_error(e_VMerror);
    op->value.bytes = buf->data;
    op->tas.rsize = buf->size;

    s = file_alloc_stream(imemory, "zfilelineedit(stream)");
    if (s == 0)
	return_error(e_VMerror);

    sread_string(s, buf->data, count);
    s->save_close = s->procs.close;
    s->procs.close = file_close_disable;

    filename = statement ? gs_iodev_statementedit.dname
	: gs_iodev_lineedit.dname;
    code = ssetfilename(s, (const byte *)filename, strlen(filename)+1);
    if (code < 0) {
	sclose(s);
	return_error(e_VMerror);
    }

    pop(3);
    make_stream_file(osp, s, "r");

    return code;
}

/* ------ Initialization procedure ------ */

const op_def ziodev_op_defs[] =
{
    {"1.getiodevice", zgetiodevice},
    op_def_end(0)
};
