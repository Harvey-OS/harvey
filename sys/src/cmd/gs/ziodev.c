/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* ziodev.c */
/* Standard IODevice implementation */
#include "stdio_.h"
#include "string_.h"
#include "ghost.h"
#include "gp.h"
#include "gpcheck.h"
#include "gsstruct.h"			/* for registering root */
#include "gsparam.h"
#include "errors.h"
#include "oper.h"
#include "stream.h"
#include "ialloc.h"
#include "ivmspace.h"
#include "gxiodev.h"			/* must come after stream.h */
					/* and before files.h */
#include "files.h"
#include "store.h"

/* Complete the definition of the %os% device. */
/* The open_file routine is exported for pipes and for %null. */
int
iodev_os_open_file(gx_io_device *iodev, const char *fname, uint len,
  const char *file_access, stream **ps, gs_memory_t *mem)
{	return file_open_stream(fname, len, file_access,
				file_default_buffer_size, ps,
				iodev->procs.fopen);
}

/* Define the special devices. */
#define iodev_special(dname, init, open)\
  { dname, "Special",\
     { init, open, iodev_no_open_file, iodev_no_fopen, iodev_no_fclose,\
       iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,\
       iodev_no_enumerate_files, NULL, NULL,\
       iodev_no_get_params, iodev_no_put_params\
     }\
  }

#define stdin_buf_size 128
stream *gs_stream_stdin;	/* exported for zfileio.c only */
bool gs_stdin_is_interactive;	/* exported for command line only */
private iodev_proc_init(stdin_init);
private iodev_proc_open_device(stdin_open);
gx_io_device gs_iodev_stdin =
  iodev_special("%stdin%", stdin_init, stdin_open);

#define stdout_buf_size 128
stream *gs_stream_stdout;	/* exported for zfileio.c only */
private iodev_proc_init(stdout_init);
private iodev_proc_open_device(stdout_open);
gx_io_device gs_iodev_stdout =
  iodev_special("%stdout%", stdout_init, stdout_open);

#define stderr_buf_size 128
stream *gs_stream_stderr;	/* exported for zfileio.c only */
private iodev_proc_init(stderr_init);
private iodev_proc_open_device(stderr_open);
gx_io_device gs_iodev_stderr =
  iodev_special("%stderr%", stderr_init, stderr_open);

#define lineedit_buf_size 160
private iodev_proc_open_device(lineedit_open);
gx_io_device gs_iodev_lineedit =
  iodev_special("%lineedit%", iodev_no_init, lineedit_open);

private iodev_proc_open_device(statementedit_open);
gx_io_device gs_iodev_statementedit =
  iodev_special("%statementedit%", iodev_no_init, statementedit_open);

/* ------ Operators ------ */

/* <int> .getiodevice <string> */
int
zgetiodevice(register os_ptr op)
{	gx_io_device *iodev;
	const byte *dname;
	check_type(*op, t_integer);
	if ( op->value.intval != (int)op->value.intval )
		return_error(e_rangecheck);
	iodev = gs_getiodevice((int)(op->value.intval));
	if ( iodev == 0 )		/* index out of range */
		return_error(e_rangecheck);
	dname = (const byte *)iodev->dname;
	make_const_string(op, a_readonly | avm_foreign,
			  strlen((char *)dname), dname);
	return 0;
}

/* ------- %stdin, %stdout, and %stderr ------ */

private gs_gc_root_t stdin_root, stdout_root, stderr_root;

private int
  s_stdin_read_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));

private int
stdin_init(gx_io_device *iodev, gs_memory_t *mem)
{
	/****** stdin SHOULD NOT LINE-BUFFER ******/

	byte *buf = gs_alloc_bytes(mem, stdin_buf_size, "stdin_init");
	stream *s = s_alloc(mem, "stdin_init");
	sread_file(s, gs_stdin, buf, stdin_buf_size);
	/* We want stdin to read only one character at a time, */
	/* but it must have a substantial buffer, in case it is used */
	/* by a stream that requires more than one input byte */
	/* to make progress. */
	s->procs.process = s_stdin_read_process;
	s_init_read_id(s);
	s->procs.close = file_close_finish;
	s->prev = s->next = 0;
	gs_stream_stdin = s;
	gs_stdin_is_interactive = true;
	gs_register_struct_root(imemory, &stdin_root,
				(void **)&gs_stream_stdin, "gs_stream_stdin");
	return 0;
}

/* Read from stdin into the buffer. */
/* If interactive, only read one character. */
private int
s_stdin_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	FILE *file = ((stream *)st)->file;	/* hack for file streams */
	int wcount = (int)(pw->limit - pw->ptr);
	int count;

	if ( wcount > 0 )
	  {	if ( gs_stdin_is_interactive )
		  wcount = 1;
		count = fread(pw->ptr + 1, 1, wcount, file);
		if ( count < 0 )
		  count = 0;
		pw->ptr += count;
	  }
	else
	  count = 0;		/* return 1 if no error/EOF */
	process_interrupts();
	return (ferror(file) ? ERRC : feof(file) ? EOFC : count == wcount ? 1 : 0);
}

private int
stdin_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	if ( !streq1(access, 'r') )
		return_error(e_invalidfileaccess);
	*ps = gs_stream_stdin;
	return 0;
}

private int
stdout_init(gx_io_device *iodev, gs_memory_t *mem)
{	byte *buf = gs_alloc_bytes(mem, stdout_buf_size, "stdout_init");
	stream *s = s_alloc(mem, "stdout_init");
	swrite_file(s, gs_stdout, buf, stdout_buf_size);
	s_init_write_id(s);
	s->procs.close = file_close_finish;
	s->prev = s->next = 0;
	gs_stream_stdout = s;
	gs_register_struct_root(imemory, &stdout_root,
				(void **)&gs_stream_stdout, "gs_stream_stdout");
	return 0;
}

private int
stdout_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	if ( !streq1(access, 'w') )
		return_error(e_invalidfileaccess);
	*ps = gs_stream_stdout;
	return 0;
}

private int
stderr_init(gx_io_device *iodev, gs_memory_t *mem)
{	byte *buf = gs_alloc_bytes(mem, stderr_buf_size, "stderr_init");
	stream *s = s_alloc(mem, "stderr_init");
	swrite_file(s, gs_stderr, buf, stderr_buf_size);
	s_init_write_id(s);
	s->procs.close = file_close_finish;
	s->prev = s->next = 0;
	gs_stream_stderr = s;
	gs_register_struct_root(imemory, &stderr_root,
				(void **)&gs_stream_stderr, "gs_stream_stderr");
	return 0;
}

private int
stderr_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	if ( !streq1(access, 'w') )
		return_error(e_invalidfileaccess);
	*ps = gs_stream_stderr;
	return 0;
}

/* ------ %lineedit and %statementedit ------ */

private int
lineedit_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	uint count = 0;
	bool in_eol = false;
	int code;
	stream *s;
	byte *buf;
#define max_line lineedit_buf_size
	byte line[max_line];
	if ( strcmp(access, "r") )
		return_error(e_invalidfileaccess);
	s = file_alloc_stream("lineedit_open(stream)");
	if ( s == 0 )
		return_error(e_VMerror);
	code = zreadline_from(gs_stream_stdin, line, max_line,
			      &count, &in_eol);
#undef max_line
	if ( code != 0 )
		return_error((code == EOFC ? e_undefinedfilename : e_ioerror));
	buf = gs_alloc_string(mem, count, "lineedit_open(buffer)");
	if ( buf == 0 )
		return_error(e_VMerror);
	memcpy(buf, line, count);
	sread_string(s, buf, count);
	s->save_close = s->procs.close;
	s->procs.close = file_close_disable;
	*ps = s;
	return 0;
}

private int
statementedit_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	/* NOT IMPLEMENTED PROPERLY YET */
	return lineedit_open(iodev, access, ps, mem);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(ziodev_op_defs) {
	{"1.getiodevice", zgetiodevice},
END_OP_DEFS(0) }
