/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.

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

/* $Id: gdevdjet.c,v 1.13 2005/04/07 09:12:15 igor Exp $ */
/* HP LaserJet/DeskJet driver for Ghostscript */
#include "gdevprn.h"
#include "gdevdljm.h"

/*
 * Thanks for various improvements to:
 *      Jim Mayer (mayer@wrc.xerox.com)
 *      Jan-Mark Wams (jms@cs.vu.nl)
 *      Frans van Hoesel (hoesel@chem.rug.nl)
 *      George Cameron (g.cameron@biomed.abdn.ac.uk)
 *      Nick Duffek (nsd@bbc.com)
 * Thanks for the FS-600 driver to:
 *	Peter Schildmann (peter.schildmann@etechnik.uni-rostock.de)
 * Thanks for the LJIIID duplex capability to:
 *      PDP (Philip) Brown (phil@3soft-uk.com)
 * Thanks for the OCE 9050 driver to:
 *      William Bader (wbader@EECS.Lehigh.Edu)
 * Thanks for the LJ4D duplex capability to:
 *	Les Johnson <les@infolabs.com>
 */

/*
 * You may select a default resolution of 75, 100, 150, 300, or
 * (LJ4 only) 600 DPI in the makefile, or an actual resolution
 * on the gs command line.
 *
 * If the preprocessor symbol A4 is defined, the default paper size is
 * the European A4 size; otherwise it is the U.S. letter size (8.5"x11").
 *
 * To determine the proper "margin" settings for your printer, see the
 * file align.ps.
 */

/* Define the default, maximum resolutions. */
#ifdef X_DPI
#  define X_DPI2 X_DPI
#else
#  define X_DPI 300
#  define X_DPI2 600
#endif
#ifdef Y_DPI
#  define Y_DPI2 Y_DPI
#else
#  define Y_DPI 300
#  define Y_DPI2 600
#endif

/*
 * For all DeskJet Printers:
 *
 *  Maximum printing width               = 2400 dots = 8"
 *  Maximum recommended printing height  = 3100 dots = 10 1/3"
 *
 * All Deskjets have 1/2" unprintable bottom margin.
 * The recommendation comes from the HP Software Developer's Guide for
 * the DeskJet 500, DeskJet PLUS, and DeskJet printers, version C.01.00
 * of 12/1/90.
 *
 * Note that the margins defined just below here apply only to the DeskJet;
 * the paper size, width and height apply to the LaserJet as well.
 */

/* Margins are left, bottom, right, top. */
/* from Frans van Hoesel hoesel@rugr86.rug.nl. */
/* A4 has a left margin of 1/8 inch and at a printing width of
 * 8 inch this give a right margin of 0.143. The 0.09 top margin is
 * not the actual margin - which is 0.07 - but compensates for the
 * inexact paperlength which is set to 117 10ths.
 * Somebody should check for letter sized paper. I left it at 0.07".
 */
#define DESKJET_MARGINS_LETTER  (float)0.2, (float)0.45, (float)0.3, (float)0.05
#define DESKJET_MARGINS_A4	(float)0.125, (float)0.5, (float)0.143, (float)0.09
/* Similar margins for the LaserJet. */
/* These are defined in the PCL 5 Technical Reference Manual. */
/* Note that for PCL 5 printers, we get the printer to translate the */
/* coordinate system: the margins only define the unprintable area. */
#define LASERJET_MARGINS_A4	(float)0.167, (float)0.167, (float)0.167, (float)0.167
#define LASERJET_MARGINS_LETTER	(float)0.167, (float)0.167, (float)0.167, (float)0.167

/* See gdevdljm.h for the definitions of the PCL_ features. */

/* The device descriptors */
private dev_proc_open_device(hpjet_open);
private dev_proc_close_device(hpjet_close);
private dev_proc_print_page_copies(djet_print_page_copies);
private dev_proc_print_page_copies(djet500_print_page_copies);
private dev_proc_print_page_copies(fs600_print_page_copies);
private dev_proc_print_page_copies(ljet_print_page_copies);
private dev_proc_print_page_copies(ljetplus_print_page_copies);
private dev_proc_print_page_copies(ljet2p_print_page_copies);
private dev_proc_print_page_copies(ljet3_print_page_copies);
private dev_proc_print_page_copies(ljet3d_print_page_copies);
private dev_proc_print_page_copies(ljet4_print_page_copies);
private dev_proc_print_page_copies(ljet4d_print_page_copies);
private dev_proc_print_page_copies(lp2563_print_page_copies);
private dev_proc_print_page_copies(oce9050_print_page_copies);
private dev_proc_get_params(hpjet_get_params);
private dev_proc_put_params(hpjet_put_params);

private const gx_device_procs prn_hp_procs =
prn_params_procs(hpjet_open, gdev_prn_output_page, hpjet_close,
		 hpjet_get_params, hpjet_put_params);

typedef struct gx_device_hpjet_s gx_device_hpjet;

struct gx_device_hpjet_s {
    gx_device_common;
    gx_prn_device_common;
    int MediaPosition;
    bool MediaPosition_set;
    bool ManualFeed;
    bool ManualFeed_set;
};

#define HPJET_DEVICE(procs, dname, w10, h10, xdpi, ydpi, lm, bm, rm, tm, color_bits, print_page_copies)\
  { prn_device_std_margins_body_copies(gx_device_hpjet, procs, dname, \
        w10, h10, xdpi, ydpi, lm, tm, lm, bm, rm, tm, color_bits, \
        print_page_copies), \
    0, false, false, false }

const gx_device_hpjet gs_deskjet_device =
HPJET_DEVICE(prn_hp_procs, "deskjet",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0, 0, 0, 0,		/* margins filled in by hpjet_open */
	     1, djet_print_page_copies);

const gx_device_hpjet gs_djet500_device =
HPJET_DEVICE(prn_hp_procs, "djet500",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0, 0, 0, 0,		/* margins filled in by hpjet_open */
	     1, djet500_print_page_copies);

const gx_device_hpjet gs_fs600_device =
HPJET_DEVICE(prn_hp_procs, "fs600",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI2, Y_DPI2,
	     0.23, 0.0, 0.23, 0.04,      /* margins */
	     1, fs600_print_page_copies);

const gx_device_hpjet gs_laserjet_device =
HPJET_DEVICE(prn_hp_procs, "laserjet",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0.05, 0.25, 0.55, 0.25,	/* margins */
	     1, ljet_print_page_copies);

const gx_device_hpjet gs_ljetplus_device =
HPJET_DEVICE(prn_hp_procs, "ljetplus",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0.05, 0.25, 0.55, 0.25,	/* margins */
	     1, ljetplus_print_page_copies);

const gx_device_hpjet gs_ljet2p_device =
HPJET_DEVICE(prn_hp_procs, "ljet2p",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0.20, 0.25, 0.25, 0.25,	/* margins */
	     1, ljet2p_print_page_copies);

const gx_device_hpjet gs_ljet3_device =
HPJET_DEVICE(prn_hp_procs, "ljet3",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0.20, 0.25, 0.25, 0.25,	/* margins */
	     1, ljet3_print_page_copies);

const gx_device_hpjet gs_ljet3d_device =
HPJET_DEVICE(prn_hp_procs, "ljet3d",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0.20, 0.25, 0.25, 0.25,	/* margins */
	     1, ljet3d_print_page_copies);

const gx_device_hpjet gs_ljet4_device =
HPJET_DEVICE(prn_hp_procs, "ljet4",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI2, Y_DPI2,
	     0, 0, 0, 0,		/* margins */
	     1, ljet4_print_page_copies);

const gx_device_hpjet gs_ljet4d_device =
HPJET_DEVICE(prn_hp_procs, "ljet4d",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI2, Y_DPI2,
	     0, 0, 0, 0,		/* margins */
	     1, ljet4d_print_page_copies);

const gx_device_hpjet gs_lp2563_device =
HPJET_DEVICE(prn_hp_procs, "lp2563",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     0, 0, 0, 0,		/* margins */
	     1, lp2563_print_page_copies);

const gx_device_hpjet gs_oce9050_device =
HPJET_DEVICE(prn_hp_procs, "oce9050",
	     24 * 10, 24 * 10,	/* 24 inch roll (can print 32" also) */
	     400, 400,		/* 400 dpi */
	     0, 0, 0, 0,		/* margins */
	     1, oce9050_print_page_copies);

/* Open the printer, adjusting the margins if necessary. */
private int
hpjet_open(gx_device * pdev)
{				/* Change the margins if necessary. */
    gx_device_printer *const ppdev = (gx_device_printer *)pdev;
    const float *m = 0;
    bool move_origin = true;

    if (ppdev->printer_procs.print_page_copies == djet_print_page_copies ||
	ppdev->printer_procs.print_page_copies == djet500_print_page_copies
	) {
	static const float m_a4[4] =
	{DESKJET_MARGINS_A4};
	static const float m_letter[4] =
	{DESKJET_MARGINS_LETTER};

	m = (gdev_pcl_paper_size(pdev) == PAPER_SIZE_A4 ? m_a4 :
	     m_letter);
    } else if (ppdev->printer_procs.print_page_copies == oce9050_print_page_copies ||
	       ppdev->printer_procs.print_page_copies == lp2563_print_page_copies
	);
    else {			/* LaserJet */
	static const float m_a4[4] =
	{LASERJET_MARGINS_A4};
	static const float m_letter[4] =
	{LASERJET_MARGINS_LETTER};

	m = (gdev_pcl_paper_size(pdev) == PAPER_SIZE_A4 ? m_a4 :
	     m_letter);
	move_origin = false;
    }
    if (m != 0)
	gx_device_set_margins(pdev, m, move_origin);
    /* If this is a LJIIID, enable Duplex. */
    if (ppdev->printer_procs.print_page_copies == ljet3d_print_page_copies)
	ppdev->Duplex = true, ppdev->Duplex_set = 0;
    if (ppdev->printer_procs.print_page_copies == ljet4d_print_page_copies)
	ppdev->Duplex = true, ppdev->Duplex_set = 0;
    return gdev_prn_open(pdev);
}

/* hpjet_close is only here to eject odd numbered pages in duplex mode, */
/* and to reset the printer so the ink cartridge doesn't clog up. */
private int
hpjet_close(gx_device * pdev)
{
    gx_device_printer *const ppdev = (gx_device_printer *)pdev;
    int code = gdev_prn_open_printer(pdev, 1);

    if (code < 0)
	return code;
    if (ppdev->PageCount > 0) {
	if (ppdev->Duplex_set >= 0 && ppdev->Duplex)
	    fputs("\033&l0H", ppdev->file);

	fputs("\033E", ppdev->file);
    }

    return gdev_prn_close(pdev);
}

/* ------ Internal routines ------ */

/* Make an init string that contains paper tray selection. The resulting
   init string is stored in buf, so make sure that buf is at least 5
   bytes larger than str. */
private void
hpjet_make_init(gx_device_printer *pdev, char *buf, const char *str)
{
    gx_device_hpjet *dev = (gx_device_hpjet *)pdev;
    int paper_source = -1;
    int paper_source_tab[] = { 5, 1 };

    if (dev->ManualFeed_set && dev->ManualFeed) paper_source = 2;
    else if (dev->MediaPosition_set && dev->MediaPosition >= 0 &&
	     dev->MediaPosition < countof(paper_source_tab))
	paper_source = paper_source_tab[dev->MediaPosition];
    if (paper_source >= 0)
	sprintf(buf, "%s\033&l%dH", str, paper_source);
    else
	sprintf(buf, "%s", str);
}

/* The DeskJet can compress (mode 2) */
private int
djet_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
		       int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033&k1W\033*b2M");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_DJ_FEATURES, init);
}
/* The DeskJet500 can compress (modes 2&3) */
private int
djet500_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			  int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033&k1W");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_DJ500_FEATURES, init);
}
/* The Kyocera FS-600 laser printer (and perhaps other printers */
/* which use the PeerlessPrint5 firmware) doesn't handle        */
/* ESC&l#u and ESC&l#Z correctly.                               */
private int
fs600_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			int num_copies)
{
    int dots_per_inch = (int)pdev->y_pixels_per_inch;
    char base_init[60];
    char init[80];

    sprintf(base_init, "\033*r0F\033&u%dD", dots_per_inch);
    hpjet_make_init(pdev, init, base_init);
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					dots_per_inch, PCL_FS600_FEATURES,
					init);
}
/* The LaserJet series II can't compress */
private int
ljet_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
		       int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033*b0M");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LJ_FEATURES, init);
}
/* The LaserJet Plus can't compress */
private int
ljetplus_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			   int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033*b0M");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LJplus_FEATURES, init);
}
/* LaserJet series IIp & IId compress (mode 2) */
/* but don't support *p+ or *b vertical spacing. */
private int
ljet2p_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			 int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033*r0F\033*b2M");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LJ2p_FEATURES, init);
}
/* All LaserJet series IIIs (III,IIId,IIIp,IIIsi) compress (modes 2&3) */
/* They also need their coordinate system translated slightly. */
private int
ljet3_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033&l-180u36Z\033*r0F");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LJ3_FEATURES, init);
}
/* LaserJet IIId is same as LaserJet III, except for duplex */
private int
ljet3d_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			 int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033&l-180u36Z\033*r0F");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LJ3D_FEATURES, init);
}
/* LaserJet 4 series compresses, and it needs a special sequence to */
/* allow it to specify coordinates at 600 dpi. */
/* It too needs its coordinate system translated slightly. */
private int
ljet4_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			int num_copies)
{
    int dots_per_inch = (int)pdev->y_pixels_per_inch;
    char base_init[60];
    char init[80];

    sprintf(base_init, "\033&l-180u36Z\033*r0F\033&u%dD", dots_per_inch);
    hpjet_make_init(pdev, init, base_init);
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					dots_per_inch, PCL_LJ4_FEATURES,
					init);
}
private int
ljet4d_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			 int num_copies)
{
    int dots_per_inch = (int)pdev->y_pixels_per_inch;
    char base_init[60];
    char init[80];

    sprintf(base_init, "\033&l-180u36Z\033*r0F\033&u%dD", dots_per_inch);
    hpjet_make_init(pdev, init, base_init);
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					dots_per_inch, PCL_LJ4D_FEATURES,
					init);
}
/* The 2563B line printer can't compress */
/* and doesn't support *p+ or *b vertical spacing. */
private int
lp2563_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			 int num_copies)
{
    char init[80];

    hpjet_make_init(pdev, init, "\033*b0M");
    return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					300, PCL_LP2563B_FEATURES, init);
}
/* The Oce line printer has TIFF compression */
/* and doesn't support *p+ or *b vertical spacing. */
private int
oce9050_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			  int num_copies)
{
    int code;
    char init[80];

    /* Switch to HP_RTL. */
    fputs("\033%1B", prn_stream);	/* Enter HPGL/2 mode */
    fputs("BP", prn_stream);	/* Begin Plot */
    fputs("IN;", prn_stream);	/* Initialize (start plot) */
    fputs("\033%1A", prn_stream);	/* Enter PCL mode */

    hpjet_make_init(pdev, init, "\033*b0M");

    code = dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					400, PCL_OCE9050_FEATURES, init);

    /* Return to HPGL/2 mode. */
    fputs("\033%1B", prn_stream);	/* Enter HPGL/2 mode */
    if (code == 0) {
	fputs("PU", prn_stream);	/* Pen Up */
	fputs("SP0", prn_stream);	/* Pen Select */
	fputs("PG;", prn_stream);	/* Advance Full Page */
	fputs("\033E", prn_stream);	/* Reset */
    }
    return code;
}

private int
hpjet_get_params(gx_device *pdev, gs_param_list *plist)
{
    gx_device_hpjet *dev = (gx_device_hpjet *)pdev;
    int code = gdev_prn_get_params(pdev, plist);

    if (code >= 0)
	code = param_write_bool(plist, "ManualFeed", &dev->ManualFeed);
    return code;
}

private int
hpjet_put_params(gx_device *pdev, gs_param_list *plist)
{
    gx_device_hpjet *dev = (gx_device_hpjet *)pdev;
    int code;
    bool ManualFeed;
    bool ManualFeed_set = false;
    int MediaPosition;
    bool MediaPosition_set = false;

    code = param_read_bool(plist, "ManualFeed", &ManualFeed);
    if (code == 0) ManualFeed_set = true;
    if (code >= 0) {
	code = param_read_int(plist, "%MediaSource", &MediaPosition);
	if (code == 0) MediaPosition_set = true;
	else if (code < 0) {
	    if (param_read_null(plist, "%MediaSource") == 0) {
		code = 0;
	    }
	}
    }

    if (code >= 0)
	code = gdev_prn_put_params(pdev, plist);

    if (code >= 0) {
	if (ManualFeed_set) {
	    dev->ManualFeed = ManualFeed;
	    dev->ManualFeed_set = true;
	}
	if (MediaPosition_set) {
	    dev->MediaPosition = MediaPosition;
	    dev->MediaPosition_set = true;
	}
    }

    return code;
}
