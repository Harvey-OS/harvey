/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevmrun.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Definition of run-length encoded memory device */

#ifndef gdevmrun_INCLUDED
#  define gdevmrun_INCLUDED

/*
 * This memory device stores full-size pixels with run-length
 * encoding if possible, switching to the standard uncompressed
 * representation if necessary.
 */

#include "gxdevmem.h"

/*
 * Define the device, built on a memory device.
 */
typedef struct gx_device_run_s {
    gx_device_memory md;	/* must be first */
    uint runs_per_line;
    int umin, umax1;		/* some range of uninitialized lines */
    int smin, smax1;		/* some range in standard (not run) form */
    /*
     * Save memory device procedures that we replace with run-oriented
     * ones, for use with the uncompressed representation.
     */
    struct sp_ {
	dev_proc_copy_mono((*copy_mono));
	dev_proc_copy_color((*copy_color));
	dev_proc_fill_rectangle((*fill_rectangle));
	dev_proc_copy_alpha((*copy_alpha));
	dev_proc_strip_tile_rectangle((*strip_tile_rectangle));
	dev_proc_strip_copy_rop((*strip_copy_rop));
	dev_proc_get_bits_rectangle((*get_bits_rectangle));
    } save_procs;
} gx_device_run;

/*
 * Convert a memory device to run-length form.  The mdev argument should be
 * const, but it isn't because we need to call gx_device_white.
 */
int gdev_run_from_mem(P2(gx_device_run *rdev, gx_device_memory *mdev));

#endif /* gdevmrun_INCLUDED */
