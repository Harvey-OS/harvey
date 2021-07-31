/* Copyright (C) 1989, 1995, 1996, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevmswn.c,v 1.2 2000/09/19 19:00:14 lpd Exp $ */
/*
 * Microsoft Windows 3.n driver for Ghostscript.
 *
 * Original version by Russell Lang and Maurice Castro with help from
 * Programming Windows, 2nd Ed., Charles Petzold, Microsoft Press;
 * created from gdevbgi.c and gnuplot/term/win.trm 5th June 1992.
 * Extensively modified by L. Peter Deutsch, Aladdin Enterprises.
 */
#include "gdevmswn.h"
#include "gp.h"
#include "gpcheck.h"
#include "gsparam.h"
#include "gdevpccm.h"
#include "gsdll.h"
#include "gsdllwin.h"

/* Forward references */
private int win_set_bits_per_pixel(P2(gx_device_win *, int));

#define TIMER_ID 1

/* Open the win driver */
int
win_open(gx_device * dev)
{
    HDC hdc;
    int code;

    if (dev->width == INITIAL_WIDTH)
	dev->width = (int)(8.5 * dev->x_pixels_per_inch);
    if (dev->height == INITIAL_HEIGHT)
	dev->height = (int)(11.0 * dev->y_pixels_per_inch);

    if (wdev->BitsPerPixel == 0) {
	int depth;

	/* Set parameters that were unknown before opening device */
	/* Find out if the device supports color */
	/* We recognize 1, 4, 8, 16, 24 bit/pixel devices */
	hdc = GetDC(NULL);	/* get hdc for desktop */
	depth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
	if (depth > 16) {
	    wdev->BitsPerPixel = 24;
	} else if (depth > 8) {
	    wdev->BitsPerPixel = 16;
	} else if (depth >= 8) {
	    wdev->BitsPerPixel = 8;
	} else if (depth >= 4) {
	    wdev->BitsPerPixel = 4;
	} else {
	    wdev->BitsPerPixel = 1;
	}
	ReleaseDC(NULL, hdc);
	wdev->mapped_color_flags = 0;
    }
    if ((code = win_set_bits_per_pixel(wdev, wdev->BitsPerPixel)) < 0)
	return code;

    if (wdev->nColors > 0) {
	/* create palette for display */
	if ((wdev->limgpalette = win_makepalette(wdev))
	    == (LPLOGPALETTE) NULL)
	    return win_nomemory();
	wdev->himgpalette = CreatePalette(wdev->limgpalette);
    }
    return 0;
}

/* Make the output appear on the screen. */
int
win_sync_output(gx_device * dev)
{
    (*pgsdll_callback) (GSDLL_SYNC, (unsigned char *)wdev, 0);
    return (0);
}

/* Make the window visible, and display the output. */
int
win_output_page(gx_device * dev, int copies, int flush)
{
    (*pgsdll_callback) (GSDLL_PAGE, (unsigned char *)wdev, 0);
    return gx_finish_output_page(dev, copies, flush);;
}

/* Close the win driver */
int
win_close(gx_device * dev)
{
    /* Free resources */
    if (wdev->nColors > 0) {
	gs_free(wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
	DeleteObject(wdev->himgpalette);
	gs_free((char *)(wdev->limgpalette), 1, sizeof(LOGPALETTE) +
		(1 << (wdev->color_info.depth)) * sizeof(PALETTEENTRY),
		"win_close");
    }
    return (0);
}

/* Map a r-g-b color to the colors available under Windows */
gx_color_index
win_map_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		  gx_color_value b)
{
    switch (wdev->BitsPerPixel) {
	case 24:
	    return (((unsigned long)b >> (gx_color_value_bits - 8)) << 16) +
		(((unsigned long)g >> (gx_color_value_bits - 8)) << 8) +
		(((unsigned long)r >> (gx_color_value_bits - 8)));
	case 16:{
		gx_color_index color = ((r >> (gx_color_value_bits - 5)) << 11) +
				       ((g >> (gx_color_value_bits - 6)) << 5) +
				       (b >> (gx_color_value_bits - 5));
#if arch_is_big_endian
		ushort color16 = (ushort)color;
#else
		ushort color16 = (ushort)((color << 8) | (color >> 8));
#endif
		return color16;
	    }
	case 15:{
		gx_color_index color = ((r >> (gx_color_value_bits - 5)) << 10) +
				       ((g >> (gx_color_value_bits - 5)) << 5) +
				       (b >> (gx_color_value_bits - 5));
#if arch_is_big_endian
		ushort color15 = (ushort)color;
#else
		ushort color15 = (ushort)((color << 8) | (color >> 8));
#endif
		return color15;
	    }
	case 8:{
		int i;
		LPLOGPALETTE lpal = wdev->limgpalette;
		PALETTEENTRY *pep;
		byte cr, cg, cb;
		int mc_index;
		byte mc_mask;

		/* Check for a color in the palette of 64. */
		{
		    static const byte pal64[32] =
		    {
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1
		    };

		    if (pal64[r >> (gx_color_value_bits - 5)] &&
			pal64[g >> (gx_color_value_bits - 5)] &&
			pal64[b >> (gx_color_value_bits - 5)]
			)
			return (gx_color_index) (
				   ((r >> (gx_color_value_bits - 2)) << 4) +
				   ((g >> (gx_color_value_bits - 2)) << 2) +
					    (b >> (gx_color_value_bits - 2))
			    );
		}

		/* map colors to 0->255 in 32 steps */
		cr = win_color_value(r);
		cg = win_color_value(g);
		cb = win_color_value(b);

		/* Search in palette, skipping the first 64. */
		mc_index = ((cr >> 3) << 7) + ((cg >> 3) << 2) + (cb >> 6);
		mc_mask = 0x80 >> ((cb >> 3) & 7);
		if (wdev->mapped_color_flags[mc_index] & mc_mask)
		    for (i = wdev->nColors, pep = &lpal->palPalEntry[i];
			 --pep, --i >= 64;
			) {
			if (cr == pep->peRed &&
			    cg == pep->peGreen &&
			    cb == pep->peBlue
			    )
			    return ((gx_color_index) i);	/* found it */
		    }
		/* next try adding it to palette */
		i = wdev->nColors;
		if (i < 220) {	/* allow 36 for windows and other apps */
		    LPLOGPALETTE lipal = wdev->limgpalette;

		    wdev->nColors = i + 1;

		    DeleteObject(wdev->himgpalette);
		    lipal->palPalEntry[i].peFlags = 0;
		    lipal->palPalEntry[i].peRed = cr;
		    lipal->palPalEntry[i].peGreen = cg;
		    lipal->palPalEntry[i].peBlue = cb;
		    lipal->palNumEntries = wdev->nColors;
		    wdev->himgpalette = CreatePalette(lipal);

		    wdev->mapped_color_flags[mc_index] |= mc_mask;
		    return ((gx_color_index) i);	/* return new palette index */
		}
		return (gx_no_color_index);	/* not found - dither instead */
	    }
	case 4:
	    if ((r == g) && (g == b) && (r >= gx_max_color_value / 3 * 2 - 1)
		&& (r < gx_max_color_value / 4 * 3))
		return ((gx_color_index) 8);	/* light gray */
	    return pc_4bit_map_rgb_color(dev, r, g, b);
    }
    return (gx_default_map_rgb_color(dev, r, g, b));
}

/* Map a color code to r-g-b. */
int
win_map_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
    gx_color_value one;
    ushort value;

    switch (wdev->BitsPerPixel) {
	case 24:
	    one = (gx_color_value) (gx_max_color_value / 255);
	    prgb[0] = ((color) & 255) * one;
	    prgb[1] = ((color >> 8) & 255) * one;
	    prgb[2] = ((color >> 16) & 255) * one;
	    break;
	case 16:
	    value = (color >> 11) & 0x1f;
	    prgb[0] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4)) >> (16 - gx_color_value_bits);
	    value = (color >> 5) & 0x3f;
	    prgb[1] = ((value << 10) + (value << 4) + (value >> 2)) >> (16 - gx_color_value_bits);
	    value = (color) & 0x1f;
	    prgb[2] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4)) >> (16 - gx_color_value_bits);
	    break;
	case 15:
	    value = (color >> 10) & 0x1f;
	    prgb[0] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4)) >> (16 - gx_color_value_bits);
	    value = (color >> 5) & 0x1f;
	    prgb[1] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4)) >> (16 - gx_color_value_bits);
	    value = (color) & 0x1f;
	    prgb[2] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4)) >> (16 - gx_color_value_bits);
	    break;
	case 8:
	    if (!dev->is_open)
		return -1;
	    one = (gx_color_value) (gx_max_color_value / 255);
	    prgb[0] = wdev->limgpalette->palPalEntry[(int)color].peRed * one;
	    prgb[1] = wdev->limgpalette->palPalEntry[(int)color].peGreen * one;
	    prgb[2] = wdev->limgpalette->palPalEntry[(int)color].peBlue * one;
	    break;
	case 4:
	    if (color == 8)	/* VGA light gray */
		prgb[0] = prgb[1] = prgb[2] = (gx_max_color_value / 4 * 3);
	    else
		pc_4bit_map_color_rgb(dev, color, prgb);
	    break;
	default:
	    prgb[0] = prgb[1] = prgb[2] =
		(int)color ? gx_max_color_value : 0;
    }
    return 0;
}

/* Get Win parameters */
int
win_get_params(gx_device * dev, gs_param_list * plist)
{
    int code = gx_default_get_params(dev, plist);

    return code;
}

/* Put parameters. */
/* Set window parameters -- size and resolution. */
/* We implement this ourselves so that we can do it without */
/* closing and opening the device. */
int
win_put_params(gx_device * dev, gs_param_list * plist)
{
    int ecode = 0, code;
    bool is_open = dev->is_open;
    int width = dev->width;
    int height = dev->height;
    int old_bpp = dev->color_info.depth;
    int bpp = old_bpp;
    byte *old_flags = wdev->mapped_color_flags;

    /* Handle extra parameters */

    switch (code = param_read_int(plist, "BitsPerPixel", &bpp)) {
	case 0:
	    if (dev->is_open && bpp != old_bpp)
		ecode = gs_error_rangecheck;
	    else {		/* Don't release existing mapped_color_flags. */
		if (bpp != 8)
		    wdev->mapped_color_flags = 0;
		code = win_set_bits_per_pixel(wdev, bpp);
		if (code < 0)
		    ecode = code;
		else
		    break;
	    }
	    goto bppe;
	default:
	    ecode = code;
	  bppe:param_signal_error(plist, "BitsPerPixel", ecode);
	case 1:
	    break;
    }

    if (ecode >= 0) {		/* Prevent gx_default_put_params from closing the device. */
	dev->is_open = false;
	ecode = gx_default_put_params(dev, plist);
	dev->is_open = is_open;
    }
    if (ecode < 0) {		/* If we allocated mapped_color_flags, release it. */
	if (wdev->mapped_color_flags != 0 && old_flags == 0)
	    gs_free(wdev->mapped_color_flags, 4096, 1,
		    "win_put_params");
	wdev->mapped_color_flags = old_flags;
	if (bpp != old_bpp)
	    win_set_bits_per_pixel(wdev, old_bpp);
	return ecode;
    }
    if (wdev->mapped_color_flags == 0 && old_flags != 0) {	/* Release old mapped_color_flags. */
	gs_free(old_flags, 4096, 1, "win_put_params");
    }
    /* Hand off the change to the implementation. */
    if (is_open && (bpp != old_bpp ||
		    dev->width != width || dev->height != height)
	) {
	int ccode;

	(*wdev->free_bitmap) (wdev);
	ccode = (*wdev->alloc_bitmap) (wdev, (gx_device *) wdev);
	if (ccode < 0) {	/* Bad news!  Some of the other device parameters */
	    /* may have changed.  We don't handle this. */
	    /* This is ****** WRONG ******. */
	    dev->width = width;
	    dev->height = height;
	    win_set_bits_per_pixel(wdev, old_bpp);
	    (*wdev->alloc_bitmap) (wdev, dev);
	    return ccode;
	}
    }
    return 0;
}

/* ------ Internal routines ------ */

#undef wdev



/* out of memory error message box */
int
win_nomemory(void)
{
    MessageBox((HWND) NULL, (LPSTR) "Not enough memory", (LPSTR) szAppName, MB_ICONSTOP);
    return gs_error_limitcheck;
}


LPLOGPALETTE
win_makepalette(gx_device_win * wdev)
{
    int i, val;
    LPLOGPALETTE logpalette;

    logpalette = (LPLOGPALETTE) gs_malloc(1, sizeof(LOGPALETTE) +
		     (1 << (wdev->color_info.depth)) * sizeof(PALETTEENTRY),
					  "win_makepalette");
    if (logpalette == (LPLOGPALETTE) NULL)
	return (0);
    logpalette->palVersion = 0x300;
    logpalette->palNumEntries = wdev->nColors;
    for (i = 0; i < wdev->nColors; i++) {
	logpalette->palPalEntry[i].peFlags = 0;
	switch (wdev->nColors) {
	    case 64:
		/* colors are rrggbb */
		logpalette->palPalEntry[i].peRed = ((i & 0x30) >> 4) * 85;
		logpalette->palPalEntry[i].peGreen = ((i & 0xC) >> 2) * 85;
		logpalette->palPalEntry[i].peBlue = (i & 3) * 85;
		break;
	    case 16:
		/* colors are irgb */
		val = (i & 8 ? 255 : 128);
		logpalette->palPalEntry[i].peRed = i & 4 ? val : 0;
		logpalette->palPalEntry[i].peGreen = i & 2 ? val : 0;
		logpalette->palPalEntry[i].peBlue = i & 1 ? val : 0;
		if (i == 8) {	/* light gray */
		    logpalette->palPalEntry[i].peRed =
			logpalette->palPalEntry[i].peGreen =
			logpalette->palPalEntry[i].peBlue = 192;
		}
		break;
	    case 2:
		logpalette->palPalEntry[i].peRed =
		    logpalette->palPalEntry[i].peGreen =
		    logpalette->palPalEntry[i].peBlue = (i ? 255 : 0);
		break;
	}
    }
    return (logpalette);
}


private int
win_set_bits_per_pixel(gx_device_win * wdev, int bpp)
{
    static const gx_device_color_info win_24bit_color = dci_color(24, 255, 255);
    static const gx_device_color_info win_16bit_color = dci_color(16, 255, 255);
    static const gx_device_color_info win_8bit_color = dci_color(8, 31, 4);
    static const gx_device_color_info win_ega_color = dci_pc_4bit;
    static const gx_device_color_info win_vga_color = dci_pc_4bit;
    static const gx_device_color_info win_mono_color = dci_black_and_white;
    /* remember old anti_alias info */
    gx_device_anti_alias_info anti_alias = wdev->color_info.anti_alias;
    HDC hdc;

    switch (bpp) {
	case 24:
	    wdev->color_info = win_24bit_color;
	    wdev->nColors = -1;
	    break;
	case 16:
	case 15:
	    wdev->color_info = win_16bit_color;
	    wdev->nColors = -1;
	    break;
	case 8:
	    /* use 64 static colors and 166 dynamic colors from 8 planes */
	    wdev->color_info = win_8bit_color;
	    wdev->nColors = 64;
	    break;
	case 4:
	    hdc = GetDC(NULL);
	    if (GetDeviceCaps(hdc, VERTRES) <= 350)
		wdev->color_info = win_ega_color;
	    else
		wdev->color_info = win_vga_color;
	    ReleaseDC(NULL, hdc);
	    wdev->nColors = 16;
	    break;
	case 1:
	    wdev->color_info = win_mono_color;
	    wdev->nColors = 2;
	    break;
	default:
	    return (gs_error_rangecheck);
    }
    wdev->BitsPerPixel = bpp;

    /* If necessary, allocate and clear the mapped color flags. */
    if (bpp == 8) {
	if (wdev->mapped_color_flags == 0) {
	    wdev->mapped_color_flags = gs_malloc(4096, 1, "win_set_bits_per_pixel");
	    if (wdev->mapped_color_flags == 0)
		return_error(gs_error_VMerror);
	}
	memset(wdev->mapped_color_flags, 0, 4096);
    } else {
	gs_free(wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
	wdev->mapped_color_flags = 0;
    }
    /* restore old anti_alias info */
    wdev->color_info.anti_alias = anti_alias;
    return 0;
}
