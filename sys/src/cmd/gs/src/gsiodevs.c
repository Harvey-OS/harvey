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

/*$Id: gsiodevs.c,v 1.2 2000/09/19 19:00:29 lpd Exp $ */
/* %stdxxx IODevice implementation for non-PostScript configurations */
#include "gx.h"
#include "gserrors.h"
#include "gxiodev.h"
#include "stream.h"
#include "strimpl.h"

const char iodev_dtype_stdio[] = "Special";
#define iodev_stdio(dname, open) {\
    dname, iodev_dtype_stdio,\
	{ iodev_no_init, open, iodev_no_open_file,\
	  iodev_no_fopen, iodev_no_fclose,\
	  iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,\
	  iodev_no_enumerate_files, NULL, NULL,\
	  iodev_no_get_params, iodev_no_put_params\
	}\
}

#define STDIO_BUF_SIZE 128
private int
stdio_close_file(stream *s)
{
    /* Don't close stdio files, but do free the buffer. */
    gs_memory_t *mem = s->memory;

    s->file = 0;
    gs_free_object(mem, s->cbuf, "stdio_close_file(buffer)");
    return 0;
}
private int
stdio_open(gx_io_device * iodev, const char *access, stream ** ps,
	   gs_memory_t * mem, char rw, FILE *file,
	   void (*srw_file)(P4(stream *, FILE *, byte *, uint)))
{
    stream *s;
    byte *buf;

    if (!streq1(access, rw))
	return_error(gs_error_invalidfileaccess);
    s = s_alloc(mem, "stdio_open(stream)");
    buf = gs_alloc_bytes(mem, STDIO_BUF_SIZE, "stdio_open(buffer)");
    if (s == 0 || buf == 0) {
	gs_free_object(mem, buf, "stdio_open(buffer)");
	gs_free_object(mem, s, "stdio_open(stream)");
	return_error(gs_error_VMerror);
    }
    srw_file(s, file, buf, STDIO_BUF_SIZE);
    s->procs.close = stdio_close_file;
    *ps = s;
    return 0;
}

private int
stdin_open(gx_io_device * iodev, const char *access, stream ** ps,
	   gs_memory_t * mem)
{
    return stdio_open(iodev, access, ps, mem, 'r', gs_stdin, sread_file);
}
const gx_io_device gs_iodev_stdin = iodev_stdio("%stdin%", stdin_open);

private int
stdout_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    return stdio_open(iodev, access, ps, mem, 'w', gs_stdout, swrite_file);
}
const gx_io_device gs_iodev_stdout = iodev_stdio("%stdout%", stdout_open);

private int
stderr_open(gx_io_device * iodev, const char *access, stream ** ps,
	    gs_memory_t * mem)
{
    return stdio_open(iodev, access, ps, mem, 'w', gs_stderr, swrite_file);
}
const gx_io_device gs_iodev_stderr = iodev_stdio("%stderr%", stderr_open);
