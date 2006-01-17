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

/* $Id: gsiodev.c,v 1.8 2005/07/28 15:24:29 alexcher Exp $ */
/* IODevice implementation for Ghostscript */
#include "errno_.h"
#include "string_.h"
#include "unistd_.h"
#include "gx.h"
#include "gserrors.h"
#include "gp.h"
#include "gscdefs.h"
#include "gsparam.h"
#include "gsstruct.h"
#include "gxiodev.h"

/* Import the IODevice table from gconf.c. */
extern_gx_io_device_table();

/* Define a table of local copies of the IODevices, */
/* allocated at startup.  This just postpones the day of reckoning.... */
private gx_io_device **io_device_table;

private_st_io_device();
gs_private_st_ptr(st_io_device_ptr, gx_io_device *, "gx_io_device *",
		  iodev_ptr_enum_ptrs, iodev_ptr_reloc_ptrs);
gs_private_st_element(st_io_device_ptr_element, gx_io_device *,
      "gx_io_device *[]", iodev_ptr_elt_enum_ptrs, iodev_ptr_elt_reloc_ptrs,
		      st_io_device_ptr);

/* Define the OS (%os%) device. */
iodev_proc_fopen(iodev_os_fopen);
iodev_proc_fclose(iodev_os_fclose);
private iodev_proc_delete_file(os_delete);
private iodev_proc_rename_file(os_rename);
private iodev_proc_file_status(os_status);
private iodev_proc_enumerate_files(os_enumerate);
private iodev_proc_get_params(os_get_params);
const gx_io_device gs_iodev_os =
{
    "%os%", "FileSystem",
    {iodev_no_init, iodev_no_open_device,
     NULL /*iodev_os_open_file */ , iodev_os_fopen, iodev_os_fclose,
     os_delete, os_rename, os_status,
     os_enumerate, gp_enumerate_files_next, gp_enumerate_files_close,
     os_get_params, iodev_no_put_params
    }
};

/* ------ Initialization ------ */

init_proc(gs_iodev_init);	/* check prototype */
int
gs_iodev_init(gs_memory_t * mem)
{				/* Make writable copies of all IODevices. */
    gx_io_device **table =
	gs_alloc_struct_array(mem, gx_io_device_table_count,
			      gx_io_device *, &st_io_device_ptr_element,
			      "gs_iodev_init(table)");
    int i, j;
    int code = 0;

    if (table == 0)
	return_error(gs_error_VMerror);
    for (i = 0; i < gx_io_device_table_count; ++i) {
	gx_io_device *iodev =
	    gs_alloc_struct(mem, gx_io_device, &st_io_device,
			    "gs_iodev_init(iodev)");

	if (iodev == 0)
	    goto fail;
	table[i] = iodev;
	memcpy(table[i], gx_io_device_table[i], sizeof(gx_io_device));
    }
    io_device_table = table;
    code = gs_register_struct_root(mem, NULL, (void **)&io_device_table,
				   "io_device_table");
    if (code < 0)
	goto fail;
    /* Run the one-time initialization of each IODevice. */
    for (j = 0; j < gx_io_device_table_count; ++j)
	if ((code = (table[j]->procs.init)(table[j], mem)) < 0)
	    goto f2;
    return 0;
 f2:
    /****** CAN'T FIND THE ROOT ******/
    /*gs_unregister_root(mem, root, "io_device_table");*/
 fail:
    for (; i >= 0; --i)
	gs_free_object(mem, table[i - 1], "gs_iodev_init(iodev)");
    gs_free_object(mem, table, "gs_iodev_init(table)");
    io_device_table = 0;
    return (code < 0 ? code : gs_note_error(gs_error_VMerror));
}

/* ------ Default (unimplemented) IODevice procedures ------ */

int
iodev_no_init(gx_io_device * iodev, gs_memory_t * mem)
{
    return 0;
}

int
iodev_no_open_device(gx_io_device * iodev, const char *access, stream ** ps,
		     gs_memory_t * mem)
{
    return_error(gs_error_invalidfileaccess);
}

int
iodev_no_open_file(gx_io_device * iodev, const char *fname, uint namelen,
		   const char *access, stream ** ps, gs_memory_t * mem)
{
    return_error(gs_error_invalidfileaccess);
}

int
iodev_no_fopen(gx_io_device * iodev, const char *fname, const char *access,
	       FILE ** pfile, char *rfname, uint rnamelen)
{
    return_error(gs_error_invalidfileaccess);
}

int
iodev_no_fclose(gx_io_device * iodev, FILE * file)
{
    return_error(gs_error_ioerror);
}

int
iodev_no_delete_file(gx_io_device * iodev, const char *fname)
{
    return_error(gs_error_invalidfileaccess);
}

int
iodev_no_rename_file(gx_io_device * iodev, const char *from, const char *to)
{
    return_error(gs_error_invalidfileaccess);
}

int
iodev_no_file_status(gx_io_device * iodev, const char *fname, struct stat *pstat)
{
    return_error(gs_error_undefinedfilename);
}

file_enum *
iodev_no_enumerate_files(gx_io_device * iodev, const char *pat, uint patlen,
			 gs_memory_t * memory)
{
    return NULL;
}

int
iodev_no_get_params(gx_io_device * iodev, gs_param_list * plist)
{
    return 0;
}

int
iodev_no_put_params(gx_io_device * iodev, gs_param_list * plist)
{
    return param_commit(plist);
}

/* ------ %os% ------ */

/* The fopen routine is exported for %null. */
int
iodev_os_fopen(gx_io_device * iodev, const char *fname, const char *access,
	       FILE ** pfile, char *rfname, uint rnamelen)
{
    errno = 0;
    *pfile = gp_fopen(fname, access);
    if (*pfile == NULL)
	return_error(gs_fopen_errno_to_code(errno));
    if (rfname != NULL && rfname != fname)
	strcpy(rfname, fname);
    return 0;
}

/* The fclose routine is exported for %null. */
int
iodev_os_fclose(gx_io_device * iodev, FILE * file)
{
    fclose(file);
    return 0;
}

private int
os_delete(gx_io_device * iodev, const char *fname)
{
    return (unlink(fname) == 0 ? 0 : gs_error_ioerror);
}

private int
os_rename(gx_io_device * iodev, const char *from, const char *to)
{
    return (rename(from, to) == 0 ? 0 : gs_error_ioerror);
}

private int
os_status(gx_io_device * iodev, const char *fname, struct stat *pstat)
{				/* The RS/6000 prototype for stat doesn't include const, */
    /* so we have to explicitly remove the const modifier. */
    return (stat((char *)fname, pstat) < 0 ? gs_error_undefinedfilename : 0);
}

private file_enum *
os_enumerate(gx_io_device * iodev, const char *pat, uint patlen,
	     gs_memory_t * mem)
{
    return gp_enumerate_files_init(pat, patlen, mem);
}

private int
os_get_params(gx_io_device * iodev, gs_param_list * plist)
{
    int code;
    int i0 = 0, i2 = 2;
    bool btrue = true, bfalse = false;
    int BlockSize;
    long Free, LogicalSize;

    /*
     * Return fake values for BlockSize and Free, since we can't get the
     * correct values in a platform-independent manner.
     */
    BlockSize = 1024;
    LogicalSize = 2000000000 / BlockSize;	/* about 2 Gb */
    Free = LogicalSize * 3 / 4;			/* about 1.5 Gb */

    if (
	(code = param_write_bool(plist, "HasNames", &btrue)) < 0 ||
	(code = param_write_int(plist, "BlockSize", &BlockSize)) < 0 ||
	(code = param_write_long(plist, "Free", &Free)) < 0 ||
	(code = param_write_int(plist, "InitializeAction", &i0)) < 0 ||
	(code = param_write_bool(plist, "Mounted", &btrue)) < 0 ||
	(code = param_write_bool(plist, "Removable", &bfalse)) < 0 ||
	(code = param_write_bool(plist, "Searchable", &btrue)) < 0 ||
	(code = param_write_int(plist, "SearchOrder", &i2)) < 0 ||
	(code = param_write_bool(plist, "Writeable", &btrue)) < 0 ||
	(code = param_write_long(plist, "LogicalSize", &LogicalSize)) < 0
	)
	return code;
    return 0;
}

/* ------ Utilities ------ */

/* Get the N'th IODevice from the known device table. */
gx_io_device *
gs_getiodevice(int index)
{
    if (index < 0 || index >= gx_io_device_table_count)
	return 0;		/* index out of range */
    return io_device_table[index];
}

/* Look up an IODevice name. */
/* The name may be either %device or %device%. */
gx_io_device *
gs_findiodevice(const byte * str, uint len)
{
    int i;

    if (len > 1 && str[len - 1] == '%')
	len--;
    for (i = 0; i < gx_io_device_table_count; ++i) {
	gx_io_device *iodev = io_device_table[i];
	const char *dname = iodev->dname;

	if (dname && strlen(dname) == len + 1 && !memcmp(str, dname, len))
	    return iodev;
    }
    return 0;
}

/* ------ Accessors ------ */

/* Get IODevice parameters. */
int
gs_getdevparams(gx_io_device * iodev, gs_param_list * plist)
{				/* All IODevices have the Type parameter. */
    gs_param_string ts;
    int code;

    param_string_from_string(ts, iodev->dtype);
    code = param_write_name(plist, "Type", &ts);
    if (code < 0)
	return code;
    return (*iodev->procs.get_params) (iodev, plist);
}

/* Put IODevice parameters. */
int
gs_putdevparams(gx_io_device * iodev, gs_param_list * plist)
{
    return (*iodev->procs.put_params) (iodev, plist);
}

/* Convert an OS error number to a PostScript error */
/* if opening a file fails. */
int
gs_fopen_errno_to_code(int eno)
{				/* Different OSs vary widely in their error codes. */
    /* We try to cover as many variations as we know about. */
    switch (eno) {
#ifdef ENOENT
	case ENOENT:
	    return_error(gs_error_undefinedfilename);
#endif
#ifdef ENOFILE
#  ifndef ENOENT
#    define ENOENT ENOFILE
#  endif
#  if ENOFILE != ENOENT
	case ENOFILE:
	    return_error(gs_error_undefinedfilename);
#  endif
#endif
#ifdef ENAMETOOLONG
	case ENAMETOOLONG:
	    return_error(gs_error_undefinedfilename);
#endif
#ifdef EACCES
	case EACCES:
	    return_error(gs_error_invalidfileaccess);
#endif
#ifdef EMFILE
	case EMFILE:
	    return_error(gs_error_limitcheck);
#endif
#ifdef ENFILE
	case ENFILE:
	    return_error(gs_error_limitcheck);
#endif
	default:
	    return_error(gs_error_ioerror);
    }
}
