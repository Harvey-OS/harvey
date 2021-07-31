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

/* gsiodev.c */
/* IODevice implementation for Ghostscript */
#include "errno_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gp.h"
#include "gsparam.h"
#include "gxiodev.h"

/* Import the IODevice table from gconfig.c. */
extern gx_io_device *gx_io_device_table[];
extern uint gx_io_device_table_count;

/* Define the OS (%os%) device. */
iodev_proc_fopen(iodev_os_fopen);
iodev_proc_fclose(iodev_os_fclose);
private iodev_proc_delete_file(os_delete);
private iodev_proc_rename_file(os_rename);
private iodev_proc_file_status(os_status);
private iodev_proc_enumerate_files(os_enumerate);
private iodev_proc_get_params(os_get_params);
gx_io_device gs_iodev_os = {
	"%os%", "FileSystem",
	{ iodev_no_init, iodev_no_open_device,
	  NULL /*iodev_os_open_file*/, iodev_os_fopen, iodev_os_fclose,
	  os_delete, os_rename, os_status,
	  os_enumerate, gp_enumerate_files_next, gp_enumerate_files_close,
	  os_get_params, iodev_no_put_params
	}
};

/* ------ Initialization ------ */

void
gs_iodev_init(gs_memory_t *mem)
{	/* Run the one-time initialization of each IODevice. */
	gx_io_device **piodev = &gx_io_device_table[0];
	for ( ; *piodev; piodev++ )
		((*piodev)->procs.init)(*piodev, mem);
}

/* ------ Default (unimplemented) IODevice procedures ------ */

int
iodev_no_init(gx_io_device *iodev, gs_memory_t *mem)
{	return 0;
}

int
iodev_no_open_device(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	return_error(gs_error_invalidfileaccess);
}

int
iodev_no_open_file(gx_io_device *iodev, const char *fname, uint namelen,
  const char *access, stream **ps, gs_memory_t *mem)
{	return_error(gs_error_invalidfileaccess);
}

int
iodev_no_fopen(gx_io_device *iodev, const char *fname, const char *access,
  FILE **pfile, char *rfname, uint rnamelen)
{	return_error(gs_error_invalidfileaccess);
}

int
iodev_no_fclose(gx_io_device *iodev, FILE *file)
{	return_error(gs_error_ioerror);
}

int
iodev_no_delete_file(gx_io_device *iodev, const char *fname)
{	return_error(gs_error_invalidfileaccess);
}

int
iodev_no_rename_file(gx_io_device *iodev, const char *from, const char *to)
{	return_error(gs_error_invalidfileaccess);
}

int
iodev_no_file_status(gx_io_device *iodev, const char *fname, struct stat *pstat)
{	return_error(gs_error_undefinedfilename);
}

file_enum *
iodev_no_enumerate_files(gx_io_device *iodev, const char *pat, uint patlen,
  gs_memory_t *memory)
{	return NULL;
}

int
iodev_no_get_params(gx_io_device *iodev, gs_param_list *plist)
{	return 0;
}

int
iodev_no_put_params(gx_io_device *iodev, gs_param_list *plist)
{	return 0;
}

/* ------ %os% ------ */

/* The fopen routine is exported for %null. */
int
iodev_os_fopen(gx_io_device *iodev, const char *fname, const char *access,
  FILE **pfile, char *rfname, uint rnamelen)
{	errno = 0;
	*pfile = fopen(fname, access);
	if ( *pfile == NULL )
	  return_error(gs_fopen_errno_to_code(errno));
	if ( rfname != NULL )
	  strcpy(rfname, fname);
	return 0;
}

/* The fclose routine is exported for %null. */
int
iodev_os_fclose(gx_io_device *iodev, FILE *file)
{	fclose(file);
	return 0;
}

private int
os_delete(gx_io_device *iodev, const char *fname)
{	return (unlink(fname) == 0 ? 0 : gs_error_ioerror);
}

private int
os_rename(gx_io_device *iodev, const char *from, const char *to)
{	return (rename(from, to) == 0 ? 0 : gs_error_ioerror);
}

private int
os_status(gx_io_device *iodev, const char *fname, struct stat *pstat)
{	/* The RS/6000 prototype for stat doesn't include const, */
	/* so we have to explicitly remove the const modifier. */
	return (stat((char *)fname, pstat) < 0 ? gs_error_undefinedfilename : 0);
}

private file_enum *
os_enumerate(gx_io_device *iodev, const char *pat, uint patlen,
  gs_memory_t *mem)
{	return gp_enumerate_files_init(pat, patlen, mem);
}

private int
os_get_params(gx_io_device *iodev, gs_param_list *plist)
{	/* We aren't going to implement *all* of the Adobe parameters.... */
	int code;
	bool btrue = true;
	gs_param_string tfs;
	param_string_from_string(tfs, "FileSystem");
	if ( (code = param_write_bool(plist, "HasNames", &btrue)) < 0 )
	  return code;
	return param_write_name(plist, "Type", &tfs);
}

/* ------ Utilities ------ */

/* Get the N'th IODevice from the known device table. */
gx_io_device *
gs_getiodevice(int index)
{	if ( index < 0 || index >= gx_io_device_table_count )
		return 0;		/* index out of range */
	return gx_io_device_table[index];
}

/* Look up an IODevice name. */
/* The name may be either %device or %device%. */
gx_io_device *
gs_findiodevice(const byte *str, uint len)
{	gx_io_device **pftab;
	if ( len > 1 && str[len - 1] == '%' )
	  len--;
	for ( pftab = gx_io_device_table; *pftab != NULL; pftab++ )
	  { const char *dname = (*pftab)->dname;
	    if ( strlen(dname) == len + 1 && !memcmp(str, dname, len) )
	      return *pftab;
	  }
	return 0;
}

/* Convert an OS error number to a PostScript error */
/* if opening a file fails. */
int
gs_fopen_errno_to_code(int eno)
{	/* Different OSs vary widely in their error codes. */
	/* We try to cover as many variations as we know about. */
	switch ( eno )
	  {
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
