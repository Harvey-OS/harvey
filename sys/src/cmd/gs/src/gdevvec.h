/* Copyright (C) 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevvec.h,v 1.16 2005/01/28 19:11:07 igor Exp $ */
/* Common definitions for "vector" devices */

#ifndef gdevvec_INCLUDED
#  define gdevvec_INCLUDED

#include "gp.h"			/* for gp_file_name_sizeof */
#include "gsropt.h"
#include "gxdevice.h"
#include "gdevbbox.h"
#include "gxiparam.h"
#include "gxistate.h"
#include "gxhldevc.h"
#include "stream.h"

/*
 * "Vector" devices produce a stream of higher-level drawing commands rather
 * than a raster image.  (We don't like the term "vector", since the command
 * vocabulary typically includes text and raster images as well as actual
 * vectors, but it's widely used in the industry, and we weren't able to
 * find one that read better.)  Some examples of "vector" formats are PDF,
 * PostScript, PCL XL, HP-GL/2 + RTL, CGM, Windows Metafile, and Macintosh
 * PICT.
 *
 * This file extends the basic driver structure with elements likely to be
 * useful to vector devices.  These include:
 *
 *      - Tracking whether any marks have been made on the page;
 *
 *      - Keeping track of the page bounding box;
 *
 *      - A copy of the most recently written current graphics state
 *      parameters;
 *
 *      - An output stream (for drivers that compress or otherwise filter
 *      their output);
 *
 *      - A vector of procedures for writing changes to the graphics state.
 *
 *      - The ability to work with scaled output coordinate systems.
 *
 * We expect to add more elements and procedures as we gain more experience
 * with this kind of driver.
 */

/* ================ Types and structures ================ */

/* Define the abstract type for a vector device. */
typedef struct gx_device_vector_s gx_device_vector;

/* Define the maximum size of the output file name. */
#define fname_size (gp_file_name_sizeof - 1)

/* Define the longest dash pattern we can remember. */
#define max_dash 11

/*
 * Define procedures for writing common output elements.  Not all devices
 * will support all of these elements.  Note that these procedures normally
 * only write out commands, and don't update the driver state itself.  All
 * of them are optional, called only as indicated under the utility
 * procedures below.
 */
typedef enum {
    gx_path_type_none = 0,
    /*
     * All combinations of flags are legal.  Multiple commands are
     * executed in the order fill, stroke, clip.
     */
    gx_path_type_fill = 1,
    gx_path_type_stroke = 2,
    gx_path_type_clip = 4,
    gx_path_type_winding_number = 0,
    gx_path_type_even_odd = 8,
    gx_path_type_optimize = 16,	/* OK to optimize paths by merging seg.s */
    gx_path_type_always_close = 32, /* include final closepath even if not stroke */
    gx_path_type_rule = gx_path_type_winding_number | gx_path_type_even_odd
} gx_path_type_t;
typedef enum {
    gx_rect_x_first,
    gx_rect_y_first
} gx_rect_direction_t;
typedef struct gx_device_vector_procs_s {
    /* Page management */
    int (*beginpage) (gx_device_vector * vdev);
    /* Imager state */
    int (*setlinewidth) (gx_device_vector * vdev, floatp width);
    int (*setlinecap) (gx_device_vector * vdev, gs_line_cap cap);
    int (*setlinejoin) (gx_device_vector * vdev, gs_line_join join);
    int (*setmiterlimit) (gx_device_vector * vdev, floatp limit);
    int (*setdash) (gx_device_vector * vdev, const float *pattern,
		    uint count, floatp offset);
    int (*setflat) (gx_device_vector * vdev, floatp flatness);
    int (*setlogop) (gx_device_vector * vdev, gs_logical_operation_t lop,
		     gs_logical_operation_t diff);
    /* Other state */
    bool (*can_handle_hl_color) (gx_device_vector * vdev, const gs_imager_state * pis, 
                         const gx_drawing_color * pdc);
    int (*setfillcolor) (gx_device_vector * vdev, const gs_imager_state * pis, 
                         const gx_drawing_color * pdc);
    int (*setstrokecolor) (gx_device_vector * vdev, const gs_imager_state * pis,
                           const gx_drawing_color * pdc);
    /* Paths */
    /* dopath and dorect are normally defaulted */
    int (*dopath) (gx_device_vector * vdev, const gx_path * ppath,
		   gx_path_type_t type, const gs_matrix *pmat);
    int (*dorect) (gx_device_vector * vdev, fixed x0, fixed y0, fixed x1,
		   fixed y1, gx_path_type_t type);
    int (*beginpath) (gx_device_vector * vdev, gx_path_type_t type);
    int (*moveto) (gx_device_vector * vdev, floatp x0, floatp y0,
		   floatp x, floatp y, gx_path_type_t type);
    int (*lineto) (gx_device_vector * vdev, floatp x0, floatp y0,
		   floatp x, floatp y, gx_path_type_t type);
    int (*curveto) (gx_device_vector * vdev, floatp x0, floatp y0,
		    floatp x1, floatp y1, floatp x2, floatp y2,
		    floatp x3, floatp y3, gx_path_type_t type);
    int (*closepath) (gx_device_vector * vdev, floatp x0, floatp y0,
		      floatp x_start, floatp y_start, gx_path_type_t type);
    int (*endpath) (gx_device_vector * vdev, gx_path_type_t type);
} gx_device_vector_procs;

/* Default implementations of procedures */
/* setflat does nothing */
int gdev_vector_setflat(gx_device_vector * vdev, floatp flatness);

/* dopath may call dorect, beginpath, moveto/lineto/curveto/closepath, */
/* endpath */
int gdev_vector_dopath(gx_device_vector * vdev, const gx_path * ppath,
		       gx_path_type_t type, const gs_matrix *pmat);

/* dorect may call beginpath, moveto, lineto, closepath */
int gdev_vector_dorect(gx_device_vector * vdev, fixed x0, fixed y0,
		       fixed x1, fixed y1, gx_path_type_t type);

/* Finally, define the extended device structure. */
#define gx_device_vector_common\
	gx_device_common;\
	gs_memory_t *v_memory;\
		/* Output element writing procedures */\
	const gx_device_vector_procs *vec_procs;\
		/* Output file */\
	char fname[fname_size + 1];\
	FILE *file;\
	stream *strm;\
	byte *strmbuf;\
	uint strmbuf_size;\
	int open_options;	/* see below */\
		/* Graphics state */\
	gs_imager_state state;\
	float dash_pattern[max_dash];\
	bool fill_used_process_color;\
	bool stroke_used_process_color;\
	gx_hl_saved_color saved_fill_color;\
	gx_hl_saved_color saved_stroke_color;\
	gs_id no_clip_path_id;	/* indicates no clipping */\
	gs_id clip_path_id;\
		/* Other state */\
	gx_path_type_t fill_options, stroke_options;  /* optimize */\
	gs_point scale;		/* device coords / scale => output coords */\
	bool in_page;		/* true if any marks on this page */\
	gx_device_bbox *bbox_device;	/* for tracking bounding box */\
		/* Cached values */\
	gx_color_index black, white
#define vdev_proc(vdev, p) ((vdev)->vec_procs->p)

#define vector_initial_values\
	0,		/* v_memory */\
	0,		/* vec_procs */\
	 { 0 },		/* fname */\
	0,		/* file */\
	0,		/* strm */\
	0,		/* strmbuf */\
	0,		/* strmbuf_size */\
	0,		/* open_options */\
	 { 0 },		/* state */\
	 { 0 },		/* dash_pattern */\
	true,		/* fill_used_process_color */\
	true,		/* stroke_used_process_color */\
	 { 0 },		/* fill_color ****** WRONG ****** */\
	 { 0 },		/* stroke_color ****** WRONG ****** */\
	gs_no_id,	/* clip_path_id */\
	gs_no_id,	/* no_clip_path_id */\
	0, 0,		/* fill/stroke_options */\
	 { X_DPI/72.0, Y_DPI/72.0 },	/* scale */\
	0/*false*/,	/* in_page */\
	0,		/* bbox_device */\
	gx_no_color_index,	/* black */\
	gx_no_color_index	/* white */

struct gx_device_vector_s {
    gx_device_vector_common;
};

/* st_device_vector is never instantiated per se, but we still need to */
/* extern its descriptor for the sake of subclasses. */
extern_st(st_device_vector);
#define public_st_device_vector()	/* in gdevvec.c */\
  gs_public_st_suffix_add3_final(st_device_vector, gx_device_vector,\
    "gx_device_vector", device_vector_enum_ptrs,\
    device_vector_reloc_ptrs, gx_device_finalize, st_device, strm, strmbuf,\
    bbox_device)
#define st_device_vector_max_ptrs (st_device_max_ptrs + 3)

/* ================ Utility procedures ================ */

/* Initialize the state. */
void gdev_vector_init(gx_device_vector * vdev);

/* Reset the remembered graphics state. */
void gdev_vector_reset(gx_device_vector * vdev);

/*
 * Open the output file and stream, with optional bbox tracking.
 * The options must be defined so that 0 is always the default.
 */
#define VECTOR_OPEN_FILE_ASCII 1	/* open file as text, not binary */
#define VECTOR_OPEN_FILE_SEQUENTIAL 2	/* open as non-seekable */
#define VECTOR_OPEN_FILE_SEQUENTIAL_OK 4  /* open as non-seekable if */
					/* open as seekable fails */
#define VECTOR_OPEN_FILE_BBOX 8		/* also open bbox device */
int gdev_vector_open_file_options(gx_device_vector * vdev,
				  uint strmbuf_size, int open_options);
#define gdev_vector_open_file_bbox(vdev, bufsize, bbox)\
  gdev_vector_open_file_options(vdev, bufsize,\
				(bbox ? VECTOR_OPEN_FILE_BBOX : 0))
#define gdev_vector_open_file(vdev, strmbuf_size)\
  gdev_vector_open_file_bbox(vdev, strmbuf_size, false)

/* Get the current stream, calling beginpage if in_page is false. */
stream *gdev_vector_stream(gx_device_vector * vdev);

/* Bring the logical operation up to date. */
/* May call setlogop. */
int gdev_vector_update_log_op(gx_device_vector * vdev,
			      gs_logical_operation_t lop);

/* Bring the fill color up to date. */
/* May call setfillcolor. */
int gdev_vector_update_fill_color(gx_device_vector * vdev,
				  const gs_imager_state * pis,
				  const gx_drawing_color * pdcolor);

/* Bring state up to date for filling. */
/* May call setflat, setfillcolor, setlogop. */
int gdev_vector_prepare_fill(gx_device_vector * vdev,
			     const gs_imager_state * pis,
			     const gx_fill_params * params,
			     const gx_drawing_color * pdcolor);

/* Bring state up to date for stroking.  Note that we pass the scale */
/* for the line width and dash offset explicitly. */
/* May call setlinewidth, setlinecap, setlinejoin, setmiterlimit, */
/* setdash, setflat, setstrokecolor, setlogop. */
/* Any of pis, params, and pdcolor may be NULL. */
int gdev_vector_prepare_stroke(gx_device_vector * vdev,
			       const gs_imager_state * pis,
			       const gx_stroke_params * params,
			       const gx_drawing_color * pdcolor,
			       floatp scale);

/*
 * Compute the scale for transforming the line width and dash pattern for a
 * stroke operation, and, if necessary to handle anisotropic scaling, a full
 * transformation matrix to be inverse-applied to the path elements as well.
 * Return 0 if only scaling, 1 if a full matrix is needed.
 */
int gdev_vector_stroke_scaling(const gx_device_vector *vdev,
			       const gs_imager_state *pis,
			       double *pscale, gs_matrix *pmat);

/* Prepare to write a path using the default implementation. */
typedef struct gdev_vector_dopath_state_s {
    /* Initialized by _init */
    gx_device_vector *vdev;
    gx_path_type_t type;
    bool first;
    gs_matrix scale_mat;
    /* Change dynamically */
    gs_point start;
    gs_point prev;
} gdev_vector_dopath_state_t;
void gdev_vector_dopath_init(gdev_vector_dopath_state_t *state,
			     gx_device_vector *vdev,
			     gx_path_type_t type, const gs_matrix *pmat);

/* Write a segment of a path using the default implementation. */
int gdev_vector_dopath_segment(gdev_vector_dopath_state_t *state, int pe_op,
			       gs_fixed_point vs[3]);

/* Write a polygon as part of a path (type = gx_path_type_none) */
/* or as a path. */
/* May call moveto, lineto, closepath (if close); */
/* may call beginpath & endpath if type != none. */
int gdev_vector_write_polygon(gx_device_vector * vdev,
			      const gs_fixed_point * points, uint count,
			      bool close, gx_path_type_t type);

/* Write a rectangle.  This is just a special case of write_polygon. */
int gdev_vector_write_rectangle(gx_device_vector * vdev,
				fixed x0, fixed y0, fixed x1, fixed y1,
				bool close, gx_rect_direction_t dir);

/* Write a clipping path by calling the path procedures. */
/* May call the same procedures as writepath. */
int gdev_vector_write_clip_path(gx_device_vector * vdev,
				const gx_clip_path * pcpath);

/* Bring the clipping state up to date. */
/* May call write_rectangle (q.v.), write_clip_path (q.v.). */
int gdev_vector_update_clip_path(gx_device_vector * vdev,
				 const gx_clip_path * pcpath);

/* Close the output file and stream. */
int gdev_vector_close_file(gx_device_vector * vdev);

/* ---------------- Image enumeration ---------------- */

/* Define a common set of state parameters for enumerating images. */
#define gdev_vector_image_enum_common\
	gx_image_enum_common;\
		/* Set by begin_image */\
	gs_memory_t *memory;	/* from begin_image */\
	gx_image_enum_common_t *default_info;	/* non-0 iff using default implementation */\
	gx_image_enum_common_t *bbox_info;	/* non-0 iff passing image data to bbox dev */\
	int width, height;\
	int bits_per_pixel;	/* (per plane) */\
	uint bits_per_row;	/* (per plane) */\
		/* Updated dynamically by image_data */\
	int y			/* 0 <= y < height */
typedef struct gdev_vector_image_enum_s {
    gdev_vector_image_enum_common;
} gdev_vector_image_enum_t;

extern_st(st_vector_image_enum);
#define public_st_vector_image_enum()	/* in gdevvec.c */\
  gs_public_st_ptrs2(st_vector_image_enum, gdev_vector_image_enum_t,\
    "gdev_vector_image_enum_t", vector_image_enum_enum_ptrs,\
    vector_image_enum_reloc_ptrs, default_info, bbox_info)

/*
 * Initialize for enumerating an image.  Note that the last argument is an
 * already-allocated enumerator, not a pointer to the place to store the
 * enumerator.
 */
int gdev_vector_begin_image(gx_device_vector * vdev,
			const gs_imager_state * pis, const gs_image_t * pim,
			gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		    gs_memory_t * mem, const gx_image_enum_procs_t * pprocs,
			    gdev_vector_image_enum_t * pie);

/* End an image, optionally supplying any necessary blank padding rows. */
/* Return 0 if we used the default implementation, 1 if not. */
int gdev_vector_end_image(gx_device_vector * vdev,
       gdev_vector_image_enum_t * pie, bool draw_last, gx_color_index pad);

/* ================ Device procedures ================ */

/* Redefine get/put_params to handle OutputFile. */
dev_proc_put_params(gdev_vector_put_params);
dev_proc_get_params(gdev_vector_get_params);

/* ---------------- Defaults ---------------- */

/* fill_rectangle may call setfillcolor, dorect. */
dev_proc_fill_rectangle(gdev_vector_fill_rectangle);
/* fill_path may call prepare_fill, writepath, write_clip_path. */
dev_proc_fill_path(gdev_vector_fill_path);
/* stroke_path may call prepare_stroke, write_path, write_clip_path. */
dev_proc_stroke_path(gdev_vector_stroke_path);
/* fill_trapezoid, fill_parallelogram, and fill_triangle may call */
/* setfillcolor, setlogop, beginpath, moveto, lineto, endpath. */
dev_proc_fill_trapezoid(gdev_vector_fill_trapezoid);
dev_proc_fill_parallelogram(gdev_vector_fill_parallelogram);
dev_proc_fill_triangle(gdev_vector_fill_triangle);

#endif /* gdevvec_INCLUDED */
