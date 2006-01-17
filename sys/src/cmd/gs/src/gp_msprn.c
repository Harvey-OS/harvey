/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_msprn.c,v 1.4 2004/07/07 09:07:35 ghostgum Exp $ */
/* %printer% IODevice */

#include "windows_.h"
#include "errno_.h"
#include "stdio_.h"
#include "string_.h"
#include "ctype_.h"
#include "fcntl_.h"
#include <io.h>
#include "gp.h"
#include "gscdefs.h"
#include "gserrors.h"
#include "gserror.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gxiodev.h */
#include "gxiodev.h"

/* The MS-Windows printer IODevice */

/*
 * This allows a MS-Windows printer to be specified as an 
 * output using
 *  -sOutputFile="%printer%HP DeskJet 500"
 *
 * To write to a remote printer on another server
 *  -sOutputFile="%printer%\\server\printer name"
 *
 * If you don't supply a printer name you will get
 *  Error: /undefinedfilename in --.outputpage-- 
 * If the printer name is invalid you will get
 *  Error: /invalidfileaccess in --.outputpage-- 
 *
 * This is implemented by returning the file pointer
 * for the write end of a pipe, and starting a thread
 * which reads the pipe and writes to a Windows printer.
 * This will not work in Win32s.
 *
 * The old method provided by gp_open_printer()
 *  -sOutputFile="\\spool\HP DeskJet 500"
 * should not be used except on Win32s.
 * The "\\spool\" is not a UNC name and causes confusion.
 */

private iodev_proc_init(mswin_printer_init);
private iodev_proc_fopen(mswin_printer_fopen);
private iodev_proc_fclose(mswin_printer_fclose);
const gx_io_device gs_iodev_printer = {
    "%printer%", "FileSystem",
    {mswin_printer_init, iodev_no_open_device,
     NULL /*iodev_os_open_file */ , mswin_printer_fopen, mswin_printer_fclose,
     iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

typedef struct tid_s {
    unsigned long tid;
} tid_t;


void mswin_printer_thread(void *arg)
{
    int fd = (int)arg;
    char pname[gp_file_name_sizeof];
    char data[4096];
    HANDLE hprinter = INVALID_HANDLE_VALUE;
    int count;
    DWORD written;
    DOC_INFO_1 di;

    /* Read from pipe and write to Windows printer.
     * First gp_file_name_sizeof bytes are name of the printer.
     */
    if (read(fd, pname, sizeof(pname)) != sizeof(pname)) {
	/* Didn't get the printer name */
	close(fd);
	return;
    }

    while ( (count = read(fd, data, sizeof(data))) > 0 ) {
	if (hprinter == INVALID_HANDLE_VALUE) {
	    if (!OpenPrinter(pname, &hprinter, NULL)) {
		close(fd);
		return;
	    }
	    di.pDocName = (LPTSTR)gs_product;
	    di.pOutputFile = NULL;
	    di.pDatatype = "RAW";
	    if (!StartDocPrinter(hprinter, 1, (LPBYTE) & di)) {
		AbortPrinter(hprinter);
		close(fd);
		return;
	    }
	}
	if (!WritePrinter(hprinter, (LPVOID) data, count, &written)) {
	    AbortPrinter(hprinter);
	    close(fd);
	    return;
	}
    }
    if (hprinter != INVALID_HANDLE_VALUE) {
	if (count == 0) {
	    /* EOF */
	    EndDocPrinter(hprinter);
	    ClosePrinter(hprinter);
	}
	else {
	    /* Error */
	    AbortPrinter(hprinter);
	}
    }
    close(fd);
}

/* The file device procedures */
private int
mswin_printer_init(gx_io_device * iodev, gs_memory_t * mem)
{
    /* state -> structure containing thread handle */
    iodev->state = gs_alloc_bytes(mem, sizeof(tid_t), "mswin_printer_init");
    if (iodev->state == NULL)
        return_error(gs_error_VMerror);
    ((tid_t *)iodev->state)->tid = -1;
    return 0;
}


private int
mswin_printer_fopen(gx_io_device * iodev, const char *fname, const char *access,
	   FILE ** pfile, char *rfname, uint rnamelen)
{
    DWORD version = GetVersion();
    HANDLE hprinter;
    int pipeh[2];
    unsigned long tid;
    HANDLE hthread;
    char pname[gp_file_name_sizeof];
    unsigned long *ptid = &((tid_t *)(iodev->state))->tid;

    /* Win32s supports neither pipes nor Win32 printers. */
    if (((HIWORD(version) & 0x8000) != 0) && 
	((HIWORD(version) & 0x4000) == 0))
	return_error(gs_error_invalidfileaccess);

    /* Make sure that printer exists. */
    if (!OpenPrinter((LPTSTR)fname, &hprinter, NULL))
	return_error(gs_error_invalidfileaccess);
    ClosePrinter(hprinter);

    /* Create a pipe to connect a FILE pointer to a Windows printer. */
    if (_pipe(pipeh, 4096, _O_BINARY) != 0)
	return_error(gs_fopen_errno_to_code(errno));

    *pfile = fdopen(pipeh[1], (char *)access);
    if (*pfile == NULL) {
	close(pipeh[0]);
	close(pipeh[1]);
	return_error(gs_fopen_errno_to_code(errno));
    }

    /* start a thread to read the pipe */
    tid = _beginthread(&mswin_printer_thread, 32768, pipeh[0]);
    if (tid == -1) {
	fclose(*pfile);
	close(pipeh[0]);
	return_error(gs_error_invalidfileaccess);
    }
    /* Duplicate thread handle so we can wait on it
     * even if original handle is closed by CRTL
     * when the thread finishes.
     */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)tid,
	GetCurrentProcess(), &hthread, 
	0, FALSE, DUPLICATE_SAME_ACCESS)) {
	fclose(*pfile);
	return_error(gs_error_invalidfileaccess);
    }
    *ptid = (unsigned long)hthread;

    /* Give the name of the printer to the thread by writing
     * it to the pipe.  This is avoids elaborate thread 
     * synchronisation code.
     */
    strncpy(pname, fname, sizeof(pname));
    fwrite(pname, 1, sizeof(pname), *pfile);

    return 0;
}

private int
mswin_printer_fclose(gx_io_device * iodev, FILE * file)
{
    unsigned long *ptid = &((tid_t *)(iodev->state))->tid;
    HANDLE hthread;
    fclose(file);
    if (*ptid != -1) {
	/* Wait until the print thread finishes before continuing */
	hthread = (HANDLE)*ptid;
	WaitForSingleObject(hthread, 60000);
	CloseHandle(hthread);
	*ptid = -1;
    }
    return 0;
}


