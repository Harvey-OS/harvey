/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gconf.c,v 1.6 2002/08/22 07:12:28 henrys Exp $ */
/* Configuration tables */
#include "memory_.h"
#include "gx.h"
#include "gscdefs.h"		/* interface */
#include "gconf.h"		/* for #defines */
#include "gxdevice.h"
#include "gxdhtres.h"
#include "gxiclass.h"
#include "gxiodev.h"
#include "gxiparam.h"
#include "gxcomp.h"

/*
 * The makefile generates the file gconfig.h, which consists of
 * lines of the form
 *      device_(gs_xxx_device)
 * or
 *      device2_(gs_xxx_device)
 * for each installed device;
 *      emulator_("emulator", strlen("emulator"))
 * for each known emulator;
 *	function_type_(xxx, gs_function_type_xxx)
 * for each known function type;
 *	halftone_(gs_dht_xxx)
 * for each known (device) halftone;
 *	image_class_(gs_image_class_xxx)
 * for each known image class;
 *	image_type_(xxx, gs_image_type_xxx)
 * for each known image type;
 *      init_(gs_xxx_init)
 * for each initialization procedure;
 *      io_device_(gs_iodev_xxx)
 * for each known IODevice;
 *      oper_(xxx_op_defs)
 * for each operator option;
 *      psfile_("gs_xxxx.ps", strlen("gs_xxxx.ps"))
 * for each optional initialization file.
 *      plug_(gs_xxx_init)
 * for each plugin;
 *
 * We include this file multiple times to generate various different
 * source structures.  (It's a hack, but we haven't come up with anything
 * more satisfactory.)
 */

/* ---------------- Resources (devices, inits, IODevices) ---------------- */

/* Declare devices, image types, init procedures, and IODevices as extern. */
#define compositor_(comp_type) extern gs_composite_type_t comp_type;
#define device_(dev) extern gx_device dev;
#define device2_(dev) extern const gx_device dev;
#define halftone_(dht) extern DEVICE_HALFTONE_RESOURCE_PROC(dht);
#define image_class_(cls) extern iclass_proc(cls);
#define image_type_(i,type) extern const gx_image_type_t type;
#define init_(proc) extern init_proc(proc);
#define io_device_(iodev) extern const gx_io_device iodev;
#include "gconf.h"
#undef io_device_
#undef init_
#undef image_type_
#undef image_class_
#undef halftone_
#undef device2_
#undef device_
#undef compositor_

/* Set up compositor type table. */
#define compositor_(comp_type) &comp_type,
private const gs_composite_type_t *const gx_compositor_list[] = {
#include "gconf.h"
    0
};
#undef compositor_

/* Set up the device table. */
#define device_(dev) (const gx_device *)&dev,
#define device2_(dev) &dev,
private const gx_device *const gx_device_list[] = {
#include "gconf.h"
	 0
};
#undef device2_
#undef device_

/* Set up the (device) halftone table. */
extern_gx_device_halftone_list();
#define halftone_(dht) dht,
const gx_dht_proc gx_device_halftone_list[] = {
#include "gconf.h"
    0
};
#undef halftone_

/* Set up the image class table. */
extern_gx_image_class_table();
#define image_class_(cls) cls,
const gx_image_class_t gx_image_class_table[] = {
#include "gconf.h"
    0
};
#undef image_class_
/* We must use unsigned here, not uint.  See gscdefs.h. */
const unsigned gx_image_class_table_count = countof(gx_image_class_table) - 1;

/* Set up the image type table. */
extern_gx_image_type_table();
#define image_type_(i,type) &type,
const gx_image_type_t *const gx_image_type_table[] = {
#include "gconf.h"
    0
};
#undef image_type_
/* We must use unsigned here, not uint.  See gscdefs.h. */
const unsigned gx_image_type_table_count = countof(gx_image_type_table) - 1;

/* Set up the initialization procedure table. */
extern_gx_init_table();
#define init_(proc) proc,
const gx_init_proc gx_init_table[] = {
#include "gconf.h"
    0
};
#undef init_

/* Set up the IODevice table.  The first entry must be %os%, */
/* since it is the default for files with no explicit device specified. */
extern_gx_io_device_table();
extern gx_io_device gs_iodev_os;
#define io_device_(iodev) &iodev,
const gx_io_device *const gx_io_device_table[] = {
    &gs_iodev_os,
#include "gconf.h"
    0
};
#undef io_device_
/* We must use unsigned here, not uint.  See gscdefs.h. */
const unsigned gx_io_device_table_count = countof(gx_io_device_table) - 1;

/* Find a compositor by name. */
extern_gs_find_compositor();
const gs_composite_type_t *
gs_find_compositor(int comp_id)
{
    const gs_composite_type_t *const * ppcomp = gx_compositor_list;
    const gs_composite_type_t *  pcomp;

    while ((pcomp = *ppcomp++) != 0 && pcomp->comp_id != comp_id)
        ;
    return pcomp;
}

/* Return the list of device prototypes, a NULL list of their structure */
/* descriptors (no longer used), and (as the value) the length of the lists. */
extern_gs_lib_device_list();
int
gs_lib_device_list(const gx_device * const **plist,
		   gs_memory_struct_type_t ** pst)
{
    if (plist != 0)
	*plist = gx_device_list;
    if (pst != 0)
	*pst = NULL;
    return countof(gx_device_list) - 1;
}
