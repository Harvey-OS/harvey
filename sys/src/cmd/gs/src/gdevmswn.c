/* Copyright (C) 1989, 1995, 1996, 1998, 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmswn.c,v 1.12 2004/09/20 22:14:59 dan Exp $ */
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

/* Forward references */
private int win_set_bits_per_pixel(gx_device_win *, int);

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
    if (pgsdll_callback)
	(*pgsdll_callback) (GSDLL_SYNC, (unsigned char *)wdev, 0);
    return (0);
}

/* Make the window visible, and display the output. */
int
win_output_page(gx_device * dev, int copies, int flush)
{
    if (pgsdll_callback)
	(*pgsdll_callback) (GSDLL_PAGE, (unsigned char *)wdev, 0);
    return gx_finish_output_page(dev, copies, flush);;
}

/* Close the win driver */
int
win_close(gx_device * dev)
{
    /* Free resources */
    if (wdev->nColors > 0) {
	gs_free(dev->memory, 
		wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
	DeleteObject(wdev->himgpalette);
	gs_free(dev->memory, 
		(char *)(wdev->limgpalette), 1, sizeof(LOGPALETTE) +
		(1 << (wdev->color_info.depth)) * sizeof(PALETTEENTRY),
		"win_close");
    }
    return (0);
}

/* Map a r-g-b color to the colors available under Windows */
gx_color_index
win_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_color_value r = cv[0];
    gx_color_value g = cv[1];
    gx_color_value b = cv[2];
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
	    return pc_4bit_map_rgb_color(dev, cv);
    }
    return (gx_default_map_rgb_color(dev, cv));
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
	    gs_free(wdev->memory,
		    wdev->mapped_color_flags, 4096, 1,
		    "win_put_params");
	wdev->mapped_color_flags = old_flags;
	if (bpp != old_bpp)
	    win_set_bits_per_pixel(wdev, old_bpp);
	return ecode;
    }
    if (wdev->mapped_color_flags == 0 && old_flags != 0) {	/* Release old mapped_color_flags. */
	gs_free(dev->memory,
		old_flags, 4096, 1, "win_put_params");
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

    logpalette = (LPLOGPALETTE) gs_malloc(wdev->memory, 1, sizeof(LOGPALETTE) +
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
	    wdev->mapped_color_flags = gs_malloc(wdev->memory,
						 4096, 1, "win_set_bits_per_pixel");
	    if (wdev->mapped_color_flags == 0)
		return_error(gs_error_VMerror);
	}
	memset(wdev->mapped_color_flags, 0, 4096);
    } else {
	gs_free(wdev->memory, 
		wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
	wdev->mapped_color_flags = 0;
    }

    /* copy encode/decode procedures */
    wdev->procs.encode_color = wdev->procs.map_rgb_color;
    wdev->procs.decode_color = wdev->procs.map_color_rgb;
    if (bpp == 1) {
	wdev->procs.get_color_mapping_procs = 
	    gx_default_DevGray_get_color_mapping_procs;
	wdev->procs.get_color_comp_index = 
	    gx_default_DevGray_get_color_comp_index;
    }
    else {
	wdev->procs.get_color_mapping_procs = 
	    gx_default_DevRGB_get_color_mapping_procs;
	wdev->procs.get_color_comp_index = 
	    gx_default_DevRGB_get_color_comp_index;
    }

    /* restore old anti_alias info */
    wdev->color_info.anti_alias = anti_alias;
    return 0;
}
