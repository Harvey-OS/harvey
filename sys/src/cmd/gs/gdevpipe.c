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

/* gdevpipe.c */
/* %pipe% IODevice */
#include "errno_.h"
#include "stdio_.h"
#include "string_.h"
#include "gserror.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gxiodev.h */
#include "stream.h"
#include "gxiodev.h"

/* popen isn't POSIX-standard, so we declare it here. */
/* Because of inconsistent (and sometimes incorrect) header files, */
/* we must omit the argument list. */
extern FILE *popen( /* P2(const char *, const char *) */ );
extern int pclose(P1(FILE *));

/* The pipe IODevice */
private iodev_proc_fopen(pipe_fopen);
private iodev_proc_fclose(pipe_fclose);
gx_io_device gs_iodev_pipe = {
	"%pipe%", "FileSystem",
	{ iodev_no_init, iodev_no_open_device,
	  NULL /*iodev_os_open_file*/, pipe_fopen, pipe_fclose,
	  iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
	  iodev_no_enumerate_files, NULL, NULL,
	  iodev_no_get_params, iodev_no_put_params
	}
};

/* The file device procedures */

private int
pipe_fopen(gx_io_device *iodev, const char *fname, const char *access,
  FILE **pfile, char *rfname, uint rnamelen)
{	/* The OSF/1 1.3 library doesn't include const in the */
	/* prototype for popen.... */
	errno = 0;
	*pfile = popen((char *)fname, (char *)access);
	if ( *pfile == NULL )
	  return_error(gs_fopen_errno_to_code(errno));
	if ( rfname != NULL )
	  strcpy(rfname, fname);
	return 0;
}

private int
pipe_fclose(gx_io_device *iodev, FILE *file)
{	pclose(file);
	return 0;
}
