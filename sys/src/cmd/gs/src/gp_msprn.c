/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/*$Id: gp_msprn.c,v 1.1 2001/03/26 11:28:20 ghostgum Exp $ */
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

    if (iodev->state == NULL)
	return_error(gs_error_invalidfileaccess);

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
    *ptid = _beginthread(&mswin_printer_thread, 32768, pipeh[0]);
    if (*ptid == -1) {
	fclose(*pfile);
	close(pipeh[0]);
	return_error(gs_error_invalidfileaccess);
    }

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
    if ((iodev->state != NULL) && (*ptid != -1)) {
	/* Wait until the print thread finishes before continuing */
	hthread = (HANDLE)*ptid;
	WaitForSingleObject(hthread, 60000);
	CloseHandle(hthread);
	*ptid = -1;
    }
    return 0;
}


