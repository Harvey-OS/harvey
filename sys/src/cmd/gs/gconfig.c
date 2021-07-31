/* Copyright (C) 1989, 1992, 1993, 1994, 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* gconfig.c */
/* Configuration tables */
#include "gx.h"
#include "gscdefs.h"		/* interface */
/*
 * Since we only declare variables of type gx_device *,
 * it should be sufficient to define struct gx_device_s as
 * an abstract (undefined) structure.  However, the VAX VMS compiler
 * isn't happy with this, so we have to include the full definition.
 */
#include "gxdevice.h"
#include "gxiodev.h"

/*
 * The makefile generates the file gconfig.h, which consists of
 * lines of the form
 *	device__(gs_xxx_device)
 * for each installed device;
 *	io_device__(gs_iodev_xxx)
 * for each known IODevice;
 *	oper__(xxx_op_defs)
 * for each operator option;
 *	psfile__("gs_xxxx.ps")
 * for each optional initialization file.
 *
 * We include this file multiple times to generate various different
 * source structures.  (It's a hack, but we haven't come up with anything
 * more satisfactory.)
 */

/* Declare the devices as extern. */
#define device_(dev) extern far_data gx_device dev;
#define io_device_(iodev) extern gx_io_device iodev;
#include "gconfig.h"
#undef io_device_
#undef device_

/* Set up the device table. */
#define device_(dev) &dev,
gx_device *gx_device_list[] = {
#include "gconfig.h"
	0
};
#undef device_
uint gx_device_list_count = countof(gx_device_list) - 1;

/* Set up the IODevice table.  The first entry must be %os%, */
/* since it is the default for files with no explicit device specified. */
extern gx_io_device gs_iodev_os;
#define io_device_(iodev) &iodev,
gx_io_device *gx_io_device_table[] = {
	&gs_iodev_os,
#include "gconfig.h"
	0
};
#undef io_device_
uint gx_io_device_table_count = countof(gx_io_device_table) - 1;

/* Here is where the library search path, the name of the */
/* initialization file, and the doc directory are defined. */
const char *gs_doc_directory = GS_DOCDIR;
const char *gs_lib_default_path = GS_LIB_DEFAULT;
const char *gs_init_file = GS_INIT;

/* Define various parameters of this interpreter. */
/* All of these can be set in the makefile. */
/* They should all be const; see gscdefs.h for more information. */
#ifndef GS_BUILDTIME
#  define GS_BUILDTIME\
	0		/****** HOW TO SET THIS? ******/
#endif
long gs_buildtime = GS_BUILDTIME;
#ifndef GS_COPYRIGHT
#  define GS_COPYRIGHT\
	"Copyright (C) 1995 Aladdin Enterprises, Menlo Park, CA.  All rights reserved."
#endif
const char *gs_copyright = GS_COPYRIGHT;	
#ifndef GS_PRODUCT
#  define GS_PRODUCT\
	"Aladdin Ghostscript"
#endif
const char *gs_product = GS_PRODUCT;
#ifndef GS_REVISION
#  define GS_REVISION\
	323		/* primary release # x 100 + */\
			/* secondary release # x 10 + minor release #. */
#endif
long gs_revision = GS_REVISION;		/* should be const, see gscdefs.h */
#ifndef GS_REVISIONDATE
#  define GS_REVISIONDATE\
	19950105	/* year x 10000 + month x 100 + day. */
#endif
long gs_revisiondate = GS_REVISIONDATE;	/* should be const, see gscdefs.h */
#ifndef GS_SERIALNUMBER
#  define GS_SERIALNUMBER\
	42		/* a famous number */
#endif
long gs_serialnumber = GS_SERIALNUMBER;	/* should be const, see gscdefs.h */

/* Some C compilers insist on executable code here, so.... */
void
gconfig_dummy(void)
{
}
