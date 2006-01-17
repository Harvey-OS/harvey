/* Copyright (C) 2004 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_os2pr.c,v 1.1 2004/09/14 06:45:55 ghostgum Exp $ */
/* %printer% IODevice */

#define INCL_DOS
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_SPLERRORS
#define INCL_BASE
#define INCL_ERRORS
#define INCL_WIN
#include <os2.h>

#include "errno_.h"
#include "stdio_.h"
#include "string_.h"
#include "gp.h"
#include "gscdefs.h"
#include "gserrors.h"
#include "gserror.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gxiodev.h */
#include "gxiodev.h"

/* The OS/2 printer IODevice */

/*
 * This allows an OS/2 printer to be specified as an 
 * output using
 *  -sOutputFile="%printer%AppleLas"
 * where "AppleLas" is the physical name of the queue.
 *
 * If you don't supply a printer name you will get
 *  Error: /undefinedfilename in --.outputpage-- 
 * If the printer name is invalid you will get
 *  Error: /invalidfileaccess in --.outputpage-- 
 *
 * This is implemented by writing to a temporary file
 * then copying it to the spooler.
 *
 * A better method would be to return a file pointer
 * to the write end of a pipe, and starting a thread
 * which reads the pipe and writes to an OS/2 printer.
 * This method didn't work properly on the second
 * thread within ghostscript.
 */

private iodev_proc_init(os2_printer_init);
private iodev_proc_fopen(os2_printer_fopen);
private iodev_proc_fclose(os2_printer_fclose);
const gx_io_device gs_iodev_printer = {
    "%printer%", "FileSystem",
    {os2_printer_init, iodev_no_open_device,
     NULL /*iodev_os_open_file */ , os2_printer_fopen, os2_printer_fclose,
     iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

typedef struct os2_printer_s {
    char queue[gp_file_name_sizeof];
    char filename[gp_file_name_sizeof];
} os2_printer_t;

/* The file device procedures */
private int
os2_printer_init(gx_io_device * iodev, gs_memory_t * mem)
{
    /* state -> structure containing thread handle */
    iodev->state = gs_alloc_bytes(mem, sizeof(os2_printer_t), 
	"os2_printer_init");
    if (iodev->state == NULL)
        return_error(gs_error_VMerror);
    memset(iodev->state, 0, sizeof(os2_printer_t));
    return 0;
}


private int
os2_printer_fopen(gx_io_device * iodev, const char *fname, const char *access,
	   FILE ** pfile, char *rfname, uint rnamelen)
{
    os2_printer_t *pr = (os2_printer_t *)iodev->state;
    char driver_name[256];

    /* Make sure that printer exists. */
    if (pm_find_queue(fname, driver_name)) {
	/* error, list valid queue names */
	eprintf("Invalid queue name.  Use one of:\n");
	pm_find_queue(NULL, NULL);
	return_error(gs_error_undefinedfilename);
    }

    strncpy(pr->queue, fname, sizeof(pr->queue)-1);

    /* Create a temporary file */
    *pfile = gp_open_scratch_file("gs", pr->filename, access);
    if (*pfile == NULL)
	return_error(gs_fopen_errno_to_code(errno));

    return 0;
}

private int
os2_printer_fclose(gx_io_device * iodev, FILE * file)
{
    os2_printer_t *pr = (os2_printer_t *)iodev->state;
    fclose(file);
    pm_spool(pr->filename, pr->queue); 
    unlink(pr->filename);
    return 0;
}

