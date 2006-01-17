/* Copyright (C) 1989, 1995, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevsvga.h,v 1.5 2002/06/16 07:25:26 lpd Exp $ */
/* Common definitions and procedures for SuperVGA drivers */
/* Requires gdevpcfb.h */

#ifndef gdevsvga_INCLUDED
#  define gdevsvga_INCLUDED

/* Common procedures */

	/* See gxdevice.h for the definitions of the procedures. */

dev_proc_close_device(svga_close);
dev_proc_map_rgb_color(svga_map_rgb_color);
dev_proc_map_color_rgb(svga_map_color_rgb);
dev_proc_fill_rectangle(svga_fill_rectangle);
dev_proc_copy_mono(svga_copy_mono);
dev_proc_copy_color(svga_copy_color);
dev_proc_get_params(svga_get_params);
dev_proc_put_params(svga_put_params);
dev_proc_get_bits(svga_get_bits);
dev_proc_copy_alpha(svga_copy_alpha);

/* Table structure for looking up graphics modes. */
typedef struct {
    int width, height;		/* "key" */
    int mode;			/* "value" */
} mode_info;

/* The device descriptor structure */
typedef struct gx_device_svga_s gx_device_svga;
struct gx_device_svga_s {
    gx_device_common;
    int (*get_mode) (void);
    void (*set_mode) (int);
    void (*set_page) (gx_device_svga * fbdev, int pnum, int wnum);
    bool fixed_colors;		/* if true, used a fixed palette */
    const mode_info *mode;	/* BIOS display mode info */
    uint raster;		/* frame buffer bytes per line */
    int current_page;		/* current page */
    int wnum_read, wnum_write;	/* window #s for read vs. write */
    /* Following are device-specific. */
    union {
	struct {
	    void (*bios_set_page) (int, int);	/* set-page function */
	    int pn_shift;	/* log2(64K/granularity) */
	} vesa;
	struct {
	    int select_reg;	/* page-select register */
	} atiw;
	struct {
	    int et_model;	/* 4 for ET4000, */
	    /* 3 for ET3000 */
	} tseng;
    } info;
};

/* The initial parameters map an appropriate fraction of */
/* the screen to a full-page coordinate space. */
/* This may or may not be what is desired! */
#define svga_color_device(procs, name, depth, maxv, dither, get_mode, set_mode, set_page) {\
	std_device_color_body(gx_device_svga, &procs, name,\
	  640, 480,\
	  480 / PAGE_HEIGHT_INCHES, 480 / PAGE_HEIGHT_INCHES,\
	  /*dci_color(*/depth, maxv, dither/*)*/),\
	 { 0 },		/* std_procs */\
	get_mode, set_mode, set_page,\
	0 /*fixed_colors*/\
   }
#define svga_device(procs, name, get_mode, set_mode, set_page)\
  svga_color_device(procs, name, 8, 31, 4, get_mode, set_mode, set_page)

/* Utility procedures */
void svga_init_colors(gx_device *);
int svga_find_mode(gx_device *, const mode_info *);
int svga_open(gx_device *);

#endif /* gdevsvga_INCLUDED */
