/* Copyright (C) 1993, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: ziodev.c,v 1.3 2000/09/19 19:00:54 lpd Exp $ */
/* Standard IODevice implementation */
#include "memory_.h"
#include "stdio_.h"
#include "string_.h"
#include "ghost.h"
#include "gp.h"
#include "gpcheck.h"
#include "oper.h"
#include "stream.h"
#include "ialloc.h"
#include "iscan.h"
#include "ivmspace.h"
#include "gxiodev.h"		/* must come after stream.h */
				/* and before files.h */
#include "files.h"
#include "scanchar.h"		/* for char_EOL */
#include "store.h"

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
private iodev_proc_open_device(lineedit_open);
const gx_io_device gs_iodev_lineedit =
    iodev_special("%lineedit%", iodev_no_init, lineedit_open);

#define STATEMENTEDIT_BUF_SIZE 50	/* initial size, not fixed size */
private iodev_proc_open_device(statementedit_open);
const gx_io_device gs_iodev_statementedit =
    iodev_special("%statementedit%", iodev_no_init, statementedit_open);

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

private int
line_collect(gx_io_device * iodev, const char *access, stream ** ps,
	     gs_memory_t * mem, uint initial_buf_size, bool statement)
{
    uint count = 0;
    bool in_eol = false;
    int code;
    gx_io_device *indev = gs_findiodevice((const byte *)"%stdin", 6);
    /* HACK: get the context pointer from the IODevice.  See above. */
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;
    stream *s;
    stream *ins;
    gs_string str;
    /*
     * buf exists only for stylistic parallelism: all occurrences of
     * buf-> could just as well be str. .
     */
    gs_string *const buf = &str;

    if (strcmp(access, "r"))
	return_error(e_invalidfileaccess);
    s = file_alloc_stream(mem, "line_collect(stream)");
    if (s == 0)
	return_error(e_VMerror);
    /* HACK: see above. */
    indev->state = i_ctx_p;
    code = (indev->procs.open_device)(indev, access, &ins, mem);
    indev->state = 0;
    if (code < 0)
	return code;
    buf->size = initial_buf_size;
    buf->data = gs_alloc_string(mem, buf->size, "line_collect(buffer)");
    if (buf->data == 0)
	return_error(e_VMerror);
rd:
    /*
     * We have to stop 1 character short of the buffer size,
     * because %statementedit must append an EOL.
     */
    buf->size--;
    /* HACK: set %stdin's state so that GNU readline can retrieve it. */
    indev->state = i_ctx_p;
    code = zreadline_from(ins, buf, mem, &count, &in_eol);
    buf->size++;		/* restore correct size */
    indev->state = 0;
    switch (code) {
	case EOFC:
	    code = gs_note_error(e_undefinedfilename);
	    /* falls through */
	case 0:
	    break;
	default:
	    code = gs_note_error(e_ioerror);
	    break;
	case 1:		/* filled buffer */
	    {
		uint nsize;
		byte *nbuf;

#if arch_ints_are_short
		if (nsize == max_uint) {
		    code = gs_note_error(e_limitcheck);
		    break;
		} else if (nsize >= max_uint / 2)
		    nsize = max_uint;
		else
#endif
		    nsize = buf->size * 2;
		nbuf = gs_resize_string(mem, buf->data, buf->size, nsize,
					"line_collect(grow buffer)");
		if (nbuf == 0) {
		    code = gs_note_error(e_VMerror);
		    break;
		}
		buf->data = nbuf;
		buf->size = nsize;
		goto rd;
	    }
    }
    if (code != 0) {
	gs_free_string(mem, buf->data, buf->size, "line_collect(buffer)");
	return code;
    }
    if (statement) {
	/* If we don't have a complete token, keep going. */
	stream st;
	stream *ts = &st;
	scanner_state state;
	ref ignore_value;
	uint depth = ref_stack_count(&o_stack);
	int code;

	/* Add a terminating EOL. */
	buf->data[count++] = char_EOL;
	sread_string(ts, buf->data, count);
sc:
	scanner_state_init_check(&state, false, true);
	code = scan_token(i_ctx_p, ts, &ignore_value, &state);
	ref_stack_pop_to(&o_stack, depth);
	switch (code) {
	    case 0:		/* read a token */
	    case scan_BOS:
		goto sc;	/* keep going until we run out of data */
	    case scan_Refill:
		goto rd;
	    case scan_EOF:
		break;
	    default:		/* error */
		gs_free_string(mem, buf->data, buf->size, "line_collect(buffer)");
		return code;
	}
    }
    buf->data = gs_resize_string(mem, buf->data, buf->size, count,
			   "line_collect(resize buffer)");
    if (buf->data == 0)
	return_error(e_VMerror);
    sread_string(s, buf->data, count);
    s->save_close = s->procs.close;
    s->procs.close = file_close_disable;
    *ps = s;
    return 0;
}

private int
lineedit_open(gx_io_device * iodev, const char *access, stream ** ps,
	      gs_memory_t * mem)
{
    int code = line_collect(iodev, access, ps, mem,
			    LINEEDIT_BUF_SIZE, false);

    return (code < 0 ? code : 1);
}

private int
statementedit_open(gx_io_device * iodev, const char *access, stream ** ps,
		   gs_memory_t * mem)
{
    int code = line_collect(iodev, access, ps, mem,
			    STATEMENTEDIT_BUF_SIZE, true);

    return (code < 0 ? code : 1);
}

/* ------ Initialization procedure ------ */

const op_def ziodev_op_defs[] =
{
    {"1.getiodevice", zgetiodevice},
    op_def_end(0)
};
