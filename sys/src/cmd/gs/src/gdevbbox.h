/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevbbox.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Definitions and interface for bbox (bounding box accumulator) device */
/* Requires gxdevice.h */

#ifndef gdevbbox_INCLUDED
#  define gdevbbox_INCLUDED

/*
 * This device keeps track of the per-page bounding box, and also optionally
 * forwards all drawing commands to a target.  It can be used either as a
 * free-standing device or as a component (e.g., by the EPS writer).
 *
 * One way to use a bounding box device is simply to include bbox.dev in the
 * value of DEVICE_DEVSn in the makefile.  This produces a free-standing
 * device named 'bbox' that can be selected in the usual way (-sDEVICE=bbox)
 * and that prints out the bounding box at each showpage or copypage without
 * doing any drawing.
 *
 * The other way to use a bounding box device is from C code as a component
 * in a device pipeline.  To set up a bounding box device that doesn't do
 * any drawing:
 *      gx_device_bbox *bdev =
 *        gs_alloc_struct_immovable(some_memory,
 *                                  gx_device_bbox, &st_device_bbox,
 *                                  "some identifying string for debugging");
 *      gx_device_bbox_init(bdev, NULL);
 * Non-drawing bounding box devices have an "infinite" page size.
 *
 * To set up a bounding box device that draws to another device tdev:
 *      gx_device_bbox *bdev =
 *        gs_alloc_struct_immovable(some_memory,
 *                                  gx_device_bbox, &st_device_bbox,
 *                                  "some identifying string for debugging");
 *      gx_device_bbox_init(bdev, tdev);
 * Bounding box devices that draw to a real device appear to have the
 * same page size as that device.
 *
 * To intercept the end-of-page to call a routine eop of your own, after
 * setting up the device:
 *      dev_proc_output_page(eop);      -- declare a prototype for eop
 *      ...
 *      set_dev_proc(bdev, output_page, eop);
 *      ...
 *      int eop(gx_device *dev, int num_copies, int flush)
 *      {       gs_rect bbox;
 *              gx_device_bbox_bbox((gx_device_bbox *)dev, &bbox);
 *              << do whatever you want >>
 *              return gx_forward_output_page(dev, num_copies, flush);
 *      }
 *
 * Note that bounding box devices, unlike almost all other forwarding
 * devices, conditionally propagate the open_device and close_device
 * calls to their target.  By default, they do propagate these calls:
 * use gx_device_bbox_fwd_open_close to change this if you want.
 */
/*
 * Define virtual procedures for managing the accumulated bounding box.
 * These may be redefined by subclasses, and are also used for compositors.
 */
typedef struct gx_device_bbox_procs_s {

#define dev_bbox_proc_init_box(proc)\
  bool proc(P1(void *proc_data))
    dev_bbox_proc_init_box((*init_box));

#define dev_bbox_proc_get_box(proc)\
  void proc(P2(const void *proc_data, gs_fixed_rect *pbox))
    dev_bbox_proc_get_box((*get_box));

#define dev_bbox_proc_add_rect(proc)\
  void proc(P5(void *proc_data, fixed x0, fixed y0, fixed x1, fixed y1))
    dev_bbox_proc_add_rect((*add_rect));

#define dev_bbox_proc_in_rect(proc)\
  bool proc(P2(const void *proc_data, const gs_fixed_rect *pbox))
    dev_bbox_proc_in_rect((*in_rect));

} gx_device_bbox_procs_t;
/* Default implementations */
dev_bbox_proc_init_box(bbox_default_init_box);
dev_bbox_proc_get_box(bbox_default_get_box);
dev_bbox_proc_add_rect(bbox_default_add_rect);
dev_bbox_proc_in_rect(bbox_default_in_rect);

#define gx_device_bbox_common\
	gx_device_forward_common;\
	bool free_standing;\
	bool forward_open_close;\
	gx_device_bbox_procs_t box_procs;\
	void *box_proc_data;\
	bool white_is_opaque;\
	/* The following are updated dynamically. */\
	gs_fixed_rect bbox;\
	gx_color_index black, white;\
	gx_color_index transparent /* white or gx_no_color_index */
typedef struct gx_device_bbox_s gx_device_bbox;
#define gx_device_bbox_common_initial(fs, foc, wio)\
  0 /* target */,\
  fs, foc, {0}, 0, wio,\
  {{0, 0}, {0, 0}}, gx_no_color_index, gx_no_color_index, gx_no_color_index
struct gx_device_bbox_s {
    gx_device_bbox_common;
};

extern_st(st_device_bbox);
#define public_st_device_bbox()	/* in gdevbbox.c */\
  gs_public_st_suffix_add1_final(st_device_bbox, gx_device_bbox,\
    "gx_device_bbox", device_bbox_enum_ptrs, device_bbox_reloc_ptrs,\
    gx_device_finalize, st_device_forward, box_proc_data)

/* Initialize a bounding box device. */
void gx_device_bbox_init(P2(gx_device_bbox * dev, gx_device * target));

/* Set whether a bounding box device propagates open/close to its target. */
void gx_device_bbox_fwd_open_close(P2(gx_device_bbox * dev,
				      bool forward_open_close));

/* Set whether a bounding box device considers white to be opaque. */
void gx_device_bbox_set_white_opaque(P2(gx_device_bbox *dev,
					bool white_is_opaque));

/* Read back the bounding box in 1/72" units. */
void gx_device_bbox_bbox(P2(gx_device_bbox * dev, gs_rect * pbbox));

/* Release a bounding box device. */
void gx_device_bbox_release(P1(gx_device_bbox *dev));

#endif /* gdevbbox_INCLUDED */
