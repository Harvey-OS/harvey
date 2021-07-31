/* Copyright (C) 1993, 1994, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxiodev.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Structure and default implementation of IODvices */
/* Requires gsmemory.h */

#ifndef gxiodev_INCLUDED
#  define gxiodev_INCLUDED

#include "stat_.h"

/*
 * Note that IODevices are not the same as Ghostscript output devices.
 * See section 3.8.2 of the PostScript Language Reference Manual,
 * Second Edition, for more information.
 */

#ifndef gx_io_device_DEFINED
#  define gx_io_device_DEFINED
typedef struct gx_io_device_s gx_io_device;
#endif
typedef struct gx_io_device_procs_s gx_io_device_procs;  /* defined here */

/* The IODevice table is defined in gconf.c; its extern is in gscdefs.h. */

#ifndef file_enum_DEFINED	/* also defined in gp.h */
#  define file_enum_DEFINED
struct file_enum_s;		/* opaque to client, defined by implementors */
typedef struct file_enum_s file_enum;
#endif

/* Define an opaque type for parameter lists. */
#ifndef gs_param_list_DEFINED
#  define gs_param_list_DEFINED
typedef struct gs_param_list_s gs_param_list;
#endif

/* Define an opaque type for streams. */
#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif

/* Definition of IODevice procedures */
/* Note that file names for fopen, delete, rename, and status */
/* are C strings, not pointer + length. */
/* Note also that "streams" are a higher-level concept; */
/* the open_device and open_file procedures are normally NULL. */

struct gx_io_device_procs_s {

#define iodev_proc_init(proc)\
  int proc(P2(gx_io_device *iodev, gs_memory_t *mem))
    iodev_proc_init((*init));	/* one-time initialization */

#define iodev_proc_open_device(proc)\
  int proc(P4(gx_io_device *iodev, const char *access, stream **ps,\
	      gs_memory_t *mem))
    iodev_proc_open_device((*open_device));

#define iodev_proc_open_file(proc)\
  int proc(P6(gx_io_device *iodev, const char *fname, uint namelen,\
	      const char *access, stream **ps, gs_memory_t *mem))
    iodev_proc_open_file((*open_file));

    /* fopen was changed in release 2.9.6, */
    /* and again in 3.20 to return the real fname separately */

#define iodev_proc_fopen(proc)\
  int proc(P6(gx_io_device *iodev, const char *fname, const char *access,\
	      FILE **pfile, char *rfname, uint rnamelen))
    iodev_proc_fopen((*fopen));

#define iodev_proc_fclose(proc)\
  int proc(P2(gx_io_device *iodev, FILE *file))
    iodev_proc_fclose((*fclose));

#define iodev_proc_delete_file(proc)\
  int proc(P2(gx_io_device *iodev, const char *fname))
    iodev_proc_delete_file((*delete_file));

#define iodev_proc_rename_file(proc)\
  int proc(P3(gx_io_device *iodev, const char *from, const char *to))
    iodev_proc_rename_file((*rename_file));

#define iodev_proc_file_status(proc)\
  int proc(P3(gx_io_device *iodev, const char *fname, struct stat *pstat))
    iodev_proc_file_status((*file_status));

#define iodev_proc_enumerate_files(proc)\
  file_enum *proc(P4(gx_io_device *iodev, const char *pat, uint patlen,\
		     gs_memory_t *mem))
    iodev_proc_enumerate_files((*enumerate_files));

#define iodev_proc_enumerate_next(proc)\
  uint proc(P3(file_enum *pfen, char *ptr, uint maxlen))
    iodev_proc_enumerate_next((*enumerate_next));

#define iodev_proc_enumerate_close(proc)\
  void proc(P1(file_enum *pfen))
    iodev_proc_enumerate_close((*enumerate_close));

    /* Added in release 2.9 */

#define iodev_proc_get_params(proc)\
  int proc(P2(gx_io_device *iodev, gs_param_list *plist))
    iodev_proc_get_params((*get_params));

#define iodev_proc_put_params(proc)\
  int proc(P2(gx_io_device *iodev, gs_param_list *plist))
    iodev_proc_put_params((*put_params));

};

/* The following typedef is needed because ansi2knr can't handle */
/* iodev_proc_fopen((*procname)) in a formal argument list. */
typedef iodev_proc_fopen((*iodev_proc_fopen_t));

/* Default implementations of procedures */
iodev_proc_init(iodev_no_init);
iodev_proc_open_device(iodev_no_open_device);
iodev_proc_open_file(iodev_no_open_file);
iodev_proc_fopen(iodev_no_fopen);
iodev_proc_fclose(iodev_no_fclose);
iodev_proc_delete_file(iodev_no_delete_file);
iodev_proc_rename_file(iodev_no_rename_file);
iodev_proc_file_status(iodev_no_file_status);
iodev_proc_enumerate_files(iodev_no_enumerate_files);
iodev_proc_get_params(iodev_no_get_params);
iodev_proc_put_params(iodev_no_put_params);
/* The %os% implemention of fopen and fclose. */
/* These are exported for pipes and for %null. */
iodev_proc_fopen(iodev_os_fopen);
iodev_proc_fclose(iodev_os_fclose);

/* Get the N'th IODevice. */
gx_io_device *gs_getiodevice(P1(int));

#define iodev_default (gs_getiodevice(0))

/* Look up an IODevice name. */
gx_io_device *gs_findiodevice(P2(const byte *, uint));

/* Get and put IODevice parameters. */
int gs_getdevparams(P2(gx_io_device *, gs_param_list *));
int gs_putdevparams(P2(gx_io_device *, gs_param_list *));

/* Convert an OS error number to a PostScript error */
/* if opening a file fails. */
int gs_fopen_errno_to_code(P1(int));

/* Test whether a string is equal to a character. */
/* (This is used for access testing in file_open procedures.) */
#define streq1(str, chr)\
  ((str)[0] == (chr) && (str)[1] == 0)

/* Finally, the IODevice structure itself. */
struct gx_io_device_s {
    const char *dname;		/* the IODevice name */
    const char *dtype;		/* the type returned by currentdevparams */
    gx_io_device_procs procs;
    void *state;		/* (if the IODevice has state) */
};

#define private_st_io_device()	/* in gsiodev.c */\
  gs_private_st_ptrs1(st_io_device, gx_io_device, "gx_io_device",\
    io_device_enum_ptrs, io_device_reloc_ptrs, state)

#endif /* gxiodev_INCLUDED */
