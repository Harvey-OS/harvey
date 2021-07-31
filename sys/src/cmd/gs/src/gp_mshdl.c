/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gp_mshdl.c,v 1.2 2000/09/19 19:00:24 lpd Exp $ */
/* %handle% IODevice */
#include "errno_.h"
#include "stdio_.h"
#include "string_.h"
#include "ctype_.h"
#include <io.h>
#include "gserror.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gxiodev.h */
#include "gxiodev.h"

/* The MS-Windows handle IODevice */

/* This allows an MS-Windows file handle to be passed in to
 * Ghostscript as %handle%NNNNNNNN where NNNNNNNN is the hexadecimal
 * value of the handle.  
 * The typical use is for another program to create a pipe, 
 * pass the write end into Ghostscript using 
 *  -sOutputFile="%handle%NNNNNNNN"
 * so that Ghostscript printer output can be captured by the
 * other program.  The handle would be created with CreatePipe().
 * If Ghostscript is not a DLL, the pipe will have to be inheritable 
 * by the Ghostscript process.
 */

private iodev_proc_fopen(mswin_handle_fopen);
private iodev_proc_fclose(mswin_handle_fclose);
const gx_io_device gs_iodev_handle = {
    "%handle%", "FileSystem",
    {iodev_no_init, iodev_no_open_device,
     NULL /*iodev_os_open_file */ , mswin_handle_fopen, mswin_handle_fclose,
     iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

/* The file device procedures */
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif

/* Allow printer filename specified by -sOutputFile="..."
 * to contain a hexadecimal encoded OS file handle in the
 * form -sOutputFile="\\handle\\000001c5"
 *
 * Returns:
 *   The OS file handle on success.
 *   INVALID_HANDLE_VALUE on failure.
 *
 * This allows the caller to create an OS pipe and give us a 
 * handle to the write end of the pipe.
 * If we are called as an EXE, the OS file handle must be 
 * inherited by Ghostscript.
 * Pipes aren't supported under Win32s.
 */
private long 
get_os_handle(const char *name)
{
    ulong hfile;	/* This must be as long as the longest handle. */
			/* This is correct for Win32, maybe wrong for Win64. */
    int i, ch;

    for (i = 0; (ch = name[i]) != 0; ++i)
	if (!isxdigit(ch))
	    return (long)INVALID_HANDLE_VALUE;
    if (sscanf(name, "%lx", &hfile) != 1)
	return (long)INVALID_HANDLE_VALUE;
    return (long)hfile; 
}

private int
mswin_handle_fopen(gx_io_device * iodev, const char *fname, const char *access,
	   FILE ** pfile, char *rfname, uint rnamelen)
{
    int fd;
    long hfile;	/* Correct for Win32, may be wrong for Win64 */
    errno = 0;

    if ((hfile = get_os_handle(fname)) == (long)INVALID_HANDLE_VALUE)
	return_error(gs_fopen_errno_to_code(EBADF));

    /* associate a C file handle with an OS file handle */
    fd = _open_osfhandle((long)hfile, 0);
    if (fd == -1)
	return_error(gs_fopen_errno_to_code(EBADF));

    /* associate a C file stream with C file handle */
    *pfile = fdopen(fd, (char *)access);
    if (*pfile == NULL)
	return_error(gs_fopen_errno_to_code(errno));

    if (rfname != NULL)
	strcpy(rfname, fname);
    return 0;
}

private int
mswin_handle_fclose(gx_io_device * iodev, FILE * file)
{
    fclose(file);
    return 0;
}
