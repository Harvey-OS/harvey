/* Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ziodevsc.c,v 1.7 2004/08/04 19:36:13 stefan Exp $ */
/* %stdxxx IODevice implementation using callouts for PostScript interpreter */
#include "stdio_.h"
#include "ghost.h"
#include "gpcheck.h"
#include "oper.h"
#include "stream.h"
#include "gxiodev.h"		/* must come after stream.h */
				/* and before files.h */
#include "istream.h"
#include "files.h"
#include "ifilter.h"
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
private iodev_proc_init(stdin_init);
private iodev_proc_open_device(stdin_open);
const gx_io_device gs_iodev_stdin =
    iodev_special("%stdin%", stdin_init, stdin_open);

#define STDOUT_BUF_SIZE 128
private iodev_proc_open_device(stdout_open);
const gx_io_device gs_iodev_stdout =
    iodev_special("%stdout%", iodev_no_init, stdout_open);

#define STDERR_BUF_SIZE 128
private iodev_proc_open_device(stderr_open);
const gx_io_device gs_iodev_stderr =
    iodev_special("%stderr%", iodev_no_init, stderr_open);

/* ------- %stdin, %stdout, and %stderr ------ */

/*
 * According to Adobe, it is legal to close the %std... files and then
 * re-open them later.  However, the re-opened file object is not 'eq' to
 * the original file object (in our implementation, it has a different
 * read_id or write_id).
 */

private int
stdio_close(stream *s)
{
    int code = (*s->save_close)(s);
    if (code)
	return code;
    /* Increment the IDs to prevent further access. */
    s->read_id = s->write_id = (s->read_id | s->write_id) + 1;
    return 0;
}

private int
stdin_init(gx_io_device * iodev, gs_memory_t * mem)
{
    mem->gs_lib_ctx->stdin_is_interactive = true;
    return 0;
}

/* stdin stream implemented as procedure */
private int
stdin_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;
    int code;

    if (!streq1(access, 'r'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stdin)) {
	/* procedure source */
	gs_ref_memory_t *imem = (gs_ref_memory_t *)imemory_system;
	ref rint;

	/* The procedure isn't used. */
	/* Set it to literal 0 to recognised stdin. */
	make_int(&rint, 0);

	/* implement stdin as a procedure */
	code = sread_proc(&rint, &s, imem);
	if (code < 0)
	    return code;
	s->save_close = s_std_null;
	s->procs.close = stdio_close;
	/* allocate buffer */
	if (s->cbuf == 0) {
	    int len = STDIN_BUF_SIZE;
	    byte *buf = gs_alloc_bytes((gs_memory_t *)imemory_system,
	    		len, "stdin_open");
	    if (buf == 0)
		return_error(e_VMerror);
	    s->cbuf = buf;
	    s->srptr = s->srlimit = s->swptr = buf - 1;
	    s->swlimit = buf - 1 + len;
	    s->bsize = s->cbsize = len;
	}
	s->state->min_left = 0;
	make_file(&ref_stdin, a_read | avm_system, s->read_id, s);
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
    /* Only stdin should be a procedure based stream, opened for
     * reading and with a literal 0 as the procedure.
     */
    if (s_is_valid(s) && s_is_reading(s) && s_is_proc(s)) {
        stream_proc_state *state = (stream_proc_state *)s->state;
	if ((r_type(&(state->proc)) == t_integer) &&
		(state->proc.value.intval == 0))
	    return true;
    }
    return false;
}

/* stdout stream implemented as procedure */
private int
stdout_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;
    int code;

    if (!streq1(access, 'w'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stdout)) {
	/* procedure source */
	gs_ref_memory_t *imem = (gs_ref_memory_t *)imemory_system;
	ref rint;

	/* The procedure isn't used. */
	/* Set it to literal 1 to recognised stdout. */
	make_int(&rint, 1);

	/* implement stdout as a procedure */
	code = swrite_proc(&rint, &s, imem);
	if (code < 0)
	    return code;
	s->save_close = s->procs.flush;
	s->procs.close = stdio_close;
	/* allocate buffer */
	if (s->cbuf == 0) {
	    int len = STDOUT_BUF_SIZE;
	    byte *buf = gs_alloc_bytes((gs_memory_t *)imemory_system,
	    		len, "stdout_open");
	    if (buf == 0)
		return_error(e_VMerror);
	    s->cbuf = buf;
	    s->srptr = s->srlimit = s->swptr = buf - 1;
	    s->swlimit = buf - 1 + len;
	    s->bsize = s->cbsize = len;
	}
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

/* stderr stream implemented as procedure */
private int
stderr_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    i_ctx_t *i_ctx_p = (i_ctx_t *)iodev->state;	/* see above */
    stream *s;
    int code;

    if (!streq1(access, 'w'))
	return_error(e_invalidfileaccess);
    if (file_is_invalid(s, &ref_stderr)) {
	/* procedure source */
	gs_ref_memory_t *imem = (gs_ref_memory_t *)imemory_system;
	ref rint;

	/* The procedure isn't used. */
	/* Set it to literal 2 to recognised stderr. */
	make_int(&rint, 2);

	/* implement stderr as a procedure */
	code = swrite_proc(&rint, &s, imem);
	if (code < 0)
	    return code;
	s->save_close = s->procs.flush;
	s->procs.close = stdio_close;
	/* allocate buffer */
	if (s->cbuf == 0) {
	    int len = STDERR_BUF_SIZE;
	    byte *buf = gs_alloc_bytes((gs_memory_t *)imemory_system,
	    		len, "stderr_open");
	    if (buf == 0)
		return_error(e_VMerror);
	    s->cbuf = buf;
	    s->srptr = s->srlimit = s->swptr = buf - 1;
	    s->swlimit = buf - 1 + len;
	    s->bsize = s->cbsize = len;
	}
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
