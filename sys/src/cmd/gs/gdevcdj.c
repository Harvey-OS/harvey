/* Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevcdj.c */
/* H-P colour printer drivers */
#include "std.h"		/* to stop stdlib.h redefining types */
#include <stdlib.h>		/* for rand() */
#include "gdevprn.h"
#include "gdevpcl.h"
#include "gsparam.h"
#include "gsstate.h"

/***
 *** This file contains multiple drivers.  The main body of code, and all
 *** but the DesignJet driver, were contributed by George Cameron;
 *** please contact g.cameron@biomed.abdn.ac.uk if you have questions.
 *     1 - cdj500:	HP DeskJet 500C
 *     2 - cdj550:	HP DeskJet 550C
 *     3 - pjxl300:	HP PaintJet XL300
 *     4 - pj:		HP PaintJet
 *     5 - pjxl:	HP PaintJet XL
 *     6 - declj250:	DEC LJ250
 *** The DesignJet 650C driver was contributed by Koert Zeilstra;
 *** please contact koert@zen.cais.com if you have questions.
 *     7 - dnj650c      HP DesignJet 650C
 *** The LaserJet 4 driver with dithering was contributed by Eckhard
 *** Rueggeberg; please contact eckhard@ts.go.dlr.de if you have questions.
 *     8 - lj4dith:	HP LaserJet 4 with dithering
 ***/

/*
 * All of these drivers have 8-bit (monochrome), 16-bit and 24-bit
 *     (colour) and for the DJ 550C 32-bit, (colour, cmyk mode)
 *     options in addition to the usual 1-bit and 3-bit modes
 * It is also possible to set various printer-specific parameters
 *     from the gs command line, eg.
 *
 *  gs -sDEVICE=cdj550 -dBitsPerPixel=16 -dDepletion=1 -dShingling=2 tiger.ps
 *
 * Please consult the appropriate section in the devices.doc file for
 * further details on all these drivers.
 */

#define DESKJET_PRINT_LIMIT  0.04	/* 'real' top margin? */
#define PAINTJET_PRINT_LIMIT 0.0	/* This is a guess.. */

/* Margins are left, bottom, right, top. */
#define DESKJET_MARGINS_LETTER   0.25, 0.50, 0.25, 0.167
#define DESKJET_MARGINS_A4       0.125, 0.50, 0.143, 0.167
#define LJET4_MARGINS  		 0.26, 0.0, 0.0, 0.0
/* The PaintJet and DesignJet seem to have the same margins */
/* regardless of paper size. */
#define PAINTJET_MARGINS         0.167, 0.167, 0.167, 0.167
#define DESIGNJET_MARGINS        0.167, 0.167, 0.167, 0.167

/* Default page size is US-Letter or A4 (other sizes from command line) */
#ifndef A4
#  define WIDTH_10THS            85
#  define HEIGHT_10THS           110
#else
#  define WIDTH_10THS            83      /* 210mm */
#  define HEIGHT_10THS           117     /* 297mm */
#endif

/* Define bits-per-pixel for generic drivers - default is 24-bit mode */
#ifndef BITSPERPIXEL
#  define BITSPERPIXEL 24
#endif

#define W sizeof(word)
#define I sizeof(int)

/* Printer types */
#define DJ500C   0
#define DJ550C   1
#define PJXL300  2
#define PJ180    3
#define PJXL180  4
#define DECLJ250 5
#define DNJ650C  6
#define LJ4DITH  7

/* No. of ink jets (used to minimise head movements) */
#define HEAD_ROWS_MONO 50
#define HEAD_ROWS_COLOUR 16

/* Colour mapping procedures */
private dev_proc_map_rgb_color (gdev_pcl_map_rgb_color);
private dev_proc_map_color_rgb (gdev_pcl_map_color_rgb);

/* Print-page, parameters and miscellaneous procedures */
private dev_proc_open_device(dj500c_open);
private dev_proc_open_device(dj550c_open);
private dev_proc_open_device(dnj650c_open);
private dev_proc_open_device(lj4dith_open);
private dev_proc_open_device(pj_open);
private dev_proc_open_device(pjxl_open);
private dev_proc_open_device(pjxl300_open);
private dev_proc_print_page(declj250_print_page);
private dev_proc_print_page(dj500c_print_page);
private dev_proc_print_page(dj550c_print_page);
private dev_proc_print_page(dnj650c_print_page);
private dev_proc_print_page(lj4dith_print_page);
private dev_proc_print_page(pj_print_page);
private dev_proc_print_page(pjxl_print_page);
private dev_proc_print_page(pjxl300_print_page);
private dev_proc_get_params(cdj_get_params);
private dev_proc_get_params(pj_get_params);
private dev_proc_get_params(pjxl_get_params);
private dev_proc_put_params(cdj_put_params);
private dev_proc_put_params(pj_put_params);
private dev_proc_put_params(pjxl_put_params);

/* The device descriptors */
typedef struct gx_device_cdj_s gx_device_cdj;
struct gx_device_cdj_s {
	gx_device_common;
	gx_prn_device_common;
	int correction;           /* Black correction parameter */
	int shingling;		  /* Interlaced, multi-pass printing */
	int depletion;		  /* 'Intelligent' dot-removal */
};

typedef struct gx_device_pjxl_s gx_device_pjxl;
struct gx_device_pjxl_s {
	gx_device_common;
	gx_prn_device_common;
	uint correction;          /* Black correction parameter */
	int printqual;            /* Mechanical print quality */
	int rendertype;           /* Driver or printer dithering control */
};

typedef struct gx_device_hp_s gx_device_hp;
struct gx_device_hp_s {
	gx_device_common;
	gx_prn_device_common;
	uint correction;          /* Black correction parameter
				   * (used only by DJ500C) */
};

typedef struct gx_device_hp_s gx_device_pj;

#define hp_device ((gx_device_hp *)pdev)
#define cdj       ((gx_device_cdj *)pdev)
#define pjxl      ((gx_device_pjxl *)pdev)
#define pj    ((gx_device_pj *)pdev)

/* Note: the computation of color_info values here must match */
/* the computation in the set_bpp procedure below. */
#define prn_hp_colour_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page)\
    prn_device_body(gx_device_printer, procs, dev_name,\
    WIDTH_10THS, HEIGHT_10THS, x_dpi, y_dpi, 0, 0, 0, 0,\
    (bpp == 32 ? 4 : (bpp == 1 || bpp == 8) ? 1 : 3), bpp,\
    (bpp >= 8 ? 255 : 1), (bpp >= 8 ? 255 : bpp > 1 ? 1 : 0),\
    (bpp >= 8 ? 5 : 2), (bpp >= 8 ? 5 : bpp > 1 ? 2 : 0),\
    print_page)

#define cdj_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page, correction, shingling, depletion)\
{ prn_hp_colour_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page),\
    correction,\
    shingling,\
    depletion\
}

#define pjxl_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page, printqual, rendertype)\
{ prn_hp_colour_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page), 0, \
    printqual,\
    rendertype\
}

#define pj_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page)\
{ prn_hp_colour_device(procs, dev_name, x_dpi, y_dpi, bpp, print_page), 0\
}

#define hp_colour_procs(proc_colour_open, proc_get_params, proc_put_params) {\
	proc_colour_open,\
	gx_default_get_initial_matrix,\
	gx_default_sync_output,\
	gdev_prn_output_page,\
	gdev_prn_close,\
	gdev_pcl_map_rgb_color,\
	gdev_pcl_map_color_rgb,\
	NULL,	/* fill_rectangle */\
	NULL,	/* tile_rectangle */\
	NULL,	/* copy_mono */\
	NULL,	/* copy_color */\
	NULL,	/* draw_line */\
	gx_default_get_bits,\
	proc_get_params,\
	proc_put_params\
}

private gx_device_procs cdj500_procs =
hp_colour_procs(dj500c_open, cdj_get_params, cdj_put_params);

private gx_device_procs cdj550_procs =
hp_colour_procs(dj550c_open, cdj_get_params, cdj_put_params);

private gx_device_procs dnj650c_procs =
hp_colour_procs(dnj650c_open, cdj_get_params, cdj_put_params);

private gx_device_procs lj4dith_procs =
hp_colour_procs(lj4dith_open, cdj_get_params, cdj_put_params);

private gx_device_procs pj_procs =
hp_colour_procs(pj_open, pj_get_params, pj_put_params);

private gx_device_procs pjxl_procs =
hp_colour_procs(pjxl_open, pjxl_get_params, pjxl_put_params);

private gx_device_procs pjxl300_procs =
hp_colour_procs(pjxl300_open, pjxl_get_params, pjxl_put_params);

gx_device_cdj far_data gs_cdjmono_device =
cdj_device(cdj500_procs, "cdjmono", 300, 300, 1,
	   dj500c_print_page, 4, 0, 1);

gx_device_cdj far_data gs_cdeskjet_device =
cdj_device(cdj500_procs, "cdeskjet", 300, 300, 3,
	   dj500c_print_page, 4, 2, 1);

gx_device_cdj far_data gs_cdjcolor_device =
cdj_device(cdj500_procs, "cdjcolor", 300, 300, 24,
	   dj500c_print_page, 4, 2, 1);

gx_device_cdj far_data gs_cdj500_device =
cdj_device(cdj500_procs, "cdj500", 300, 300, BITSPERPIXEL,
	   dj500c_print_page, 4, 2, 1);

gx_device_cdj far_data gs_cdj550_device =
cdj_device(cdj550_procs, "cdj550", 300, 300, BITSPERPIXEL,
	   dj550c_print_page, 0, 2, 1);

gx_device_pj far_data gs_declj250_device =
pj_device(pj_procs, "declj250", 180, 180, BITSPERPIXEL,
	   declj250_print_page);

gx_device_cdj far_data gs_dnj650c_device =
cdj_device(dnj650c_procs, "dnj650c", 300, 300, BITSPERPIXEL,
	   dnj650c_print_page, 0, 2, 1);
	   
gx_device_cdj far_data gs_lj4dith_device =
cdj_device(lj4dith_procs, "lj4dith", 600, 600, 8,
	   lj4dith_print_page, 4, 0, 1);

gx_device_pj far_data gs_pj_device =
pj_device(pj_procs, "pj", 180, 180, BITSPERPIXEL,
	   pj_print_page);

gx_device_pjxl far_data gs_pjxl_device =
pjxl_device(pjxl_procs, "pjxl", 180, 180, BITSPERPIXEL,
	   pjxl_print_page, 0, 0);

gx_device_pjxl far_data gs_pjxl300_device =
pjxl_device(pjxl300_procs, "pjxl300", 300, 300, BITSPERPIXEL,
	   pjxl300_print_page, 0, 0);

/* Forward references */
private int gdev_pcl_mode1compress(P3(const byte *, const byte *, byte *));
private int gdev_pcl_mode9compress(P4(int, const byte *, const byte *, byte *));
private int hp_colour_open(P2(gx_device *, int));
private int hp_colour_print_page(P3(gx_device_printer *, FILE *, int));
private int near put_param_int(P6(gs_param_list *, gs_param_name, int *, int, int, int));
private uint gdev_prn_rasterwidth(P2(const gx_device_printer *, int));
private void set_bpp(P2(gx_device *, int));
private void expand_line(P4(word *, int, int, int));

/* Open the printer and set up the margins. */
private int
dj500c_open(gx_device *pdev)
{ return hp_colour_open(pdev, DJ500C);
}

private int
dj550c_open(gx_device *pdev)
{ return hp_colour_open(pdev, DJ550C);
}

private int
dnj650c_open(gx_device *pdev)
{ return hp_colour_open(pdev, DNJ650C);
}

private int
lj4dith_open(gx_device *pdev)
{ return hp_colour_open(pdev, LJ4DITH);
}

private int
pjxl300_open(gx_device *pdev)
{ return hp_colour_open(pdev, PJXL300);
}

private int
pj_open(gx_device *pdev)
{ return hp_colour_open(pdev, PJ180);
}

private int
pjxl_open(gx_device *pdev)
{ return hp_colour_open(pdev, PJXL180);
}

private int
hp_colour_open(gx_device *pdev, int ptype)
{	/* Change the margins if necessary. */
  static const float dj_a4[4] = { DESKJET_MARGINS_A4 };
  static const float dj_letter[4] = { DESKJET_MARGINS_LETTER };
  static const float lj4_all[4] = { LJET4_MARGINS };
  static const float pj_all[4] = { PAINTJET_MARGINS };
  static const float dnj_all[4] = { DESIGNJET_MARGINS };
  const float _ds *m;

  /* Set up colour params if put_params has not already done so */
  if (pdev->color_info.num_components == 0)
    set_bpp(pdev, pdev->color_info.depth);

  switch (ptype) {
  case DJ500C:
  case DJ550C:
    m = (gdev_pcl_paper_size(pdev) == PAPER_SIZE_A4 ? dj_a4 :
	 dj_letter);
    break;
  case DNJ650C:
    m = dnj_all;
    break;
  case LJ4DITH:
    m = lj4_all;
    break;  
  case PJ180:
  case PJXL300:
  case PJXL180:
    m = pj_all;
    break;
  }
  gx_device_set_margins(pdev, m, true);
  return gdev_prn_open(pdev);
}

/* Added parameters for DeskJet 5xxC */

/* Get parameters.  In addition to the standard and printer 
 * parameters, we supply shingling and depletion parameters,
 * and control over the bits-per-pixel used in output rendering */
private int
cdj_get_params(gx_device *pdev, gs_param_list *plist)
{	int code = gdev_prn_get_params(pdev, plist);
	if ( code < 0 ||
	    (code = param_write_int(plist, "BlackCorrect", &cdj->correction)) < 0 ||
	    (code = param_write_int(plist, "Shingling", &cdj->shingling)) < 0 ||
	    (code = param_write_int(plist, "Depletion", &cdj->depletion)) < 0 ||
	    (code = param_write_int(plist, "BitsPerPixel", &cdj->color_info.depth)) < 0
	   )
	  ;
	return code;
}

/* Put parameters. */
private int
cdj_put_params(gx_device *pdev, gs_param_list *plist)
{	int correction = cdj->correction;
	int shingling = cdj->shingling;
	int depletion = cdj->depletion;
	int bpp = cdj->color_info.depth;
	int code = 0;

	code = put_param_int(plist, "BlackCorrect", &correction, 0, 9, code);
	code = put_param_int(plist, "Shingling", &shingling, 0, 2, code);
	code = put_param_int(plist, "Depletion", &depletion, 1, 3, code);
	code = put_param_int(plist, "BitsPerPixel", &bpp, 1, 32, code);

	if ( code < 0 )
	  return code;
	code = gdev_prn_put_params(pdev, plist);
	if ( code < 0 )
	  return code;

	cdj->correction = correction;
	cdj->shingling = shingling;
	cdj->depletion = depletion;
	if ( bpp != cdj->color_info.depth )
	  {	if ( pdev->is_open )
		  gs_closedevice(pdev);
		set_bpp(pdev, bpp);
	  }
	
	return 0;
}

/* Added parameters for PaintJet XL and PaintJet XL300 */

/* Get parameters.  In addition to the standard and printer
 * parameters, we supply print_quality and render_type 
 * parameters, together with bpp control. */
private int
pjxl_get_params(gx_device *pdev, gs_param_list *plist)
{	int code = gdev_prn_get_params(pdev, plist);
	if ( code < 0 ||
	    (code = param_write_int(plist, "PrintQuality", &pjxl->printqual)) < 0 ||
	    (code = param_write_int(plist, "RenderType", &pjxl->rendertype)) < 0 ||
	    (code = param_write_int(plist, "BitsPerPixel", &pjxl->color_info.depth)) < 0
	   )
	  ;
	return code;
}

/* Put parameters. */
private int
pjxl_put_params(gx_device *pdev, gs_param_list *plist)
{	int printqual = pjxl->printqual;
	int rendertype = pjxl->rendertype;
	int bpp = pjxl->color_info.depth;
	int code = 0;

	code = put_param_int(plist, "PrintQuality", &printqual, -1, 1, code);
	code = put_param_int(plist, "RenderType", &rendertype, 0, 10, code);
	code = put_param_int(plist, "BitsPerPixel", &bpp, 1, 32, code);

	if ( code < 0 )
	  return code;
	code = gdev_prn_put_params(pdev, plist);
	if ( code < 0 )
	  return code;

	pjxl->printqual = printqual;
	pjxl->rendertype = rendertype;
	if ( pjxl->rendertype > 0 )
	  {	/* If printer is doing the dithering, we must have a
		 * true-colour mode, ie. 16 or 24 bits per pixel */
		if ( bpp > 0 && bpp < 16 )
		  bpp = 24;
	  }
	if ( bpp != pdev->color_info.depth )
	  {	if ( pdev->is_open )
		  gs_closedevice(pdev);
		set_bpp(pdev, bpp);
	  }

	return 0;
}

/* Added parameters for PaintJet */

/* Get parameters.  In addition to the standard and printer */
/* parameters, we allow control of the bits-per-pixel */
private int
pj_get_params(gx_device *pdev, gs_param_list *plist)
{	int code = gdev_prn_get_params(pdev, plist);
	if ( code < 0 ||
	    (code = param_write_int(plist, "BitsPerPixel", &pj->color_info.depth)) < 0
	   )
	  ;
	return code;
}

/* Put parameters. */
private int
pj_put_params(gx_device *pdev, gs_param_list *plist)
{	int bpp = pj->color_info.depth;
	int code = put_param_int(plist, "BitsPerPixel", &bpp, 1, 32, 0);

	if ( code < 0 )
	  return code;
	code = gdev_prn_put_params(pdev, plist);
	if ( code < 0 )
	  return code;

	if ( bpp != pdev->color_info.depth )
	  {	if ( pdev->is_open )
		  gs_closedevice(pdev);
		set_bpp(pdev, bpp);
	  }

	return 0;
}

/* ------ Internal routines ------ */

/* The DeskJet500C can compress (mode 9) */
private int
dj500c_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, DJ500C);
}

/* The DeskJet550C can compress (mode 9) */
private int
dj550c_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, DJ550C);
}

/* The DesignJet650C can compress (mode 1) */
private int
dnj650c_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, DNJ650C);
}

private int
lj4dith_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, LJ4DITH);
}

/* The PJXL300 can compress (modes 2 & 3) */
private int
pjxl300_print_page(gx_device_printer * pdev, FILE * prn_stream)
{ int ret_code;
  /* Ensure we're operating in PCL mode */
  fputs("\033%-12345X@PJL enter language = PCL\n", prn_stream);
  ret_code = hp_colour_print_page(pdev, prn_stream, PJXL300);
  /* Reenter switch-configured language */
  fputs("\033%-12345X", prn_stream);
  return ret_code;
}

/* The PaintJet XL can compress (modes 2 & 3) */
private int
pjxl_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, PJXL180);
}

/* The PaintJet can compress (mode 1) */
private int
pj_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
  return hp_colour_print_page(pdev, prn_stream, PJ180);
}

/* The LJ250 can compress (mode 1) */
private int
declj250_print_page(gx_device_printer * pdev, FILE * prn_stream)
{ int ret_code;
  fputs("\033%8", prn_stream);	/* Enter PCL emulation mode */
  ret_code = hp_colour_print_page(pdev, prn_stream, DECLJ250);
  fputs("\033%@", prn_stream);	/* Exit PCL emulation mode */
  return ret_code;
}

/* MACROS FOR DITHERING (we use macros for compact source and faster code) */
/* Floyd-Steinberg dithering. Often results in a dramatic improvement in
 * subjective image quality, but can also produce dramatic increases in
 * amount of printer data generated and actual printing time!! Mode 9 2D
 * compression is still useful for fairly flat colour or blank areas but its
 * compression is much less effective in areas where the dithering has
 * effectively randomised the dot distribution. */

#define SHIFT ((I * 8) - 13)
#define RSHIFT ((I * 8) - 16)
#define RANDOM (((rand() << RSHIFT) % (MAXVALUE / 2))  - MAXVALUE / 4);
#define MINVALUE  0
#define MAXVALUE  (255 << SHIFT)
#define THRESHOLD (128 << SHIFT)
#define C 8

#define FSdither(inP, out, errP, Err, Bit, Offset, Element)\
	oldErr = Err;\
	Err = (errP[Element] + ((Err * 7 + C) >> 4) + ((int)inP[Element] << SHIFT));\
	if (Err > THRESHOLD) {\
	  out |= Bit;\
	  Err -= MAXVALUE;\
	}\
	errP[Element + Offset] += ((Err * 3 + C) >> 4);\
	errP[Element] = ((Err * 5 + oldErr + C) >> 4);

/* Here we rely on compiler optimisation to remove lines of the form
 * (if (1 >= 4) {...}, ie. the constant boolean expressions */

#define FSDline(scan, i, j, plane_size, cErr, mErr, yErr, kErr, cP, mP, yP, kP, n)\
{\
    if (scan == 0) {       /* going_up */\
      for (i = 0; i < plane_size; i++) {\
	byte c, y, m, k, bitmask;\
	int oldErr;\
	bitmask = 0x80;\
	for (c = m = y = k = 0; bitmask != 0; bitmask >>= 1) {\
	  if (n >= 4) {\
	    if (*dp) {\
	      FSdither(dp, k, ep, kErr, bitmask, -n, 0);\
	      cErr = mErr = yErr = 0;\
	    } else {\
	      FSdither(dp, c, ep, cErr, bitmask, -n, n - 3);\
	      FSdither(dp, m, ep, mErr, bitmask, -n, n - 2);\
	      FSdither(dp, y, ep, yErr, bitmask, -n, n - 1);\
	    }\
	  } else {\
	    if (n >= 3) {\
	      FSdither(dp, c, ep, cErr, bitmask, -n, n - 3);\
	      FSdither(dp, m, ep, mErr, bitmask, -n, n - 2);\
	    }\
	    FSdither(dp, y, ep, yErr, bitmask, -n, n - 1);\
	  }\
	  dp += n, ep += n;\
	}\
	if (n >= 4)\
	  *kP++ = k;\
	if (n >= 3) {\
	  *cP++ = c;\
          *mP++ = m;\
	}\
	*yP++ = y;\
      }\
    } else {		/* going_down */\
      for (i = 0; i < plane_size; i++) {\
	byte c, y, m, k, bitmask;\
	int oldErr;\
	bitmask = 0x01;\
	for (c = m = y = k = 0; bitmask != 0; bitmask <<= 1) {\
          dp -= n, ep -= n;\
	  if (n >= 4) {\
            if (*(dp)) {\
	      cErr = mErr = yErr = 0;\
	      FSdither(dp, k, ep, kErr, bitmask, n, 0);\
	    } else {\
	      FSdither(dp, y, ep, yErr, bitmask, n, n - 1);\
	      FSdither(dp, m, ep, mErr, bitmask, n, n - 2);\
	      FSdither(dp, c, ep, cErr, bitmask, n, n - 3);\
	    }\
	  } else {\
	    FSdither(dp, y, ep, yErr, bitmask, n, n - 1);\
	    if (n >= 3) {\
	      FSdither(dp, m, ep, mErr, bitmask, n, n - 2);\
	      FSdither(dp, c, ep, cErr, bitmask, n, n - 3);\
	    }\
	  }\
	}\
	*--yP = y;\
	if (n >= 3)\
	  { *--mP = m;\
	    *--cP = c;\
	  }\
	if (n >= 4)\
	  *--kP = k;\
      }\
    }\
}
/* END MACROS FOR DITHERING */

/* Some convenient shorthand .. */
#define x_dpi        (pdev->x_pixels_per_inch)
#define y_dpi        (pdev->y_pixels_per_inch)
#define CONFIG_16BIT "\033*v6W\000\003\000\005\006\005"
#define CONFIG_24BIT "\033*v6W\000\003\000\010\010\010"

/* To calculate buffer size as next greater multiple of both parameter and W */
#define calc_buffsize(a, b) (((((a) + ((b) * W) - 1) / ((b) * W))) * W)

/* Send the page to the printer.  Compress each scan line. */
private int
hp_colour_print_page(gx_device_printer * pdev, FILE * prn_stream, int ptype)
{
  uint raster_width = gdev_prn_rasterwidth(pdev, 1);
/*  int line_size = gdev_prn_rasterwidth(pdev, 0); */
  int line_size = gdev_prn_raster(pdev);
  int line_size_words = (line_size + W - 1) / W;
  int paper_size = gdev_pcl_paper_size((gx_device *)pdev);
  int num_comps = pdev->color_info.num_components;
  int bits_per_pixel = pdev->color_info.depth;
  int storage_bpp = bits_per_pixel;
  int expanded_bpp = bits_per_pixel;
  int plane_size, databuff_size;
  int combined_escapes = 1;
  int errbuff_size = 0;
  int outbuff_size = 0;
  int compression = 0;
  int scan = 0;
  int *errors[2];
  const char *cid_string;
  byte *data[4], *plane_data[4][4], *out_data;
  byte *out_row, *out_row_alt;
  word *storage;
  uint storage_size_words;

  /* Tricks and cheats ... */
  switch (ptype) {
  case DJ550C:
    if (num_comps == 3)
      num_comps = 4;                      /* 4-component printing */
    break;
  case PJXL300:
  case PJXL180:
    if (pjxl->rendertype > 0) {
      if (bits_per_pixel < 16)
	pjxl->rendertype = 0;
      else {
	/* Control codes for CID sequence */
	cid_string = (bits_per_pixel == 16) ? CONFIG_16BIT : CONFIG_24BIT;
	/* Pretend we're a monobit device so we send the data out unchanged */
	bits_per_pixel = storage_bpp = expanded_bpp = 1;
	num_comps = 1;
      }
    }
    break;
  }

  if (storage_bpp == 8 && num_comps >= 3)
    bits_per_pixel = expanded_bpp = 3;	/* Only 3 bits of each byte used */

  plane_size = calc_buffsize(line_size, storage_bpp);

  if (bits_per_pixel == 1) {		/* Data printed direct from i/p */
    databuff_size = 0;			/* so no data buffer required, */
    outbuff_size = plane_size * 4;	/* but need separate output buffers */
  }
  
  if (bits_per_pixel > 4) {		/* Error buffer for FS dithering */
    storage_bpp = expanded_bpp = 
      num_comps * 8;			/* 8, 24 or 32 bits */
    errbuff_size =			/* 4n extra values for line ends */
      calc_buffsize((plane_size * expanded_bpp + num_comps * 4) * I, 1);
  }

  databuff_size = plane_size * expanded_bpp;

  storage_size_words = ((plane_size + plane_size) * num_comps +
			databuff_size + errbuff_size + outbuff_size) / W;

  storage = (ulong *) gs_malloc(storage_size_words, W, "hp_colour_print_page");

  /*
   * The principal data pointers are stored as pairs of values, with
   * the selection being made by the 'scan' variable. The function of the
   * scan variable is overloaded, as it controls both the alternating
   * raster scan direction used in the Floyd-Steinberg dithering and also
   * the buffer alternation required for line-difference compression.
   *
   * Thus, the number of pointers required is as follows:
   * 
   *   errors:      2  (scan direction only)
   *   data:        4  (scan direction and alternating buffers)
   *   plane_data:  4  (scan direction and alternating buffers)
   */

  if (storage == 0)		/* can't allocate working area */
    return_error(gs_error_VMerror);
  else {
    int i;
    byte *p = out_data = out_row = (byte *)storage;    
    data[0] = data[1] = data[2] = p;
    data[3] = p + databuff_size;
    out_row_alt = out_row + plane_size * 2;
    if (bits_per_pixel > 1) {
      p += databuff_size;
    }
    if (bits_per_pixel > 4) {
      errors[0] = (int *)p + num_comps * 2;
      errors[1] = errors[0] + databuff_size;
      p += errbuff_size;
    }
    for (i = 0; i < num_comps; i++) {
      plane_data[0][i] = plane_data[2][i] = p;
      p += plane_size;
    }
    for (i = 0; i < num_comps; i++) {
      plane_data[1][i] = p;
      plane_data[3][i] = p + plane_size;
      p += plane_size;
    }
    if (bits_per_pixel == 1) {
      out_data = out_row = p;	  /* size is outbuff_size * 4 */
      out_row_alt = out_row + plane_size * 2;
      data[1] += databuff_size;   /* coincides with plane_data pointers */
      data[3] += databuff_size;
    }
  }
  
  /* Clear temp storage */
  memset(storage, 0, storage_size_words * W);
  
  /* Initialize printer. */
  if (ptype == LJ4DITH) 
    fputs("\033*rB", prn_stream);
  else
    fputs("\033*rbC", prn_stream);                   /* End raster graphics */
  fprintf(prn_stream, "\033*t%dR", (int)x_dpi);	   /* Set resolution */

#define DOFFSET (dev_t_margin(pdev) - DESKJET_PRINT_LIMIT)	/* Print position */
#define POFFSET (dev_t_margin(pdev) - PAINTJET_PRINT_LIMIT)
  switch (ptype) {
  case LJ4DITH:
    /* Page size, orientation, top margin & perforation skip */
    fprintf(prn_stream, "\033&l26A\033&l0o0e0L\033*r0F" );
    fprintf(prn_stream, "\033*p0x0Y" ); /* These Offsets are hacked ! */
    fprintf(prn_stream, "\033&u600D\033*r1A" );
    /* Select data compression */
    compression = 3;
    combined_escapes = 0;
    break;
  case DJ500C:
  case DJ550C:
    /* Page size, orientation, top margin & perforation skip */
    fprintf(prn_stream, "\033&l%daolE", paper_size);
    /* Set depletion and shingling levels */
    fprintf(prn_stream, "\033*o%dd%dQ", cdj->depletion, cdj->shingling);
    /* Move to top left of printed area */
    fprintf(prn_stream, "\033*p%dY", (int)(300 * DOFFSET));
    /* Set number of planes ((-)1 is mono, (-)3 is (cmy)rgb, -4 is cmyk),
     * and raster width, then start raster graphics */
    fprintf(prn_stream, "\033*r%ds-%du0A", raster_width, num_comps);
    /* Select data compression */
    compression = 9;
    break;
  case DNJ650C:
    fprintf (prn_stream, "\033%%0B"); /* Enter HPGL/2 mode */
    fprintf (prn_stream, "BP5,1"); /* Turn off autorotation */
    fprintf (prn_stream, "PS%d,%d",
	     (int)((pdev->height/pdev->y_pixels_per_inch)*1016),
	     (int)((pdev->width/pdev->x_pixels_per_inch)*1016)); /* Set length/width of page */
    fprintf (prn_stream, "PU"); /* Pen up */
    fprintf (prn_stream, "PA%d,%d", 0, 0); /* Move pen to upper-left */
    fprintf (prn_stream, "\033%%1A"); /* Enter HP-RTL mode */
    fprintf (prn_stream, "\033&a1N"); /* No negative motion - allow plotting
						while receiving */
    { static const char temp[] = {
        033, '*', 'v', '6', 'W',
	000 /* color model */,
	000 /* pixel encoding mode */,
	003 /* number of bits per index */,
	010 /* bits red */,
	010 /* bits green */,
	010 /* bits blue */
      };
      fwrite (temp, 1, sizeof(temp), prn_stream);
    }

    /* Set raster width */
    fprintf(prn_stream, "\033*r%dS", raster_width);
    /* Start raster graphics */
    fprintf(prn_stream, "\033*r1A");

    /* Select data compression */
    compression = 1;
    /* No combined escapes for raster transfers */
    combined_escapes = 0;
    break;
  case PJXL300:
    /* Page size, orientation, top margin & perforation skip */
    fprintf(prn_stream, "\033&l%daolE", paper_size);
    /* Set no-negative-motion mode, for faster (unbuffered) printing */
    fprintf(prn_stream, "\033&a1N");
    /* Set print quality */
    fprintf(prn_stream, "\033*o%dQ", pjxl->printqual);
    /* Move to top left of printed area */
    fprintf(prn_stream, "\033*p%dY", (int)(300 * POFFSET));
    /* Configure colour setup */
    if (pjxl->rendertype > 0) {
      /* Set render type */
      fprintf(prn_stream, "\033*t%dJ", pjxl->rendertype);
      /* Configure image data */
      fputs(cid_string, prn_stream);
      /* Set raster width, then start raster graphics */
      fprintf(prn_stream, "\033*r%ds1A", raster_width);
    } else {
      /* Set number of planes (1 is mono, 3 is rgb),
       * and raster width, then start raster graphics */
      fprintf(prn_stream, "\033*r%ds-%du0A", raster_width, num_comps);
    }
    /* No combined escapes for raster transfers */
    combined_escapes = 0;
    break;
  case PJXL180:
    /* Page size, orientation, top margin & perforation skip */
    fprintf(prn_stream, "\033&l%daolE", paper_size);
    /* Set print quality */
    fprintf(prn_stream, "\033*o%dQ", pjxl->printqual);
    /* Move to top left of printed area */
    fprintf(prn_stream, "\033*p%dY", (int)(180 * POFFSET));
    /* Configure colour setup */
    if (pjxl->rendertype > 0) {
      /* Set render type */
      fprintf(prn_stream, "\033*t%dJ", pjxl->rendertype);
      /* Configure image data */
      fputs(cid_string, prn_stream);
      /* Set raster width, then start raster graphics */
      fprintf(prn_stream, "\033*r%ds1A", raster_width);
    } else {
      /* Set number of planes (1 is mono, 3 is rgb),
       * and raster width, then start raster graphics */
      fprintf(prn_stream, "\033*r%ds%du0A", raster_width, num_comps);
    }
    break;
  case PJ180:
  case DECLJ250:
    /* Disable perforation skip */
    fprintf(prn_stream, "\033&lL");
    /* Move to top left of printed area */
    fprintf(prn_stream, "\033&a%dV", (int)(720 * POFFSET));
    /* Set number of planes (1 is mono, 3 is rgb),
     * and raster width, then start raster graphics */
    fprintf(prn_stream, "\033*r%ds%du0A", raster_width, num_comps);
    if (ptype == DECLJ250) {
      /* No combined escapes for raster transfers */
      combined_escapes = 0;
      /* From here on, we're a standard Paintjet .. */
      ptype = PJ180;
    }
    /* Select data compression */
    compression = 1;
    break;
  }

  /* Unfortunately, the Paintjet XL300 PCL interpreter introduces a
   * version of the PCL language which is different to all earlier HP
   * colour and mono inkjets, in that it loses the very useful ability
   * to use combined escape sequences with the raster transfer
   * commands. In this respect, it is incompatible even with the older
   * 180 dpi PaintJet and PaintJet XL printers!  Another regrettable
   * omission is that 'mode 9' compression is not supported, as this
   * mode can give both computational and PCL file size advantages. */

  if (combined_escapes) {
    /* From now on, all escape commands start with \033*b, so we
     * combine them (if the printer supports this). */
    fputs("\033*b", prn_stream);
     /* Set compression if the mode has been defined. */
    if (compression)
      fprintf(prn_stream, "%dm", compression);
  } else
    if (compression)
      fprintf(prn_stream, "\033*b%dM", compression);

  /* Send each scan line in turn */
  {
    int lend = pdev->height - (dev_t_margin(pdev) + dev_b_margin(pdev)) * y_dpi;
    int cErr, mErr, yErr, kErr;
    int this_pass, lnum, i;
    int num_blank_lines = 0;
    int start_rows = (num_comps == 1) ?
      HEAD_ROWS_MONO - 1 : HEAD_ROWS_COLOUR - 1;
    word rmask = ~(word) 0 << ((-pdev->width * storage_bpp) & (W * 8 - 1));

    cErr = mErr = yErr = kErr = 0;

    if (bits_per_pixel > 4) { /* Randomly seed initial error buffer */
      int *ep = errors[0];
      for (i = 0; i < databuff_size; i++) {
	*ep++ = RANDOM;
      }
    }

    /* Inhibit blank line printing for RGB-only printers, since in
     * this case 'blank' means black!  Also disabled for XL300 due to
     * an obscure bug in the printer's firmware */
    if (ptype == PJ180 || ptype == PJXL180 || ptype == PJXL300)
      start_rows = -1;

    this_pass = start_rows;
    for (lnum = 0; lnum < lend; lnum++) {
      word *data_words = (word *)data[scan];
      register word *end_data = data_words + line_size_words;

      gdev_prn_copy_scan_lines(pdev, lnum, data[scan], line_size);

      /* Mask off 1-bits beyond the line width. */
      end_data[-1] &= rmask;

      /* Remove trailing 0s. */
      while (end_data > data_words && end_data[-1] == 0)
	end_data--;
      if (ptype != DNJ650C)	/* DesignJet can't skip blank lines ? ? */
	if (end_data == data_words) {	/* Blank line */
	  num_blank_lines++;
	  continue;
	}
      /* Skip blank lines if any */
      if (num_blank_lines > 0) {
	if (num_blank_lines < this_pass) {
	  /* Moving down from current position
	   * causes head motion on the DeskJets, so
	   * if the number of lines is within the
	   * current pass of the print head, we're
	   * better off printing blanks. */
	  this_pass -= num_blank_lines;
	  if (combined_escapes) {
	    fputc('y', prn_stream);   /* Clear current and seed rows */
	    for (; num_blank_lines; num_blank_lines--)
	      fputc('w', prn_stream);
	  } else {
#if 0
/**************** The following code has been proposed ****************/
/**************** as a replacement: ****************/
	    fputs("\033*b1Y", prn_stream);   /* Clear current and seed rows */
	    if ( num_blank_lines > 1 )
	      fprintf(prn_stream, "\033*b%dY", num_blank_lines - 1);
	    num_blank_lines = 0;
#else
	    fputs("\033*bY", prn_stream);   /* Clear current and seed rows */
	    if (ptype == DNJ650C) {
	      fprintf (prn_stream, "\033*b%dY", num_blank_lines);
	      num_blank_lines = 0;
	    }
	    else {
	      for (; num_blank_lines; num_blank_lines--)
	        fputs("\033*bW", prn_stream);
	    }
#endif
	  }
	} else {
	  if (combined_escapes)
	    fprintf(prn_stream, "%dy", num_blank_lines);
	  else
	    fprintf(prn_stream, "\033*b%dY", num_blank_lines);
	}
	memset(plane_data[1 - scan][0], 0, plane_size * num_comps);
	num_blank_lines = 0;
	this_pass = start_rows;
      }
      {			/* Printing non-blank lines */
	register byte *kP = plane_data[scan + 2][3];
	register byte *cP = plane_data[scan + 2][2];
	register byte *mP = plane_data[scan + 2][1];
	register byte *yP = plane_data[scan + 2][0];
	register byte *dp = data[scan + 2];
	register int *ep = errors[scan];
	int zero_row_count;
	int i, j;
	byte *odp;

	if (this_pass)
	  this_pass--;
	else
	  this_pass = start_rows;

	if (expanded_bpp > bits_per_pixel)   /* Expand line if required */
	  expand_line(data_words, line_size, bits_per_pixel, expanded_bpp);

	/* In colour modes, we have some bit-shuffling to do before
	 * we can print the data; in FS mode we also have the
	 * dithering to take care of. */
	switch (expanded_bpp) {    /* Can be 1, 3, 8, 24 or 32 */
	case 3:
	  /* Transpose the data to get pixel planes. */
	  for (i = 0, odp = plane_data[scan][0]; i < databuff_size;
	       i += 8, odp++) {	/* The following is for 16-bit
				 * machines */
#define spread3(c)\
    { 0, c, c*0x100, c*0x101, c*0x10000L, c*0x10001L, c*0x10100L, c*0x10101L }
	    static ulong spr40[8] = spread3(0x40);
	    static ulong spr08[8] = spread3(8);
	    static ulong spr02[8] = spread3(2);
	    register byte *dp = data[scan] + i;
	    register ulong pword =
	    (spr40[dp[0]] << 1) +
	    (spr40[dp[1]]) +
	    (spr40[dp[2]] >> 1) +
	    (spr08[dp[3]] << 1) +
	    (spr08[dp[4]]) +
	    (spr08[dp[5]] >> 1) +
	    (spr02[dp[6]]) +
	    (spr02[dp[7]] >> 1);
	    odp[0] = (byte) (pword >> 16);
	    odp[plane_size] = (byte) (pword >> 8);
	    odp[plane_size * 2] = (byte) (pword);
	  }
	  break;

	case 8:
	  FSDline(scan, i, j, plane_size, cErr, mErr, yErr, kErr,
		  cP, mP, yP, kP, 1);
	  break;
	case 24:
	  FSDline(scan, i, j, plane_size, cErr, mErr, yErr, kErr,
		  cP, mP, yP, kP, 3);
	  break;
	case 32:
	  FSDline(scan, i, j, plane_size, cErr, mErr, yErr, kErr,
		  cP, mP, yP, kP, 4);
	  break;

	} /* switch(expanded_bpp) */

	/* Make sure all black is in the k plane */
	if (num_comps == 4) {
	  register word *kp = (word *)plane_data[scan][3];
	  register word *cp = (word *)plane_data[scan][2];
	  register word *mp = (word *)plane_data[scan][1];
	  register word *yp = (word *)plane_data[scan][0];
	  if (bits_per_pixel > 4) {  /* Done as 4 planes */
	    for (i = 0; i < plane_size / W; i++) {
	      word bits = *cp & *mp & *yp;
	      *kp++ |= bits;
	      bits = ~bits;
	      *cp++ &= bits;
	      *mp++ &= bits;
	      *yp++ &= bits;
	    }
	  } else {  /* This has really been done as 3 planes */
	    for (i = 0; i < plane_size / W; i++) {
	      word bits = *cp & *mp & *yp;
	      *kp++ = bits;
	      bits = ~bits;
	      *cp++ &= bits;
	      *mp++ &= bits;
	      *yp++ &= bits;
	    }
	  }
	}

	/* Transfer raster graphics
	 * in the order (K), C, M, Y. */
	for (zero_row_count = 0, i = num_comps - 1; i >= 0; i--) {
	  int output_plane = 1;
	  int out_count;
	  
	  switch (ptype) {
	  case DJ500C:    /* Always compress using mode 9 */
	  case DJ550C:
	    out_count = gdev_pcl_mode9compress(plane_size,
					       plane_data[scan][i],
					       plane_data[1 - scan][i],
					       out_data);

	    /* This optimisation allows early termination of the
	     * row, but this doesn't work correctly in an alternating
	     * mode 2 / mode 3 regime, so we only use it with mode 9
	     * compression */
           if (out_count == 0)
             { output_plane = 0;      /* No further output for this plane */
               if (i == 0)
                 fputc('w', prn_stream);
               else
                 zero_row_count++;
             }
           else
             { for (; zero_row_count; zero_row_count--)
                 fputc('v', prn_stream);
             }
	    break;
	  case PJ180:
	  case DNJ650C:
	    if (num_comps > 1)
	      { word *wp = (word *)plane_data[scan][i];
		for (j = 0; j < plane_size / W; j++, wp++)
		  *wp = ~*wp;
	      }
	    out_count = gdev_pcl_mode1compress((const byte *)
					       plane_data[scan][i],
					       (const byte *)
					       plane_data[scan][i] + plane_size - 1,
					       out_data);
	    break;
	  case PJXL180:    /* Need to invert data as CMY not supported */
	    if (num_comps > 1)
	      { word *wp = (word *)plane_data[scan][i];
		for (j = 0; j < plane_size / W; j++, wp++)
		  *wp = ~*wp;
	      }
	    /* fall through .. */
	  case PJXL300:     /* Compression modes 2 and 3 are both 
			     * available.  Try both and see which one
			     * produces the least output data. */
	  case LJ4DITH:
	    { const byte *plane = plane_data[scan][i];
	      byte *prev_plane = plane_data[1 - scan][i];
	      const word *row = (word *)plane;
	      const word *end_row = row + plane_size/W;
	      int count2 = gdev_pcl_mode2compress(row, end_row, out_row_alt);
	      int count3 = gdev_pcl_mode3compress(plane_size, plane, prev_plane, out_row);
	      int penalty = combined_escapes ? strlen("#m") : strlen("\033*b#M");
	      int penalty2 = (compression == 2 ? 0 : penalty);
	      int penalty3 = (compression == 3 ? 0 : penalty);
	      
	      if (count3 + penalty3 < count2 + penalty2)
		{ if ( compression != 3 ) {
		    if (combined_escapes)
		      fputs("3m", prn_stream);
		    else
		      fputs("\033*b3M", prn_stream);
		    compression = 3;
		  }
		  out_data = out_row;
		  out_count = count3;
		}
	      else
		{ if ( compression != 2 ) {
		    if (combined_escapes)
		      fputs("2m", prn_stream);
		    else
		      fputs("\033*b2M", prn_stream);
		    compression = 2;
		  }
		  out_data = out_row_alt;
		  out_count = count2;
		}
	    }
	    break;
	  }
	  if (output_plane) {
	    if (combined_escapes)
	      fprintf(prn_stream, "%d%c", out_count, "wvvv"[i]);
	    else
	      fprintf(prn_stream, "\033*b%d%c", out_count, "WVVV"[i]);
	    fwrite(out_data, sizeof(byte), out_count, prn_stream);
	  }
	  
	} /* Transfer Raster Graphics ... */
	scan = 1 - scan;          /* toggle scan direction */
      }	  /* Printing non-blank lines */
    }     /* for lnum ... */
  }       /* send each scan line in turn */

  if (combined_escapes)
    fputs("0M", prn_stream);

  /* end raster graphics */
  fputs("\033*rbC", prn_stream);

  /* eject page */
  if (ptype == PJ180)
    fputc('\f', prn_stream);
  else if (ptype == DNJ650C)
    fputs ("\033*rC\033%%0BPG;", prn_stream);
  else
    fputs("\033&l0H", prn_stream);

  /* free temporary storage */
  gs_free((char *) storage, storage_size_words, W, "hp_colour_print_page");

  return 0;
}

/*
 * Mode 9 2D compression for the HP DeskJet 5xxC. This mode can give
 * very good compression ratios, especially if there are areas of flat
 * colour (or blank areas), and so is 'highly recommended' for colour
 * printing in particular because of the very large amounts of data which
 * can be generated
 */
private int
gdev_pcl_mode9compress(int bytecount, const byte * current, const byte * previous, byte * compressed)
{
  register const byte *cur = current;
  register const byte *prev = previous;
  register byte *out = compressed;
  const byte *end = current + bytecount;

  while (cur < end) {		/* Detect a run of unchanged bytes. */
    const byte *run = cur;
    register const byte *diff;
    int offset;
    while (cur < end && *cur == *prev) {
      cur++, prev++;
    }
    if (cur == end)
      break;			/* rest of row is unchanged */
    /* Detect a run of changed bytes. */
    /* We know that *cur != *prev. */
    diff = cur;
    do {
      prev++;
      cur++;
    }
    while (cur < end && *cur != *prev);
    /* Now [run..diff) are unchanged, and */
    /* [diff..cur) are changed. */
    offset = diff - run;
    {
      const byte *stop_test = cur - 4;
      int dissimilar, similar;

      while (diff < cur) {
	const byte *compr = diff;
	const byte *next;	/* end of run */
	byte value;
	while (diff <= stop_test &&
	       ((value = *diff) != diff[1] ||
		value != diff[2] ||
		value != diff[3]))
	  diff++;

	/* Find out how long the run is */
	if (diff > stop_test)	/* no run */
	  next = diff = cur;
	else {
	  next = diff + 4;
	  while (next < cur && *next == value)
	    next++;
	}

#define MAXOFFSETU 15
#define MAXCOUNTU 7
	/* output 'dissimilar' bytes, uncompressed */
	if ((dissimilar = diff - compr)) {
	  int temp, i;

	  if ((temp = --dissimilar) > MAXCOUNTU)
	    temp = MAXCOUNTU;
	  if (offset < MAXOFFSETU)
	    *out++ = (offset << 3) | (byte) temp;
	  else {
	    *out++ = (MAXOFFSETU << 3) | (byte) temp;
	    offset -= MAXOFFSETU;
	    while (offset >= 255) {
	      *out++ = 255;
	      offset -= 255;
	    }
	    *out++ = offset;
	  }
	  if (temp == MAXCOUNTU) {
	    temp = dissimilar - MAXCOUNTU;
	    while (temp >= 255) {
	      *out++ = 255;
	      temp -= 255;
	    }
	    *out++ = (byte) temp;
	  }
	  for (i = 0; i <= dissimilar; i++)
	    *out++ = *compr++;
	  offset = 0;
	}			/* end uncompressed */
#define MAXOFFSETC 3
#define MAXCOUNTC 31
	/* output 'similar' bytes, run-length encoded */
	if ((similar = next - diff)) {
	  int temp;

	  if ((temp = (similar -= 2)) > MAXCOUNTC)
	    temp = MAXCOUNTC;
	  if (offset < MAXOFFSETC)
	    *out++ = 0x80 | (offset << 5) | (byte) temp;
	  else {
	    *out++ = 0x80 | (MAXOFFSETC << 5) | (byte) temp;
	    offset -= MAXOFFSETC;
	    while (offset >= 255) {
	      *out++ = 255;
	      offset -= 255;
	    }
	    *out++ = offset;
	  }
	  if (temp == MAXCOUNTC) {
	    temp = similar - MAXCOUNTC;
	    while (temp >= 255) {
	      *out++ = 255;
	      temp -= 255;
	    }
	    *out++ = (byte) temp;
	  }
	  *out++ = value;
	  offset = 0;
	}			/* end compressed */
	diff = next;
      }
    }
  }
  return out - compressed;
}

/*
 * Row compression for the H-P PaintJet.
 * Compresses data from row up to end_row, storing the result
 * starting at compressed.  Returns the number of bytes stored.
 * The compressed format consists of a byte N followed by a
 * data byte that is to be repeated N+1 times.
 * In the worst case, the `compressed' representation is
 * twice as large as the input.
 * We complement the bytes at the same time, because
 * we accumulated the image in complemented form.
 */
private int
gdev_pcl_mode1compress(const byte *row, const byte *end_row, byte *compressed)
{	register const byte *in = row;
	register byte *out = compressed;
	while ( in < end_row )
           {	byte test = *in++;
		const byte *run = in;
		while ( in < end_row && *in == test ) in++;
		/* Note that in - run + 1 is the repetition count. */
		while ( in - run > 255 )
                   {	*out++ = 255;
			*out++ = test;
			run += 256;
                   }
		*out++ = in - run;
		*out++ = test;
           }
	return out - compressed;
}

/*
 * Map a r-g-b color to a color index.
 * We complement the colours, since we're using cmy anyway, and
 * because the buffering routines expect white to be zero.
 * Includes colour balancing, following HP recommendations, to try
 * and correct the greenish cast resulting from an equal mix of the
 * c, m, y, inks by reducing the cyan component to give a truer black.
 */
private gx_color_index
gdev_pcl_map_rgb_color(gx_device *pdev, gx_color_value r,
				 gx_color_value g, gx_color_value b)
{
  if (gx_color_value_to_byte(r & g & b) == 0xff)
    return (gx_color_index)0;         /* white */
  else {
    int correction = hp_device->correction;
    gx_color_value c = gx_max_color_value - r;
    gx_color_value m = gx_max_color_value - g;
    gx_color_value y = gx_max_color_value - b;
    
    /* Colour correction for better blacks when using the colour ink
     * cartridge (on the DeskJet 500C only). We reduce the cyan component
     * by some fraction (eg. 4/5) to correct the slightly greenish cast
     * resulting from an equal mix of the three inks */
    if (correction) {
      ulong maxval, minval, range;
      
      maxval = c >= m ? (c >= y ? c : y) : (m >= y ? m : y);
      if (maxval > 0) {
	minval = c <= m ? (c <= y ? c : y) : (m <= y? m : y);
	range = maxval - minval;
	
#define shift (gx_color_value_bits - 12)
	c = ((c >> shift) * (range + (maxval * correction))) /
	  ((maxval * (correction + 1)) >> shift);
      }
    }
    
    switch (pdev->color_info.depth) {
    case 1:
      return ((c | m | y) > gx_max_color_value / 2 ?
	      (gx_color_index)1 : (gx_color_index)0);
    case 8:
      if (pdev->color_info.num_components >= 3)
#define gx_color_value_to_1bit(cv) ((cv) >> (gx_color_value_bits - 1))
	return (gx_color_value_to_1bit(c) +
		(gx_color_value_to_1bit(m) << 1) +
		(gx_color_value_to_1bit(y) << 2));
      else
#define red_weight 306
#define green_weight 601
#define blue_weight 117
	return ((((ulong)c * red_weight +
		  (ulong)m * green_weight +
		  (ulong)y * blue_weight)
		 >> (gx_color_value_bits + 2)));
    case 16:
#define gx_color_value_to_5bits(cv) ((cv) >> (gx_color_value_bits - 5))
#define gx_color_value_to_6bits(cv) ((cv) >> (gx_color_value_bits - 6))
      return (gx_color_value_to_5bits(y) +
	      (gx_color_value_to_6bits(m) << 5) +
	      (gx_color_value_to_5bits(c) << 11));
    case 24:
      return (gx_color_value_to_byte(y) +
	      (gx_color_value_to_byte(m) << 8) +
	      ((ulong)gx_color_value_to_byte(c) << 16));
    case 32:
      { return ((c == m && c == y) ? ((ulong)gx_color_value_to_byte(c) << 24)
	  : (gx_color_value_to_byte(y) +
	     (gx_color_value_to_byte(m) << 8) +
	     ((ulong)gx_color_value_to_byte(c) << 16)));
      }
    }
  }
  return (gx_color_index)0;   /* This never happens */
}
    
/* Map a color index to a r-g-b color. */
private int
gdev_pcl_map_color_rgb(gx_device *pdev, gx_color_index color,
			    gx_color_value prgb[3])
{
  /* For the moment, we simply ignore any black correction */
  switch (pdev->color_info.depth) {
  case 1:
    prgb[0] = prgb[1] = prgb[2] = -((gx_color_value)color ^ 1);
    break;
  case 8:
      if (pdev->color_info.num_components >= 3)
	{ gx_color_value c = (gx_color_value)color ^ 7;
	  prgb[0] = -(c & 1);
	  prgb[1] = -((c >> 1) & 1);
	  prgb[2] = -(c >> 2);
	}
      else
	{ gx_color_value value = (gx_color_value)color ^ 0xff;
	  prgb[0] = prgb[1] = prgb[2] = (value << 8) + value;
	}
    break;
  case 16:
    { gx_color_value c = (gx_color_value)color ^ 0xffff;
      ushort value = c >> 11;
      prgb[0] = ((value << 11) + (value << 6) + (value << 1) +
		 (value >> 4)) >> (16 - gx_color_value_bits);
      value = (c >> 6) & 0x3f;
      prgb[1] = ((value << 10) + (value << 4) + (value >> 2))
	>> (16 - gx_color_value_bits);
      value = c & 0x1f;
      prgb[2] = ((value << 11) + (value << 6) + (value << 1) +
		 (value >> 4)) >> (16 - gx_color_value_bits);
    }
    break;
  case 24:
    { gx_color_value c = (gx_color_value)color ^ 0xffffff;
      prgb[0] = gx_color_value_from_byte(c >> 16);
      prgb[1] = gx_color_value_from_byte((c >> 8) & 0xff);
      prgb[2] = gx_color_value_from_byte(c & 0xff);
    }
    break;
  case 32:
#define  gx_maxcol gx_color_value_from_byte(gx_color_value_to_byte(gx_max_color_value))
    { gx_color_value w = gx_maxcol - gx_color_value_from_byte(color >> 24);
      prgb[0] = w - gx_color_value_from_byte((color >> 16) & 0xff);
      prgb[1] = w - gx_color_value_from_byte((color >> 8) & 0xff);
      prgb[2] = w - gx_color_value_from_byte(color & 0xff);
    }
    break;
  }
  return 0;
}

/*
 * Convert and expand scanlines:
 *
 *       (a)	16 -> 24 bit   (1-stage)
 *       (b)	16 -> 32 bit   (2-stage)
 *   or  (c)	24 -> 32 bit   (1-stage)
 */
private void
expand_line(word *line, int linesize, int bpp, int ebpp)
{
  int endline = linesize;
  byte *start = (byte *)line;
  register byte *in, *out;

  if (bpp == 16)              /* 16 to 24 (cmy) if required */
    { register byte b0, b1;
      endline = ((endline + 1) / 2);
      in = start + endline * 2;
      out = start + (endline *= 3);
      
      while (in > start)
	{ b0 = *--in;
	  b1 = *--in;
	  *--out = (b0 << 3) + ((b0 >> 2) & 0x7);
	  *--out = (b1 << 5) + ((b0 >> 3)  & 0x1c) + ((b1 >> 1) & 0x3);
	  *--out = (b1 & 0xf8) + (b1 >> 5);
	}
    }

  if (ebpp == 32)             /* 24 (cmy) to 32 (cmyk) if required */
    { register byte c, m, y;
      endline = ((endline + 2) / 3);
      in = start + endline * 3;
      out = start + endline * 4;

      while (in > start)
	{ y = *--in;
	  m = *--in;
	  c = *--in;
	  if (c == y && c == m) {
	    *--out = 0, *--out = 0, *--out = 0;
	    *--out = c;
	  } else {
	    *--out = y, *--out = m, *--out = c;
	    *--out = 0;
	  }
	}
    }
}

private int near
put_param_int(gs_param_list *plist, gs_param_name pname, int *pvalue,
  int minval, int maxval, int ecode)
{	int code, value;
	switch ( code = param_read_int(plist, pname, &value) )
	{
	default:
		return code;
	case 1:
		return ecode;
	case 0:
		if ( value < minval || value > maxval )
			param_return_error(plist, pname, gs_error_rangecheck);
		*pvalue = value;
		return (ecode < 0 ? ecode : 1);
	}
}	

private void
set_bpp(gx_device *pdev, int bits_per_pixel)
{ gx_device_color_info *ci = &pdev->color_info;
  /* Only valid bits-per-pixel are 1, 3, 8, 16, 24, 32 */
  int bpp = bits_per_pixel < 3 ? 1 : bits_per_pixel < 8 ? 3 : 
    (bits_per_pixel >> 3) << 3;
  ci->num_components = ((bpp == 1) || (bpp == 8) ? 1 : 3);
  ci->depth = ((bpp > 1) && (bpp < 8) ? 8 : bpp);
  ci->max_gray = (bpp >= 8 ? 255 : 1);
  ci->max_color = (bpp >= 8 ? 255 : bpp > 1 ? 1 : 0);
  ci->dither_grays = (bpp >= 8 ? 5 : 2);
  ci->dither_colors = (bpp >= 8 ? 5 : bpp > 1 ? 2 : 0);
}

/* This returns either the number of pixels in a scan line, or the number
 * of bytes required to store the line, both clipped to the page margins */
private uint
gdev_prn_rasterwidth(const gx_device_printer *pdev, int pixelcount)
{
  ulong raster_width =
    pdev->width - pdev->x_pixels_per_inch * (dev_l_margin(pdev) + dev_r_margin(pdev));
  return (pixelcount ?
          (uint)raster_width :
          (uint)((raster_width * pdev->color_info.depth + 7) >> 3));
}
