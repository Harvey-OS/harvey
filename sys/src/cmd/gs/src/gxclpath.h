/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxclpath.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Definitions and support procedures for higher-level band list commands */
/* Extends (requires) gxcldev.h */

#ifndef gxclpath_INCLUDED
#  define gxclpath_INCLUDED

/* Define the flags indicating whether a band knows the current values of */
/* various miscellaneous parameters (pcls->known). */
#define flatness_known		(1<<0)
#define fill_adjust_known	(1<<1)
#define ctm_known		(1<<2)
#define line_width_known	(1<<3)
#define miter_limit_known	(1<<4)
#define misc0_known		(1<<5)
#define misc1_known		(1<<6)
#define dash_known		(1<<7)
#define alpha_known		(1<<8)
#define clip_path_known		(1<<9)
#define stroke_all_known	((1<<10)-1)
#define color_space_known	(1<<10)
/*#define all_known             ((1<<11)-1) */

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
    cmd_opv_set_color = 0xd0,	/* pqrsaaaa abbbbbcc cccddddd */
				/* (level[0] if p) (level[1] if q) */
				/* (level[2] if r) (level[3] if s) */
				/* colored halftone with base colors a,b,c,d */
    cmd_opv_set_color_short = 0xd1,	/* pqrsabcd, level[i] as above */
    cmd_opv_set_fill_adjust = 0xd2,	/* adjust_x/y(fixed) */
    cmd_opv_set_ctm = 0xd3,	/* [per sput/sget_matrix] */
    cmd_opv_set_color_space = 0xd4,	/* base(4)Indexed?(2)0(2) */
				/* [, hival#, table|map] */
    cmd_opv_set_misc2 = 0xd5,
#define cmd_set_misc2_cap_join (0x00) /* 00: cap(3)join(3) */
#define cmd_set_misc2_cj_ac_op_sa (0x40) /* 01: curve_join+1(3)acc.curves(1) */
				/* overprint(1)stroke_adj(1) */
#define cmd_set_misc2_notes (0x80)	/* 10: seg.notes(6) */
#define cmd_set_misc2_value (0xc0)	/* 11: (see below) */
#define cmd_set_misc2_flatness (0xc0+0)    /* 11000000, flatness(float) */
#define cmd_set_misc2_line_width (0xc0+1)  /* 11000001, line width(float) */
#define cmd_set_misc2_miter_limit (0xc0+2) /* 11000010, miter limit(float) */
#define cmd_set_misc2_alpha (0xc0+3)	   /* 11000011, alpha */
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
    cmd_opv_put_params = 0xdf,	/* (nothing) */
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
    /* The path drawing commands come in groups: */
    /* each group consists of a base command plus an offset */
    /* which is a cmd_dc_type. */
    cmd_opv_fill = 0xf0,
    cmd_opv_htfill = 0xf1,
    cmd_opv_colorfill = 0xf2,
    cmd_opv_eofill = 0xf3,
    cmd_opv_hteofill = 0xf4,
    cmd_opv_coloreofill = 0xf5,
    cmd_opv_stroke = 0xf6,
    cmd_opv_htstroke = 0xf7,
    cmd_opv_colorstroke = 0xf8,
    cmd_opv_polyfill = 0xf9,
    cmd_opv_htpolyfill = 0xfa,
    cmd_opv_colorpolyfill = 0xfb
} gx_cmd_xop;

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
gx_color_index cmd_drawing_colors_used(P2(gx_device_clist_writer *cldev,
					  const gx_drawing_color *pdcolor));

/*
 * Compute whether a drawing operation will require the slow (full-pixel)
 * RasterOp implementation.  If pdcolor is not NULL, it is the texture for
 * the RasterOp.
 */
bool cmd_slow_rop(P3(gx_device *dev, gs_logical_operation_t lop,
		     const gx_drawing_color *pdcolor));

/* Write out the color for filling, stroking, or masking. */
/* Return a cmd_dc_type. */
int cmd_put_drawing_color(P3(gx_device_clist_writer * cldev,
			     gx_clist_state * pcls,
			     const gx_drawing_color * pdcolor));

/* Clear (a) specific 'known' flag(s) for all bands. */
/* We must do this whenever the value of a 'known' parameter changes. */
void cmd_clear_known(P2(gx_device_clist_writer * cldev, uint known));

/* Write out values of any unknown parameters. */
#define cmd_do_write_unknown(cldev, pcls, must_know)\
  ( ~(pcls)->known & (must_know) ?\
    cmd_write_unknown(cldev, pcls, must_know) : 0 )
int cmd_write_unknown(P3(gx_device_clist_writer * cldev, gx_clist_state * pcls,
			 uint must_know));

/* Check whether we need to change the clipping path in the device. */
bool cmd_check_clip_path(P2(gx_device_clist_writer * cldev,
			    const gx_clip_path * pcpath));

#endif /* gxclpath_INCLUDED */
