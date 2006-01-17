/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxclpath.h,v 1.13 2005/10/10 18:58:18 leonardo Exp $ */
/* Definitions and support procedures for higher-level band list commands */
/* Extends (requires) gxcldev.h */

#ifndef gxclpath_INCLUDED
#  define gxclpath_INCLUDED

/*
 * Define the flags indicating whether a band knows the current values of
 * various miscellaneous parameters (pcls->known).  The first N flags match
 * the mask parameter for cmd_set_misc2 below.
 */
#define cap_join_known		(1<<0)
#define cj_ac_sa_known		(1<<1)
#define flatness_known		(1<<2)
#define line_width_known	(1<<3)
#define miter_limit_known	(1<<4)
#define op_bm_tk_known		(1<<5)
/* segment_notes must fit in the first byte (i.e. be less than 1<<7). */
#define segment_notes_known	(1<<6) /* not used in pcls->known */
/* (flags beyond this point require an extra byte) */
#define opacity_alpha_known	(1<<7)
#define shape_alpha_known	(1<<8)
#define alpha_known		(1<<9)
#define misc2_all_known		((1<<10)-1)
/* End of misc2 flags. */
#define fill_adjust_known	(1<<10)
#define ctm_known		(1<<11)
#define dash_known		(1<<12)
#define clip_path_known		(1<<13)
#define stroke_all_known	((1<<14)-1)
#define color_space_known	(1<<14)
/*#define all_known             ((1<<15)-1) */

/* Define the drawing color types for distinguishing different */
/* fill/stroke command variations. */
typedef enum {
    cmd_dc_type_pure = 0,
    cmd_dc_type_ht = 1,
    cmd_dc_type_color = 2
} cmd_dc_type;

/* Extend the command set.  See gxcldev.h for more information. */
typedef enum {
    cmd_op_misc2 = 0xd0,	/* (see below) */
    /* obsolete */
    /* cmd_opv_set_color = 0xd0, */	/* Used if base values do not fit into 1 bit */
    				/* #flags,#base[0],...#base[num_comp-1] if flags */
				/* colored halftone with base colors a,b,c,d */
    /* obsolete */
    /* cmd_opv_set_color_short = 0xd1, */ /* Used if base values fit into 1 bit */
    					/* If num_comp <= 4 then use: */
    					/* pqrsabcd, where a = base[0] */
					/* b = base[1], c= base[2], d = base[3] */
					/* p = level[0], q = level[1] */
					/* r = level[2], s = level[3] */
    					/* If num_comp > 4 then use: */
					/* #flags, #bases */
    cmd_opv_set_fill_adjust = 0xd2,	/* adjust_x/y(fixed) */
    cmd_opv_set_ctm = 0xd3,	/* [per sput/sget_matrix] */
    cmd_opv_set_color_space = 0xd4,	/* base(4)Indexed?(2)0(2) */
				/* [, hival#, table|map] */
    /*
     * cmd_opv_set_misc2_value is followed by a mask (a variable-length
     * integer), and then by parameter values for the parameters selected
     * by the mask.  See above for the "known" mask values.
     */
    /* cap_join: 0(2)cap(3)join(3) */
    /* cj_ac_sa: 0(3)curve_join+1(3)acc.curves(1)stroke_adj(1) */
    /* flatness: (float) */
    /* line width: (float) */
    /* miter limit: (float) */
    /* op_bm_tk: blend mode(5)text knockout(1)o.p.mode(1)o.p.(1) */
    /* segment notes: (byte) */
    /* opacity/shape: alpha(float)mask(TBD) */
    /* alpha: <<verbatim copy from imager state>> */
    cmd_opv_set_misc2 = 0xd5,	/* mask#, selected parameters */
    cmd_opv_set_dash = 0xd6,	/* adapt(1)abs.dot(1)n(6), dot */
				/* length(float), offset(float), */
				/* n x (float) */
    cmd_opv_enable_clip = 0xd7,	/* (nothing) */
    cmd_opv_disable_clip = 0xd8,	/* (nothing) */
    cmd_opv_begin_clip = 0xd9,	/* (nothing) */
    cmd_opv_end_clip = 0xda,	/* (nothing) */
    cmd_opv_begin_image_rect = 0xdb, /* same as begin_image, followed by */
				/* x0#, w-x1#, y0#, h-y1# */
    cmd_opv_begin_image = 0xdc,	/* image_type_table index, */
				/* [per image type] */
    cmd_opv_image_data = 0xdd,	/* height# (premature EOD if 0), */
				/* raster#, <data> */
    cmd_opv_image_plane_data = 0xde, /* height# (premature EOD if 0), */
				/* flags# (0 = same raster & data_x, */
				/* 1 = new raster & data_x, lsb first), */
				/* [raster#, [data_x#,]]* <data> */
    cmd_opv_extend = 0xdf,	/* command, varies */
    cmd_op_segment = 0xe0,	/* (see below) */
    cmd_opv_rmoveto = 0xe0,	/* dx%, dy% */
    cmd_opv_rlineto = 0xe1,	/* dx%, dy% */
    cmd_opv_hlineto = 0xe2,	/* dx% */
    cmd_opv_vlineto = 0xe3,	/* dy% */
    cmd_opv_rmlineto = 0xe4,	/* dx1%,dy1%, dx2%,dy2% */
    cmd_opv_rm2lineto = 0xe5,	/* dx1%,dy1%, dx2%,dy2%, dx3%,dy3% */
    cmd_opv_rm3lineto = 0xe6,	/* dx1%,dy1%, dx2%,dy2%, dx3%,dy3%, */
				/* [-dx2,-dy2 implicit] */
    cmd_opv_rrcurveto = 0xe7,	/* dx1%,dy1%, dx2%,dy2%, dx3%,dy3% */
      cmd_opv_min_curveto = cmd_opv_rrcurveto,
    cmd_opv_hvcurveto = 0xe8,	/* dx1%, dx2%,dy2%, dy3% */
    cmd_opv_vhcurveto = 0xe9,	/* dy1%, dx2%,dy2%, dx3% */
    cmd_opv_nrcurveto = 0xea,	/* dx2%,dy2%, dx3%,dy3% */
    cmd_opv_rncurveto = 0xeb,	/* dx1%,dy1%, dx2%,dy2% */
    cmd_opv_vqcurveto = 0xec,	/* dy1%, dx2%[,dy2=dx2 with sign */
				/* of dy1, dx3=dy1 with sign of dx2] */
    cmd_opv_hqcurveto = 0xed,	/* dx1%, [dx2=dy2 with sign */
				/* of dx1,]%dy2, [dy3=dx1 with sign */
				/* of dy2] */
    cmd_opv_scurveto = 0xee,	/* all implicit: previous op must have been */
				/* *curveto with one or more of dx/y1/3 = 0. */
				/* If h*: -dx3,dy3, -dx2,dy2, -dx1,dy1. */
				/* If v*: dx3,-dy3, dx2,-dy2, dx1,-dy1. */
      cmd_opv_max_curveto = cmd_opv_scurveto,
    cmd_opv_closepath = 0xef,	/* (nothing) */
    cmd_op_path = 0xf0,		/* (see below) */
    cmd_opv_fill = 0xf0,
    /* cmd_opv_htfill = 0xf1, */ /* obsolete */
    /* cmd_opv_colorfill = 0xf2, */ /* obsolete */
    cmd_opv_eofill = 0xf3,
    /* cmd_opv_hteofill = 0xf4, */ /* obsolete */
    /* cmd_opv_coloreofill = 0xf5, */ /* obsolete */
    cmd_opv_stroke = 0xf6,
    /* cmd_opv_htstroke = 0xf7, */ /* obsolete */
    /* cmd_opv_colorstroke = 0xf8, */ /* obsolete */
    cmd_opv_polyfill = 0xf9
    /* cmd_opv_htpolyfill = 0xfa, */ /* obsolete */
    /* cmd_opv_colorpolyfill = 0xfb */ /* obsolete */
} gx_cmd_xop;

/*
 * Further extended command set. This code always occupies a byte, which
 * is the second byte of a command whose first byte is cmd_opv_extend.
 */
typedef enum {
    cmd_opv_ext_put_params = 0x00,          /* serialized parameter list */
    cmd_opv_ext_create_compositor = 0x01,   /* compositor id,
                                             * serialized compositor */
    cmd_opv_ext_put_halftone = 0x02,        /* length of entire halftone */
    cmd_opv_ext_put_ht_seg = 0x03,          /* segment length,
                                             * halftone segment data */
    cmd_opv_ext_put_drawing_color = 0x04    /* length, color type id,
                                             * serialized color */
} gx_cmd_ext_op;

#define cmd_segment_op_num_operands_values\
  2, 2, 1, 1, 4, 6, 6, 6, 4, 4, 4, 4, 2, 2, 0, 0

#define cmd_misc2_op_name_strings\
  "set_color", "set_color_short", "set_fill_adjust", "set_ctm",\
  "set_color_space", "set_misc2", "set_dash", "enable_clip",\
  "disable_clip", "begin_clip", "end_clip", "begin_image_rect",\
  "begin_image", "image_data", "image_plane_data", "put_params"

#define cmd_segment_op_name_strings\
  "rmoveto", "rlineto", "hlineto", "vlineto",\
  "rmlineto", "rm2lineto", "rm3lineto", "rrcurveto",\
  "hvcurveto", "vhcurveto", "nrcurveto", "rncurveto",\
  "vqcurveto", "hqcurveto", "scurveto", "closepath"

#define cmd_path_op_name_strings\
  "fill", "htfill", "colorfill", "eofill",\
  "hteofill", "coloreofill", "stroke", "htstroke",\
  "colorstroke", "polyfill", "htpolyfill", "colorpolyfill",\
  "?fc?", "?fd?", "?fe?", "?ff?"

/*
 * We represent path coordinates as 'fixed' values in a variable-length,
 * relative form (s/t = sign, x/y = integer, f/g = fraction):
 *      00sxxxxx xfffffff ffffftyy yyyygggg gggggggg
 *      01sxxxxx xxxxffff ffffffff
 *      10sxxxxx xxxxxxxx xxxxffff ffffffff
 *      110sxxxx xxxxxxff
 *      111----- (a full-size `fixed' value)
 */
#define is_bits(d, n) !(((d) + ((fixed)1 << ((n) - 1))) & (-(fixed)1 << (n)))

/*
 * Maximum size of a halftone segment. This leaves enough headroom to
 * accommodate any reasonable requirements of the command buffer.
 */
#define cbuf_ht_seg_max_size    (cbuf_size - 32)    /* leave some headroom */

/* ---------------- Driver procedures ---------------- */

/* In gxclpath.c */
dev_proc_fill_path(clist_fill_path);
dev_proc_stroke_path(clist_stroke_path);
dev_proc_fill_parallelogram(clist_fill_parallelogram);
dev_proc_fill_triangle(clist_fill_triangle);

/* ---------------- Driver procedure support ---------------- */

/* The procedures and macros defined here are used when writing */
/* (gxclimag.c, gxclpath.c). */

/* Compare and update members of the imager state. */
#define state_neq(member)\
  (cdev->imager_state.member != pis->member)
#define state_update(member)\
  (cdev->imager_state.member = pis->member)

/* ------ Exported by gxclpath.c ------ */

/* Compute the colors used by a drawing color. */
gx_color_index cmd_drawing_colors_used(gx_device_clist_writer *cldev,
				       const gx_drawing_color *pdcolor);

/*
 * Compute whether a drawing operation will require the slow (full-pixel)
 * RasterOp implementation.  If pdcolor is not NULL, it is the texture for
 * the RasterOp.
 */
bool cmd_slow_rop(gx_device *dev, gs_logical_operation_t lop,
		  const gx_drawing_color *pdcolor);

/* Write out the color for filling, stroking, or masking. */
/* Return a cmd_dc_type. */
int cmd_put_drawing_color(gx_device_clist_writer * cldev,
			  gx_clist_state * pcls,
			  const gx_drawing_color * pdcolor);

/* Clear (a) specific 'known' flag(s) for all bands. */
/* We must do this whenever the value of a 'known' parameter changes. */
void cmd_clear_known(gx_device_clist_writer * cldev, uint known);

/* Compute the written CTM length. */
int cmd_write_ctm_return_length(gx_device_clist_writer * cldev, const gs_matrix *m);
/* Write out CTM. */
int cmd_write_ctm(const gs_matrix *m, byte *dp, int len);

/* Write out values of any unknown parameters. */
#define cmd_do_write_unknown(cldev, pcls, must_know)\
  ( ~(pcls)->known & (must_know) ?\
    cmd_write_unknown(cldev, pcls, must_know) : 0 )
int cmd_write_unknown(gx_device_clist_writer * cldev, gx_clist_state * pcls,
		      uint must_know);

/* Check whether we need to change the clipping path in the device. */
bool cmd_check_clip_path(gx_device_clist_writer * cldev,
			 const gx_clip_path * pcpath);

#endif /* gxclpath_INCLUDED */
