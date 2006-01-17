/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpipe.c,v 1.6 2002/02/21 22:24:51 giles Exp $ */
/* %pipe% IODevice */
#include "errno_.h"
#include "pipe_.h"
#include "stdio_.h"
#include "string_.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gxiodev.h */
#include "gxiodev.h"

/* The pipe IODevice */
private iodev_proc_fopen(pipe_fopen);
private iodev_proc_fclose(pipe_fclose);
const gx_io_device gs_iodev_pipe = {
    "%pipe%", "Special",
    {iodev_no_init, iodev_no_open_device,
     NULL /*iodev_os_open_file */ , pipe_fopen, pipe_fclose,
     iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

/* The file device procedures */

private int
pipe_fopen(gx_io_device * iodev, const char *fname, const char *access,
	   FILE ** pfile, char *rfname, uint rnamelen)
{
    errno = 0;
    /*
     * Some platforms allow opening a pipe with a '+' in the access
     * mode, even though pipes are not positionable.  Detect this here.
     */
    if (strchr(access, '+'))
	return_error(gs_error_invalidfileaccess);
    /*
     * The OSF/1 1.3 library doesn't include const in the
     * prototype for popen, so we have to break const here.
     */
    *pfile = popen((char *)fname, (char *)access);
    if (*pfile == NULL)
	return_error(gs_fopen_errno_to_code(errno));
    if (rfname != NULL)
	strcpy(rfname, fname);
    return 0;
}

private int
pipe_fclose(gx_io_device * iodev, FILE * file)
{
    pclose(file);
    return 0;
}
