/* Copyright (C) 1993, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ziodev.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
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

/* Define the special devices. */
const char iodev_dtype_stdio[] = "Special";
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

#define STDIN_BUF_SIZE 128
/*#define ref_stdin ref_stdio[0] *//* in files.h */
bool gs_stdin_is_interactive;	/* exported for command line only */
private iodev_proc_init(stdin_init);
private iodev_proc_open_device(stdin_open);
const gx_io_device gs_iodev_stdin =
    iodev_special("%stdin%", stdin_init, stdin_open);

#define STDOUT_BUF_SIZE 128
/*#define ref_stdout ref_stdio[1] *//* in files.h */
private iodev_proc_open_device(stdout_open);
const gx_io_device gs_iodev_stdout =
    iodev_special("%stdout%", iodev_no_init, stdout_open);

#define STDERR_BUF_SIZE 128
/*#define ref_stderr ref_stdio[2] *//* in files.h */
private iodev_proc_open_device(stderr_open);
const gx_io_device gs_iodev_stderr =
    iodev_special("%stderr%", iodev_no_init, stderr_open);

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

/* ------- %stdin, %stdout, and %stderr ------ */

/*
 * According to Adobe, it is legal to close the %std... files and then
 * re-open them later.  However, the re-opened file object is not 'eq' to
 * the original file object (in our implementation, it has a different
 * read_id or write_id).
 */

private int
    s_stdin_read_process(P4(stream_state *, stream_cursor_read *,
			    stream_cursor_write *, bool));

private int
stdin_init(gx_io_device * iodev, gs_memory_t * mem)
{
    gs_stdin_is_interactive = true;
    return 0;
}

/* Read from stdin into the buffer. */
/* If interactive, only read one character. */
private int
s_stdin_read_process(stream_state * st, stream_cursor_read * ignore_pr,
		     stream_cursor_write * pw, bool last)
{
    FILE *file = ((stream *) st)->file;		/* hack for file streams */
    int wcount = (int)(pw->limit - pw->ptr);
    int count;

    if (wcount > 0) {
	if (gs_stdin_is_interactive)
	    wcount = 1;
	count = fread(pw->ptr + 1, 1, wcount, file);
	if (count < 0)
	    count = 0;
	pw->ptr += count;
    } else
	count = 0;		/* return 1 if no error/EOF */
    process_interrupts();
    return (ferror(file) ? ERRC : feof(file) ? EOFC : count == wcount ? 1 : 0);
}

private int
stdin_open(gx_io_device * iodev, const char *access, stream ** ps,
	   gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;

    if (!streq1(access, 'r'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stdin)) {
	/****** stdin SHOULD NOT LINE-BUFFER ******/
	gs_memory_t *mem = imemory_system;
	byte *buf;

	s = file_alloc_stream(mem, "stdin_open(stream)");
	/* We want stdin to read only one character at a time, */
	/* but it must have a substantial buffer, in case it is used */
	/* by a stream that requires more than one input byte */
	/* to make progress. */
	buf = gs_alloc_bytes(mem, STDIN_BUF_SIZE, "stdin_open(buffer)");
	if (s == 0 || buf == 0)
	    return_error(e_VMerror);
	sread_file(s, gs_stdin, buf, STDIN_BUF_SIZE);
	s->procs.process = s_stdin_read_process;
	s->save_close = s_std_null;
	s->procs.close = file_close_file;
	make_file(&ref_stdin, a_readonly | avm_system, s->read_id, s);
	*ps = s;
	return 1;
    }
    *ps = s;
    return 0;
}
/* This is the public routine for getting the stdin stream. */
int
zget_stdin(i_ctx_t *i_ctx_p, stream ** ps)
{
    stream *s;
    gx_io_device *iodev;
    int code;

    if (file_is_valid(s, &ref_stdin)) {
	*ps = s;
	return 0;
    }
    iodev = gs_findiodevice((const byte *)"%stdin", 6);
    iodev->state = i_ctx_p;
    code = (*iodev->procs.open_device)(iodev, "r", ps, imemory_system);
    iodev->state = NULL;
    return min(code, 0);
}
/* Test whether a stream is stdin. */
bool
zis_stdin(const stream *s)
{
    return (s_is_valid(s) && s->procs.process == s_stdin_read_process);
}

private int
stdout_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;

    if (!streq1(access, 'w'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stdout)) {
	gs_memory_t *mem = imemory_system;
	byte *buf;

	s = file_alloc_stream(mem, "stdout_open(stream)");
	buf = gs_alloc_bytes(mem, STDOUT_BUF_SIZE, "stdout_open(buffer)");
	if (s == 0 || buf == 0)
	    return_error(e_VMerror);
	swrite_file(s, gs_stdout, buf, STDOUT_BUF_SIZE);
	s->save_close = s->procs.flush;
	s->procs.close = file_close_file;
	make_file(&ref_stdout, a_write | avm_system, s->write_id, s);
	*ps = s;
	return 1;
    }
    *ps = s;
    return 0;
}
/* This is the public routine for getting the stdout stream. */
int
zget_stdout(i_ctx_t *i_ctx_p, stream ** ps)
{
    stream *s;
    gx_io_device *iodev;
    int code;

    if (file_is_valid(s, &ref_stdout)) {
	*ps = s;
	return 0;
    }
    iodev = gs_findiodevice((const byte *)"%stdout", 7);
    iodev->state = i_ctx_p;
    code = (*iodev->procs.open_device)(iodev, "w", ps, imemory_system);
    iodev->state = NULL;
    return min(code, 0);
}

private int
stderr_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;

    if (!streq1(access, 'w'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stderr)) {
	gs_memory_t *mem = imemory_system;
	byte *buf;

	s = file_alloc_stream(mem, "stderr_open(stream)");
	buf = gs_alloc_bytes(mem, STDERR_BUF_SIZE, "stderr_open(buffer)");
	if (s == 0 || buf == 0)
	    return_error(e_VMerror);
	swrite_file(s, gs_stderr, buf, STDERR_BUF_SIZE);
	s->save_close = s->procs.flush;
	s->procs.close = file_close_file;
	make_file(&ref_stderr, a_write | avm_system, s->write_id, s);
	*ps = s;
	return 1;
    }
    *ps = s;
    return 0;
}
/* This is the public routine for getting the stderr stream. */
int
zget_stderr(i_ctx_t *i_ctx_p, stream ** ps)
{
    stream *s;
    gx_io_device *iodev;
    int code;

    if (file_is_valid(s, &ref_stderr)) {
	*ps = s;
	return 0;
    }
    iodev = gs_findiodevice((const byte *)"%stderr", 7);
    iodev->state = i_ctx_p;
    code = (*iodev->procs.open_device)(iodev, "w", ps, imemory_system);
    iodev->state = NULL;
    return min(code, 0);
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
