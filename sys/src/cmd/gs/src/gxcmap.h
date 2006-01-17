/* Copyright (C) 1989, 1995, 1996, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxcmap.h,v 1.12 2004/10/01 23:35:02 ghostgum Exp $ */
/* Color mapping procedures */
/* Requires gxdcolor.h. */

#ifndef gxcmap_INCLUDED
#  define gxcmap_INCLUDED

#include "gscsel.h"
#include "gxfmap.h"

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

/* Procedures for rendering colors specified by fractions. */

#define cmap_proc_gray(proc)\
  void proc(frac, gx_device_color *, const gs_imager_state *,\
	    gx_device *, gs_color_select_t)
#define cmap_proc_rgb(proc)\
  void proc(frac, frac, frac, gx_device_color *, const gs_imager_state *,\
	    gx_device *, gs_color_select_t)
#define cmap_proc_cmyk(proc)\
  void proc(frac, frac, frac, frac, gx_device_color *,\
	    const gs_imager_state *, gx_device *, gs_color_select_t)
#define cmap_proc_rgb_alpha(proc)\
  void proc(frac, frac, frac, frac, gx_device_color *,\
	       const gs_imager_state *, gx_device *, gs_color_select_t)
#define cmap_proc_separation(proc)\
  void proc(frac, gx_device_color *, const gs_imager_state *,\
	       gx_device *, gs_color_select_t)
#define cmap_proc_devicen(proc)\
  void proc(const frac *, gx_device_color *, const gs_imager_state *, \
	       gx_device *, gs_color_select_t)
#define cmap_proc_is_halftoned(proc)\
  bool proc(const gs_imager_state *, gx_device *)

/*
 * List of mapping functions from the standard color spaces to the
 * device color model. Any unused component will be mapped to 0.
 */
#define cm_map_proc_gray(proc) \
    void proc (gx_device * dev, frac gray, \
              frac * out)

#define cm_map_proc_rgb(proc) \
    void proc (gx_device * dev, \
	      const gs_imager_state *pis, \
              frac r, frac g, frac b, \
              frac * out)

#define cm_map_proc_cmyk(proc) \
    void proc (gx_device * dev, \
              frac c, frac m, frac y, frac k, \
              frac * out)

/*
 * The following procedures come from the device.  It they are
 * specified then  they are used to convert from the given
 * color space to the device's color model.  Otherwise the
 * standard conversions are used.  The procedures must be defined
 * for a DeviceN color model
 *
 * Because of a bug in the Watcom C compiler, we have to split the
 * struct from the typedef.
 */
struct gx_cm_color_map_procs_s {
    cm_map_proc_gray((*map_gray));
    cm_map_proc_rgb((*map_rgb));
    cm_map_proc_cmyk((*map_cmyk));
};

typedef struct gx_cm_color_map_procs_s  gx_cm_color_map_procs;

/*
 * Make some routine global for use in the forwarding device.
 */
cm_map_proc_gray(gray_cs_to_gray_cm);
cm_map_proc_rgb(rgb_cs_to_rgb_cm);
cm_map_proc_cmyk(cmyk_cs_to_cmyk_cm);

/*
 * Color mapping may now be device specific, so the color space
 * to color model mapping is separated from other maps, such as
 * applying the current transfer function or halftone.
 *
 * The routine pointed to by get_cmap_procs (a field in the image
 * state; see gxistate.h) should initialize the cm_color_map_procs
 * pointer, using the get_color_mapping_procs method of the device.
 *
 * Because of a bug in the Watcom C compiler, we have to split the
 * struct from the typedef.
 */
struct gx_color_map_procs_s {
    cmap_proc_gray((*map_gray));
    cmap_proc_rgb((*map_rgb));
    cmap_proc_cmyk((*map_cmyk));
    cmap_proc_rgb_alpha((*map_rgb_alpha));
    cmap_proc_separation((*map_separation));
    cmap_proc_devicen((*map_devicen));
    cmap_proc_is_halftoned((*is_halftoned));
};
typedef struct gx_color_map_procs_s gx_color_map_procs;

/*
 * Determine the color mapping procedures for a device.  Even though this
 * does not currently use information from the imager state, it must be
 * a virtual procedure of the state for internal reasons.
 */
const gx_color_map_procs *
    gx_get_cmap_procs(const gs_imager_state *, const gx_device *);
const gx_color_map_procs *
    gx_default_get_cmap_procs(const gs_imager_state *, const gx_device *);

/*
 * Set the color mapping procedures in the graphics state.  This is
 * currently only needed when switching devices, but might be used more
 * often in the future.
 */
void gx_set_cmap_procs(gs_imager_state *, const gx_device *);

/* Remap a concrete (frac) gray, RGB or CMYK color. */
/* These cannot fail, and do not return a value. */
#define gx_remap_concrete_gray(cgray, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_gray)(cgray, pdc, pis, dev, select)
#define gx_remap_concrete_rgb(cr, cg, cb, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_rgb)(cr, cg, cb, pdc, pis, dev, select)
#define gx_remap_concrete_cmyk(cc, cm, cy, ck, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_cmyk)(cc, cm, cy, ck, pdc, pis, dev, select)
#define gx_remap_concrete_rgb_alpha(cr, cg, cb, ca, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_rgb_alpha)(cr, cg, cb, ca, pdc, pis, dev, select)
#define gx_remap_concrete_separation(pcc, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_separation)(pcc, pdc, pis, dev, select)
#define gx_remap_concrete_devicen(pcc, pdc, pis, dev, select)\
  ((pis)->cmap_procs->map_devicen)(pcc, pdc, pis, dev, select)

/* Map a color */
#include "gxcindex.h"
#include "gxcvalue.h"

/*
 * These are the default routines for converting a color space into
 * a list of device colorants.
 */
extern cm_map_proc_gray(gx_default_gray_cs_to_gray_cm);
extern cm_map_proc_rgb(gx_default_rgb_cs_to_gray_cm);
extern cm_map_proc_cmyk(gx_default_cmyk_cs_to_gray_cm);

extern cm_map_proc_gray(gx_default_gray_cs_to_rgb_cm);
extern cm_map_proc_rgb(gx_default_rgb_cs_to_rgb_cm);
extern cm_map_proc_cmyk(gx_default_cmyk_cs_to_rgb_cm);

extern cm_map_proc_gray(gx_default_gray_cs_to_cmyk_cm);
extern cm_map_proc_rgb(gx_default_rgb_cs_to_cmyk_cm);
extern cm_map_proc_cmyk(gx_default_cmyk_cs_to_cmyk_cm);

extern cm_map_proc_gray(gx_default_gray_cs_to_cmyk_cm);
extern cm_map_proc_rgb(gx_default_rgb_cs_to_cmyk_cm);
extern cm_map_proc_cmyk(gx_default_cmyk_cs_to_cmyk_cm);

extern cm_map_proc_gray(gx_error_gray_cs_to_cmyk_cm);
extern cm_map_proc_rgb(gx_error_rgb_cs_to_cmyk_cm);
extern cm_map_proc_cmyk(gx_error_cmyk_cs_to_cmyk_cm);


/*
  Get the mapping procedures appropriate for the currently set
  color model.
 */
#define dev_t_proc_get_color_mapping_procs(proc, dev_t) \
    const gx_cm_color_map_procs * (proc)(const dev_t * dev)

#define dev_proc_get_color_mapping_procs(proc) \
    dev_t_proc_get_color_mapping_procs(proc, gx_device)

/*
  Define the options for the component_type parameter to get_color_comp_index
  routines.  Note:  This information is currently being used by the routines
  for identifying when they are being given a separation name.  Some devices
  automaticaly add separations to the device's components if the separation
  is not previously known and there is room in the device.
*/
#define NO_COMP_NAME_TYPE	0
#define SEPARATION_NAME		1

/*
  Convert a color component name into a colorant index.
*/
#define dev_t_proc_get_color_comp_index(proc, dev_t) \
    int (proc)(dev_t * dev, const char * pname, int name_size, int component_type)

#define dev_proc_get_color_comp_index(proc) \
    dev_t_proc_get_color_comp_index(proc, gx_device)

/*
  Map a color into the device's color model.
*/
#define dev_t_proc_encode_color(proc, dev_t) \
    gx_color_index (proc)(dev_t * dev, const gx_color_value colors[])

#define dev_proc_encode_color(proc) \
    dev_t_proc_encode_color(proc, gx_device)

/*
  Map a color index from the device's current color model into a list of
  colorant values.
*/
#define dev_t_proc_decode_color(proc, dev_t) \
    int (proc)(dev_t * dev, gx_color_index cindex, gx_color_value colors[])

#define dev_proc_decode_color(proc) \
    dev_t_proc_decode_color(proc, gx_device)



/*
 * These are the default routines for translating a color component
 * name into the device colorant index.
 */
dev_proc_get_color_comp_index(gx_error_get_color_comp_index);
dev_proc_get_color_comp_index(gx_default_DevGray_get_color_comp_index);
dev_proc_get_color_comp_index(gx_default_DevRGB_get_color_comp_index);
dev_proc_get_color_comp_index(gx_default_DevCMYK_get_color_comp_index);
dev_proc_get_color_comp_index(gx_default_DevRGBK_get_color_comp_index);

/*
 * These are the default routines for getting the color space conversion
 * routines.
 */
dev_proc_get_color_mapping_procs(gx_error_get_color_mapping_procs);
dev_proc_get_color_mapping_procs(gx_default_DevGray_get_color_mapping_procs);
dev_proc_get_color_mapping_procs(gx_default_DevRGB_get_color_mapping_procs);
dev_proc_get_color_mapping_procs(gx_default_DevCMYK_get_color_mapping_procs);
dev_proc_get_color_mapping_procs(gx_default_DevRGBK_get_color_mapping_procs);

/*
 * These are the default routines for converting a colorant value list
 * into a gx_color_index.
 */
dev_proc_encode_color(gx_error_encode_color);
dev_proc_encode_color(gx_default_encode_color);

/*
 * These are the default routines for converting a colorant value list
 * into a gx_color_index.
 */
dev_proc_encode_color(gx_default_gray_fast_encode);
dev_proc_encode_color(gx_default_gray_encode);

/*
 * This is the default encode_color routine for grayscale devices
 * that provide a map_rgb_color procedure, but don't themselves
 * provide encode_color.
 */
dev_proc_encode_color(gx_backwards_compatible_gray_encode);

/*
 * These are the default routines for converting a gx_color_index into
 * a list of device colorant values
 */
dev_proc_decode_color(gx_error_decode_color);
dev_proc_decode_color(gx_default_decode_color);


#define unit_frac(v, ftemp)\
  (ftemp = (v),\
   (is_fneg(ftemp) ? frac_0 : is_fge1(ftemp) ? frac_1 : float2frac(ftemp)))

#endif /* gxcmap_INCLUDED */
