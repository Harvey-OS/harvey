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

/*$Id: gxdcolor.h,v 1.9 2002/08/29 00:11:30 dan Exp $ */
/* Device color representation for Ghostscript */

#ifndef gxdcolor_INCLUDED
#  define gxdcolor_INCLUDED

#include "gscsel.h"
#include "gsdcolor.h"
#include "gsropt.h"
#include "gsstruct.h"		/* for extern_st, GC procs */

/* Define opaque types. */

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;

#endif

/*
 * Define a source structure for RasterOp.
 */
typedef struct gx_rop_source_s {
    const byte *sdata;
    int sourcex;
    uint sraster;
    gx_bitmap_id id;
    gx_color_index scolors[2];
    bool use_scolors;
} gx_rop_source_t;

/*
 * Note that the following definition depends on the gx_color_index for
 * black, which may not be 0.  Clients must check this and construct
 * a different null source if necessary.
 */
#define gx_rop_no_source_body(black_pixel)\
  NULL, 0, 0, gx_no_bitmap_id, {black_pixel, black_pixel}, true
#define gx_rop_source_set_color(prs, pixel)\
  ((prs)->scolors[0] = (prs)->scolors[1] = (pixel))
void gx_set_rop_no_source(const gx_rop_source_t **psource,
			  gx_rop_source_t *pno_source, gx_device *dev);
#define set_rop_no_source(source, no_source, dev)\
  gx_set_rop_no_source(&(source), &(no_source), dev)

/*
 * Define the device color structure per se.
 */

/* The typedef is in gsdcolor.h. */
/*typedef struct gx_device_color_type_s gx_device_color_type_t; */
struct gx_device_color_type_s {

    /*
     * In order to simplify memory management, we use a union, but since
     * different variants may have different pointer tracing procedures,
     * we have to define a separate GC structure type for each variant.
     */

    gs_memory_type_ptr_t stype;

    /*
     * Accessors.
     *
     * The "save_dc" method fills in a gx_device_color_saved structure
     * for the operand device color. This is may be used with the
     * "write" and "read" methods (see below) to minimize command list
     * size.
     *
     * The "get_dev_halftone" method returns a pointer to the device
     * halftone used by the current color, or NULL if there is no such
     * halftone (i.e.: the device color is a pure color).
     *
     * The "get_phase" returns true if the device color contains phase
     * information, and sets *pphase to the appropriate value. Halftones
     * that do not use the color information return false.
     */
#define dev_color_proc_save_dc(proc)\
  void proc(const gx_device_color * pdevc, gx_device_color_saved * psdc)
			 dev_color_proc_save_dc((*save_dc));

#define dev_color_proc_get_dev_halftone(proc)\
  const gx_device_halftone * proc(const gx_device_color * pdevc)
			 dev_color_proc_get_dev_halftone((*get_dev_halftone));

#define dev_color_proc_get_phase(proc)\
  bool proc(const gx_device_color * pdevc, gs_int_point * pphase)
			dev_color_proc_get_phase((*get_phase));

    /*
     * If necessary and possible, load the halftone or Pattern cache
     * with the rendering of this color.
     */

#define dev_color_proc_load(proc)\
  int proc(gx_device_color *pdevc, const gs_imager_state *pis,\
    gx_device *dev, gs_color_select_t select)
                         dev_color_proc_load((*load));

    /*
     * Fill a rectangle with the color.
     * We pass the device separately so that pattern fills can
     * substitute a tiled mask clipping device.
     */

#define dev_color_proc_fill_rectangle(proc)\
  int proc(const gx_device_color *pdevc, int x, int y, int w, int h,\
    gx_device *dev, gs_logical_operation_t lop, const gx_rop_source_t *source)
                         dev_color_proc_fill_rectangle((*fill_rectangle));

    /*
     * Fill a masked region with a color.  Nearly all device colors
     * use the default implementation, which simply parses the mask
     * into rectangles and calls fill_rectangle.  Note that in this
     * case there is no RasterOp source: the mask is the source.
     */

#define dev_color_proc_fill_masked(proc)\
  int proc(const gx_device_color *pdevc, const byte *data, int data_x,\
    int raster, gx_bitmap_id id, int x, int y, int w, int h,\
    gx_device *dev, gs_logical_operation_t lop, bool invert)
                         dev_color_proc_fill_masked((*fill_masked));

    /*
     * Test whether this color is equal to another.
     */

#define dev_color_proc_equal(proc)\
  bool proc(const gx_device_color *pdevc1, const gx_device_color *pdevc2)
                         dev_color_proc_equal((*equal));

    /*
     * Serialize and deserialize a device color.
     *
     * The "write" routine converts a device color into a string for
     * writing to the command list. *psize is the amount of space
     * available. If the saved color and the current color are the same,
     * the routine sets *psize to 0 and returns 1. Otherwise, if *psize
     * is large enough, the procedure sets *psize to the amount actually
     * used and returns 0. If *psize is too small and no other problem
     * is detected, *psize is set to the amount required and 
     * gs_error_rangecheck is returned. If some other error is detected,
     * *psize is left unchanged and the error code is returned.
     *
     * The "read" routine converts the string representation back into
     * the full device color structure. The value returned is the number
     * of bytes actually read, or < 0 in the event of an error.
     *
     * As with any instance of virtual serialization, the command list
     * code must include its own identifier of the color space type in
     * the command list, so as to know which read routine to call. The
     * procedures gx_dc_get_type_code and gx_dc_get_type_from_code are
     * provided to support this operation.
     *
     * For the write operation, psdc points to the saved version of the
     * color previously stored for a particular band. When the band is
     * rendered this will be the current device color just before the
     * color being serialized is read. This information can be used to
     * make encoding more efficient, and to discard unnecessary color
     * setting operations. To avoid any optimization, set psdc to be a
     * null pointer.
     *
     * Note that the caller is always responsible for serializing and
     * transmitting the device halftone, if this is required. Because
     * device halftones change infrequently, they are transmitted as
     * "all bands" commands. This is only possible if they are serialized
     * separately, which is why they cannot be handled by these methods.
     *
     * The first device color serialized after the halftone has been
     * changed should always contain complete information; i.e.: psdc
     * should be set to a null pointer. This is necessary as the command
     * list reader may have reset its device color when the halftone is
     * changed, so informaition from the prior device color will no
     * longer be available.
     *
     * For the read and method, the imager state is passed as an operand,
     * which allows the routine to access the current device halftone
     * (always required). Also passed in a pointer to the existing device
     * color, as this is not part of the imager state. If the writer was
     * passed a non-null psdc operand, *prior_devc must reflect the
     * information contained in *psdc.
     *
     * NB: For the read method, pdevc and prior_devc may and usually
     *     will be the same. Code implementing this method must be able
     *     to handle this situation.
     *
     * The device is provided as an operand for both routines to pass
     * color model information. This allows more compact encoding of
     * various pieces of information, in particular color indices.
     */
#define dev_color_proc_write(proc)\
  int proc(const gx_device_color *pdevc, const gx_device_color_saved *psdc,\
    const gx_device * dev, byte *data, uint *psize)
			dev_color_proc_write((*write));

#define dev_color_proc_read(proc)\
  int proc(gx_device_color *pdevc, const gs_imager_state * pis,\
    const gx_device_color *prior_devc, const gx_device * dev,\
    const byte *data, uint size, gs_memory_t *mem)
			dev_color_proc_read((*read));

    /*
     * Identify which color model components have non-zero intensities in
     * a device color. If this is the case, set the (1 << i)'th bit of
     * *pcomp_bits to 1; otherwise set it to 0. This method is used to
     * support PDF's overprint mode. The *pcomp_bits value is known to be
     * large enough for the number of device color components, and should
     * be initialized to 0 by the client.
     *
     * Returns 0 except for shading and/or color tiling patterns, for
     * which  1 is returned. For those two "colors", lower level device
     * colors must be examined to determine the desired information. This
     * is not a problem for shading colors, as overprint mode does not
     * apply to them. It is potentially a problem for colored tiling
     * patterns, but the situations in which it is a problem other, long-
     * standing implementation difficulties for patterns would also be a
     * problem.
     *
     * Returns of < 0 indicate an error, and shouldn't be possible.
     */
#define dev_color_proc_get_nonzero_comps(proc)\
  int proc(const gx_device_color * pdevc, const gx_device * dev,\
    gx_color_index * pcomp_bits)
                         dev_color_proc_get_nonzero_comps((*get_nonzero_comps));
};

/* Define the default implementation of fill_masked. */
dev_color_proc_fill_masked(gx_dc_default_fill_masked);

extern_st(st_device_color);
/* public_st_device_color() is defined in gsdcolor.h */

/* Define the standard device color types. */
/* See gsdcolor.h for details. */
extern const gx_device_color_type_t
#define gx_dc_type_none (&gx_dc_type_data_none)
      gx_dc_type_data_none,	/* gxdcolor.c */
#define gx_dc_type_null (&gx_dc_type_data_null)
      gx_dc_type_data_null,	/* gxdcolor.c */
#define gx_dc_type_pure (&gx_dc_type_data_pure)
      gx_dc_type_data_pure,	/* gxdcolor.c */
/*#define gx_dc_type_pattern (&gx_dc_type_data_pattern) */
						/*gx_dc_type_data_pattern, *//* gspcolor.c */
#define gx_dc_type_ht_binary (&gx_dc_type_data_ht_binary)
      gx_dc_type_data_ht_binary,	/* gxht.c */
#define gx_dc_type_ht_colored (&gx_dc_type_data_ht_colored)
      gx_dc_type_data_ht_colored,	/* gxcht.c */
#define gx_dc_type_wts (&gx_dc_type_data_wts)
      gx_dc_type_data_wts;	/* gxwts.c */

/* the following are exported for the benefit of gsptype1.c */
extern  dev_color_proc_get_nonzero_comps(gx_dc_pure_get_nonzero_comps);
extern  dev_color_proc_get_nonzero_comps(gx_dc_ht_binary_get_nonzero_comps);
extern  dev_color_proc_get_nonzero_comps(gx_dc_ht_colored_get_nonzero_comps);

/* convert between color types and color type indices */
extern int gx_get_dc_type_index(const gx_device_color *);
extern const gx_device_color_type_t * gx_get_dc_type_from_index(int);

/* the two canonical "get_phase" methods */
extern  dev_color_proc_get_phase(gx_dc_no_get_phase);
extern  dev_color_proc_get_phase(gx_dc_ht_get_phase);


#define gs_color_writes_pure(pgs)\
  color_writes_pure((pgs)->dev_color, (pgs)->log_op)

/* Set up device color 1 for writing into a mask cache */
/* (e.g., the character cache). */
void gx_set_device_color_1(gs_state * pgs);

/* Remap the color if necessary. */
int gx_remap_color(gs_state *);

#define gx_set_dev_color(pgs)\
  if ( !color_is_set((pgs)->dev_color) )\
   { int code_dc = gx_remap_color(pgs);\
     if ( code_dc != 0 ) return code_dc;\
   }

/* Indicate that the device color needs remapping. */
#define gx_unset_dev_color(pgs)\
  color_unset((pgs)->dev_color)

/* Load the halftone cache in preparation for drawing. */
#define gx_color_load_select(pdevc, pis, dev, select)\
  (*(pdevc)->type->load)(pdevc, pis, dev, select)
#define gx_color_load(pdevc, pis, dev)\
  gx_color_load_select(pdevc, pis, dev, gs_color_select_texture)
#define gs_state_color_load(pgs)\
  gx_color_load((pgs)->dev_color, (const gs_imager_state *)(pgs),\
		(pgs)->device)

/* Fill a rectangle with a color. */
#define gx_device_color_fill_rectangle(pdevc, x, y, w, h, dev, lop, source)\
  (*(pdevc)->type->fill_rectangle)(pdevc, x, y, w, h, dev, lop, source)
#define gx_fill_rectangle_device_rop(x, y, w, h, pdevc, dev, lop)\
  gx_device_color_fill_rectangle(pdevc, x, y, w, h, dev, lop, NULL)
#define gx_fill_rectangle_rop(x, y, w, h, pdevc, lop, pgs)\
  gx_fill_rectangle_device_rop(x, y, w, h, pdevc, (pgs)->device, lop)
#define gx_fill_rectangle(x, y, w, h, pdevc, pgs)\
  gx_fill_rectangle_rop(x, y, w, h, pdevc, (pgs)->log_op, pgs)

/*
 * Utilities to write/read color indices. Currently, a very simple mechanism
 * is used, much simpler than that used by other command-list writers. This
 * should be sufficient for most situations.
 *
 * The operand set and return values are those of the device color write/read
 * routines.
 */
extern  int     gx_dc_write_color( gx_color_index       color,
                                   const gx_device *    dev,
                                   byte *               pdata,
                                   uint *               psize );

extern  int     gx_dc_read_color( gx_color_index *  pcolor,
                                  const gx_device * dev,
                                  const byte *      pdata,
                                  int               size );

#endif /* gxdcolor_INCLUDED */
