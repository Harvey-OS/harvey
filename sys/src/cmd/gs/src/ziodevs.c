/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: ziodevs.c,v 1.2 2000/09/19 19:00:54 lpd Exp $ */
/* %stdxxx IODevice implementation for PostScript interpreter */
#include "stdio_.h"
#include "ghost.h"
#include "gpcheck.h"
#include "oper.h"
#include "stream.h"
#include "gxiodev.h"		/* must come after stream.h */
				/* and before files.h */
#include "files.h"
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
