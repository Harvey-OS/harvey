/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevx.c */
/* X Windows driver for Ghostscript library */
/* The X include files include <sys/types.h>, which, on some machines */
/* at least, define uint, ushort, and ulong, which std.h also defines. */
/* std.h has taken care of this. */
#include "gx.h"			/* for gx_bitmap; includes std.h */
#include "math_.h"
#include "memory_.h"
#include "x_.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gdevx.h"

/* Define whether to update after every write, for debugging. */
#define ALWAYS_UPDATE 0

/* Define the maximum size of the temporary pixmap for copy_mono */
/* that we are willing to leave lying around in the server */
/* between uses.  (Assume 32-bit ints here!) */
private int max_temp_pixmap = 20000;

/* Forward references */
private int set_tile(P2(gx_device *, const gx_tile_bitmap *));
private void free_cp(P1(gx_device *));
/* Screen updating machinery */
#define update_init(dev)\
  ((gx_device_X *)(dev))->up_area = 0,\
  ((gx_device_X *)(dev))->up_count = 0
#define update_flush(dev)\
  if ( ((gx_device_X *)(dev))->up_area != 0 ) update_do_flush(dev)
private void update_do_flush(P1(gx_device *));
private void x_send_event(P2(gx_device *, Atom));

/* Procedures */

extern int gdev_x_open(P1(gx_device_X *));
private dev_proc_open_device(x_open);
private dev_proc_get_initial_matrix(x_get_initial_matrix);
private dev_proc_sync_output(x_sync);
private dev_proc_output_page(x_output_page);
private dev_proc_close_device(x_close);
private dev_proc_map_rgb_color(x_map_rgb_color);
private dev_proc_map_color_rgb(x_map_color_rgb);
private dev_proc_fill_rectangle(x_fill_rectangle);
private dev_proc_tile_rectangle(x_tile_rectangle);
private dev_proc_copy_mono(x_copy_mono);
private dev_proc_copy_color(x_copy_color);
private dev_proc_draw_line(x_draw_line);
private dev_proc_put_params(x_put_params);
dev_proc_get_xfont_procs(x_get_xfont_procs);

/* The device descriptor */
private gx_device_procs x_procs = {
	x_open,
	x_get_initial_matrix,
	x_sync,
	x_output_page,
	x_close,
	x_map_rgb_color,
	x_map_color_rgb,
	x_fill_rectangle,
	x_tile_rectangle,
	x_copy_mono,
	x_copy_color,
	x_draw_line,
	gx_default_get_bits,
	NULL,			/* get_params */
	x_put_params,
	NULL,
	x_get_xfont_procs
};

/* The instance is public. */
gx_device_X gs_x11_device = {
	std_device_color_body(gx_device_X, &x_procs, "x11",
	  FAKE_RES*85/10, FAKE_RES*11,	/* x and y extent (nominal) */
	  FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	  /*dci_color(*/24, 255, 256/*)*/),
	{ 0 },			/* std_procs */
	{ /* image */
	  0, 0,			/* width, height */
	  0, XYBitmap, NULL,	/* xoffset, format, data */
	  LSBFirst, 8,    	/* byte-order, bitmap-unit */
	  MSBFirst, 8, 1,	/* bitmap-bit-order, bitmap-pad, depth */
	  0, 1,			/* bytes_per_line, bits_per_pixel */
	  0, 0, 0,		/* red_mask, green_mask, blue_mask */
	  NULL,			/* *obdata */
	   { NULL,			/* *(*create_image)() */
	     NULL,			/* (*destroy_image)() */
	     NULL,			/* (*get_pixel)() */
	     NULL,			/* (*put_pixel)() */
	     NULL,			/* *(*sub_image)() */
	     NULL			/* (*add_pixel)() */
	   },
	},
	NULL, NULL,		/* dpy, scr */
				/* (connection not initialized) */
	NULL,			/* vinfo */
	(Colormap)None,		/* cmap */
	(Window)None,		/* win */
	NULL,			/* gc */
	(Window)None,		/* pwin */
	(Pixmap)0,		/* bpixmap */
	0,			/* ghostview */
	(Window)None,		/* mwin */
#if HaveStdCMap
	NULL,			/* std_cmap */
#endif
	{ identity_matrix_body },	/* initial matrix (filled in) */
	(Atom)0, (Atom)0, (Atom)0,	/* Atoms: NEXT, PAGE, DONE */
	 { 0, 0, 0, 0 }, 0, 0,	/* update, up_area, up_count */
	(Pixmap)0,		/* dest */
	0L, ~0L,		/* colors_or, colors_and */
	 { /* cp */
	   (Pixmap)0,		/* pixmap */
	   NULL,		/* gc */
	   -1, -1		/* raster, height */
	 },
	 { /* ht */
	   (Pixmap)None,		/* pixmap */
	   (Pixmap)None,		/* no_pixmap */
	   gx_no_bitmap_id,		/* id */
	   0, 0, 0,			/* width, height, raster */
	   0, 0				/* fore_c, back_c */
	 },
	GXcopy,			/* function */
	FillSolid,		/* fill_style */
	0,			/* font */
	0, 0,			/* back_color, fore_color */
	0, 0,			/* background, foreground */
	NULL,			/* dither_colors */
	0, 0,			/* color_mask, num_rgb */
	NULL, 0,		/* dynamic_colors, max_dynamic_colors */
	0, 0,			/* dynamic_size, dynamic_allocs */
	0, 0,			/* borderColor, borderWidth */
	NULL,			/* geometry */
	128, 5,			/* maxGrayRamp, maxRGBRamp */
	NULL,			/* palette */
	NULL, NULL, NULL,	/* regularFonts, symbolFonts, dingbatFonts */
	NULL, NULL, NULL,	/* regular_fonts, symbol_fonts, dingbat_fonts */
	1, 1,			/* useXFonts, useFontExtensions */
	1, 0,			/* useScalableFonts, logXFonts */
	0.0, 0.0,		/* xResolution, yResolution */
	1,			/* useBackingPixmap */
	1, 1,			/* useXPutImage, useXSetTile */
};

/* If XPutImage doesn't work, do it ourselves. */
private void alt_put_image(P11(gx_device *dev, Display *dpy, Drawable win,
  GC gc, XImage *pi, int sx, int sy, int dx, int dy, unsigned w, unsigned h));
#define put_image(dpy,win,gc,im,sx,sy,x,y,w,h)\
  if ( xdev->useXPutImage) XPutImage(dpy,win,gc,im,sx,sy,x,y,w,h);\
  else alt_put_image(dev,dpy,win,gc,im,sx,sy,x,y,w,h)


/* Open the device.  Most of the code is in gdevxini.c. */
private int
x_open(gx_device *dev)
{
    gx_device_X *xdev = (gx_device_X *)dev;
    int code = gdev_x_open(xdev);

    if (code < 0) return code;
    update_init(dev);
    return 0;
}

/* Close the device. */
private int
x_close(gx_device *dev)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    if (xdev->ghostview) x_send_event(dev, xdev->DONE);
    if (xdev->vinfo) {
	XFree((char *)xdev->vinfo);
	xdev->vinfo = NULL;
    }
    if (xdev->dither_colors) {
	if (gx_device_has_color(xdev))
#define cube(r) (r*r*r)
	    gs_free((char *)xdev->dither_colors, sizeof(x_pixel),
		    cube(xdev->color_info.dither_colors), "x11_rgb_cube");
#undef cube
	else
	    gs_free((char *)xdev->dither_colors, sizeof(x_pixel),
		    xdev->color_info.dither_grays, "x11_gray_ramp");
	xdev->dither_colors = NULL;
    }
    if (xdev->dynamic_colors) {
	int i;
	for (i = 0; i < xdev->dynamic_size; i++) {
	    x11color *xcp = (*xdev->dynamic_colors)[i];
	    x11color *next;
	    while (xcp) {
		next = xcp->next;
		gs_free((char *)xcp, sizeof(x11color), 1, "x11_dynamic_color");
		xcp = next;
	    }
	}
	gs_free((char *)xdev->dynamic_colors, sizeof(x11color *),
		xdev->dynamic_size, "x11_dynamic_colors");
	xdev->dynamic_colors = NULL;
    }
    while (xdev->regular_fonts) {
	x11fontmap *font = xdev->regular_fonts;
	xdev->regular_fonts = font->next;
	if (font->std_names) XFreeFontNames(font->std_names);
	if (font->iso_names) XFreeFontNames(font->iso_names);
	gs_free(font->x11_name, sizeof(char), strlen(font->x11_name)+1,
		"x11_font_x11name");
	gs_free(font->ps_name, sizeof(char), strlen(font->ps_name)+1,
		"x11_font_psname");
	gs_free((char *)font, sizeof(x11fontmap), 1, "x11_fontmap");
    }
    while (xdev->symbol_fonts) {
	x11fontmap *font = xdev->symbol_fonts;
	xdev->symbol_fonts = font->next;
	if (font->std_names) XFreeFontNames(font->std_names);
	if (font->iso_names) XFreeFontNames(font->iso_names);
	gs_free(font->x11_name, sizeof(char), strlen(font->x11_name)+1,
		"x11_font_x11name");
	gs_free(font->ps_name, sizeof(char), strlen(font->ps_name)+1,
		"x11_font_psname");
	gs_free((char *)font, sizeof(x11fontmap), 1, "x11_fontmap");
    }
    while (xdev->dingbat_fonts) {
	x11fontmap *font = xdev->dingbat_fonts;
	xdev->dingbat_fonts = font->next;
	if (font->std_names) XFreeFontNames(font->std_names);
	if (font->iso_names) XFreeFontNames(font->iso_names);
	gs_free(font->x11_name, sizeof(char), strlen(font->x11_name)+1,
		"x11_font_x11name");
	gs_free(font->ps_name, sizeof(char), strlen(font->ps_name)+1,
		"x11_font_psname");
	gs_free((char *)font, sizeof(x11fontmap), 1, "x11_fontmap");
    }
    XCloseDisplay(xdev->dpy);
    return 0;
}

/* Map a color.  The "device colors" are just r,g,b packed together. */
private gx_color_index
x_map_rgb_color(register gx_device *dev,
		gx_color_value r, gx_color_value g, gx_color_value b)
{
    gx_device_X *xdev = (gx_device_X *)dev;
    /* X and ghostscript both use shorts for color values */
    unsigned short dr = r & xdev->color_mask;	/* Nearest color that */
    unsigned short dg = g & xdev->color_mask;	/* the X device can   */
    unsigned short db = b & xdev->color_mask;	/* represent          */
    unsigned short cv_max = X_max_color_value & xdev->color_mask;
    int cv_denom = (gx_max_color_value + 1);

    /* foreground and background get special treatment */
    /* They maybe mapped to other colors. */
    if (dr == 0 && dg == 0 && db == 0) {
	return xdev->foreground;
    }
    if (dr == cv_max && dg == cv_max && db == cv_max) {
	return xdev->background;
    }
#if HaveStdCMap
    /* check the standard colormap first */
    if (xdev->std_cmap) {
	XStandardColormap *cmap = xdev->std_cmap;

	if (gx_device_has_color(xdev)) {
	    unsigned short cr, cg, cb;		/* rgb cube indices */
	    unsigned short cvr, cvg, cvb;	/* color value on cube */

	    cr = r * (cmap->red_max + 1) / cv_denom;
	    cg = g * (cmap->green_max + 1) / cv_denom;
	    cb = b * (cmap->blue_max + 1) / cv_denom;
	    cvr = X_max_color_value * cr / cmap->red_max;
	    cvg = X_max_color_value * cg / cmap->green_max;
	    cvb = X_max_color_value * cb / cmap->blue_max;
	    if ((abs((int)r - (int)cvr) & xdev->color_mask) == 0 &&
		(abs((int)g - (int)cvg) & xdev->color_mask) == 0 &&
		(abs((int)b - (int)cvb) & xdev->color_mask) == 0)
		return cr * cmap->red_mult + cg * cmap->green_mult +
		       cb * cmap->blue_mult + cmap->base_pixel;
	} else {
	    unsigned short cr;
	    unsigned short cvr;
	    int dither_grays = xdev->color_info.dither_grays;

	    cr = r * dither_grays / cv_denom;
	    cvr = X_max_color_value * cr / cmap->red_max;
	    if ((abs((int)r - (int)cvr) & xdev->color_mask) == 0)
		return cr * cmap->red_mult + cmap->base_pixel;
	}
    } else
#endif
    /* If there is no standard colormap, check the dither cube/ramp */
    if (xdev->dither_colors) {
	if (gx_device_has_color(xdev)) {
	    unsigned short cr, cg, cb;		/* rgb cube indices */
	    unsigned short cvr, cvg, cvb;	/* color value on cube */
	    int dither_rgb = xdev->color_info.dither_colors;
	    unsigned short max_rgb = dither_rgb - 1;

	    cr = r * dither_rgb / cv_denom;
	    cg = g * dither_rgb / cv_denom;
	    cb = b * dither_rgb / cv_denom;
	    cvr = X_max_color_value * cr / max_rgb;
	    cvg = X_max_color_value * cg / max_rgb;
	    cvb = X_max_color_value * cb / max_rgb;
	    if ((abs((int)r - (int)cvr) & xdev->color_mask) == 0 &&
		(abs((int)g - (int)cvg) & xdev->color_mask) == 0 &&
		(abs((int)b - (int)cvb) & xdev->color_mask) == 0) {
		return xdev->dither_colors[cube_index(cr, cg, cb)];
	    }
	} else {
	    unsigned short cr;
	    unsigned short cvr;
	    int dither_grays = xdev->color_info.dither_grays;
	    unsigned short max_gray = dither_grays - 1;

	    cr = r * dither_grays / cv_denom;
	    cvr = (X_max_color_value * cr / max_gray);
	    if ((abs((int)r - (int)cvr) & xdev->color_mask) == 0)
		return xdev->dither_colors[cr];
	}
    }
    /* Finally look through the list of dynamic colors */
    if (xdev->dynamic_colors) {
	int i = (dr ^ dg ^ db) >> (16 - xdev->vinfo->bits_per_rgb);
	x11color *xcp = (*xdev->dynamic_colors)[i];
	x11color *last = NULL;
	XColor xc;

	while (xcp) {
	    if (xcp->color.red == dr && xcp->color.green == dg &&
		xcp->color.blue == db) {
		if (last) {
		    last->next = xcp->next;
		    xcp->next = (*xdev->dynamic_colors)[i];
		    (*xdev->dynamic_colors)[i] = xcp;
		}
		if (xcp->color.pad) return xcp->color.pixel;
		else return gx_no_color_index;
	    }
	    last = xcp;
	    xcp = xcp->next;
	}

	/* If not in our list of dynamic colors, */
	/* ask the X server and add an entry. */
	/* First check if dynamic table is exhausted */
	if (xdev->dynamic_allocs > xdev->max_dynamic_colors)
	    return gx_no_color_index;
	xcp = (x11color *) gs_malloc(sizeof(x11color), 1, "x11_dynamic_color");
	if (!xcp) return gx_no_color_index;
	xc.red   = xcp->color.red   = dr;
	xc.green = xcp->color.green = dg;
	xc.blue  = xcp->color.blue  = db;
	xcp->next = (*xdev->dynamic_colors)[i];
	(*xdev->dynamic_colors)[i] = xcp;
	xdev->dynamic_allocs++;
	if (XAllocColor(xdev->dpy, xdev->cmap, &xc)) {
	    xcp->color.pixel = xc.pixel;
	    xcp->color.pad = True;
	    return xc.pixel;
	} else {
	    xcp->color.pad = False;
	    return gx_no_color_index;
	}
    }
    return gx_no_color_index;
}


/* Map a "device color" back to r-g-b. */
/* This doesn't happen often, so we just ask the display */
/* Foreground and background may be mapped to other colors, so */
/* they are handled specially. */
private int
x_map_color_rgb(register gx_device *dev, gx_color_index color,
		gx_color_value prgb[3])
{
    gx_device_X *xdev = (gx_device_X *)dev;

    if (color == xdev->foreground)
	prgb[0] = prgb[1] = prgb[2] = 0;
    else if (color == xdev->background)
	prgb[0] = prgb[1] = prgb[2] = gx_max_color_value;
    else {
	XColor xc;

	xc.pixel = color;
	XQueryColor(xdev->dpy, xdev->cmap, &xc);
	prgb[0] = xc.red;
	prgb[1] = xc.green;
	prgb[2] = xc.blue;
    }
    return 0;
}

/* Get initial matrix for X device */
private void
x_get_initial_matrix(register gx_device *dev, register gs_matrix *pmat)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    pmat->xx = xdev->initial_matrix.xx;
    pmat->xy = xdev->initial_matrix.xy;
    pmat->yx = xdev->initial_matrix.yx;
    pmat->yy = xdev->initial_matrix.yy;
    pmat->tx = xdev->initial_matrix.tx;
    pmat->ty = xdev->initial_matrix.ty;
}

/* Synchronize the display with the commands already given */
private int
x_sync(register gx_device *dev)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    update_flush(dev);
    XFlush(xdev->dpy);
    return 0;
}

/* Send event to ghostview process */
private void
x_send_event(gx_device *dev, Atom msg)
{
    gx_device_X *xdev = (gx_device_X *)dev;
    XEvent event;

    event.xclient.type = ClientMessage;
    event.xclient.display = xdev->dpy;
    event.xclient.window = xdev->win;
    event.xclient.message_type = msg;
    event.xclient.format = 32;
    event.xclient.data.l[0] = xdev->mwin;
    event.xclient.data.l[1] = xdev->dest;
    XSendEvent(xdev->dpy, xdev->win, False, 0, &event);
}

/* Output "page" */
private int
x_output_page(gx_device *dev, int num_copies, int flush)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    x_sync(dev);

    /* Send ghostview a "page" client event */
    /* Wait for a "next" client event */
    if (xdev->ghostview) {
	XEvent event;

	x_send_event(dev, xdev->PAGE);
	XNextEvent(xdev->dpy, &event);
	while (event.type != ClientMessage ||
	       event.xclient.message_type != xdev->NEXT) {
	    XNextEvent(xdev->dpy, &event);
	}
    }
    return 0;
}

/* Fill a rectangle with a color. */
private int
x_fill_rectangle(register gx_device *dev,
		 int x, int y, int w, int h, gx_color_index color)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    fit_fill(dev, x, y, w, h);
    set_fill_style(FillSolid);
    set_fore_color(color);
    set_function(GXcopy);
    XFillRectangle(xdev->dpy, xdev->dest, xdev->gc, x, y, w, h);
    /* If we are filling the entire screen, reset */
    /* colors_or and colors_and.  It's wasteful to do this */
    /* on every operation, but there's no separate driver routine */
    /* for erasepage (yet). */
    if (x == 0 && y == 0 && w == xdev->width && h == xdev->height) {
	if (color == xdev->foreground || color == xdev->background) {
	    if (xdev->dynamic_colors) {
		int i;
		for (i = 0; i < xdev->dynamic_size; i++) {
		    x11color *xcp = (*xdev->dynamic_colors)[i];
		    x11color *next;
		    while (xcp) {
			next = xcp->next;
			if (xcp->color.pad) {
			    XFreeColors(xdev->dpy, xdev->cmap,
					&xcp->color.pixel, 1, 0);
			}
			gs_free((char *)xcp, sizeof(x11color), 1,
				"x11_dynamic_color");
			xcp = next;
		    }
		    (*xdev->dynamic_colors)[i] = NULL;
		}
		xdev->dynamic_allocs = 0;
	    }
	}
	xdev->colors_or = xdev->colors_and = color;
    }
    if (xdev->bpixmap != (Pixmap) 0) {
	x_update_add(dev, x, y, w, h);
    }
#ifdef DEBUG
    if (gs_debug['F'])
	dprintf5("[F] fill (%d,%d):(%d,%d) %ld\n",
		 x, y, w, h, (long)color);
#endif
    return 0;
}

/* Tile a rectangle. */
private int
x_tile_rectangle(register gx_device *dev, const gx_tile_bitmap *tile,
	  	 int x, int y, int w, int h,
		 gx_color_index zero, gx_color_index one,
		 int px, int py)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    fit_fill(dev, x, y, w, h);

    /* Give up if one color is transparent, or if the tile is colored. */
    /* We should implement the latter someday, since X can handle it. */

    if (one == gx_no_color_index || zero == gx_no_color_index)
      return gx_default_tile_rectangle(dev, tile, x, y, w, h,
				       zero, one, px, py);

    /* For the moment, give up if the phase is non-zero. */
    if (px | py)
      return gx_default_tile_rectangle(dev, tile, x, y, w, h,
				       zero, one, px, py);

    /* Imaging with a halftone often gives rise to */
    /* tile_rectangle calls for a single pixel.  Check for this now. */

    if ( h == 1 && w == 1 )
      {	uint tx = x % tile->rep_width;
	const byte *ptr =
	  tile->data + (y % tile->rep_height) * tile->raster + (tx >> 3);
	byte mask = 0x80 >> (tx & 7);
	x_pixel pixel = (*ptr & mask ? one : zero);
	set_fill_style(FillSolid);
	set_function(GXcopy);
	set_fore_color(pixel);
	XDrawPoint(xdev->dpy, xdev->dest, xdev->gc, x, y);
	if (xdev->bpixmap != (Pixmap) 0) {
	  x_update_add(dev, x, y, w, h);
	}
	return 0;
      }

    /*
     * Remember, an X tile is already filled with particular
     * pixel values (i.e., colors).  Therefore if we are changing
     * fore/background color, we must invalidate the tile (using
     * the same technique as in set_tile).  This problem only
     * bites when using grayscale -- you may want to change
     * fg/bg but use the same halftone screen.
     */
    if ((zero != xdev->ht.back_c) || (one != xdev->ht.fore_c))
	xdev->ht.id = ~tile->id;	/* force reload */

    set_back_color(zero);
    set_fore_color(one);
    if (!set_tile(dev, tile))
      {	/* Bad news.  Fall back to the default algorithm. */
	return gx_default_tile_rectangle(dev, tile, x, y, w, h,
					 zero, one, px, py);
      }
    /* Use the tile to fill the rectangle */
    set_fill_style(FillTiled);
    set_function(GXcopy);
    XFillRectangle(xdev->dpy, xdev->dest, xdev->gc, x, y, w, h);
    if (xdev->bpixmap != (Pixmap) 0) {
      x_update_add(dev, x, y, w, h);
    }
#ifdef DEBUG
    if (gs_debug['F'])
	dprintf6("[F] tile (%d,%d):(%d,%d) %ld,%ld\n",
		 x, y, w, h, (long)zero, (long)one);
#endif
    return 0;
}

/* Set up with a specified tile. */
/* Return false if we can't do it for some reason. */
private int
set_tile(register gx_device *dev, register const gx_tile_bitmap *tile)
{
    gx_device_X *xdev = (gx_device_X *)dev;

#ifdef DEBUG
    if (gs_debug['T'])
	return 0;
#endif
    if (tile->id == xdev->ht.id && tile->id != gx_no_bitmap_id)
	return xdev->useXSetTile;
    /* Set up the tile Pixmap */
    if (tile->size.x != xdev->ht.width ||
	tile->size.y != xdev->ht.height ||
	xdev->ht.pixmap == (Pixmap) 0) {
	if (xdev->ht.pixmap != (Pixmap) 0)
	    XFreePixmap(xdev->dpy, xdev->ht.pixmap);
	xdev->ht.pixmap = XCreatePixmap(xdev->dpy, xdev->win,
					tile->size.x, tile->size.y,
					xdev->vinfo->depth);
	if (xdev->ht.pixmap == (Pixmap) 0)
	    return 0;
	xdev->ht.width = tile->size.x, xdev->ht.height = tile->size.y;
	xdev->ht.raster = tile->raster;
    }
    xdev->ht.fore_c = xdev->fore_color;
    xdev->ht.back_c = xdev->back_color;
    /* Copy the tile into the Pixmap */
    xdev->image.data = (char *)tile->data;
    xdev->image.width = tile->size.x;
    xdev->image.height = tile->size.y;
    xdev->image.bytes_per_line = tile->raster;
    xdev->image.format = XYBitmap;
    set_fill_style(FillSolid);
#ifdef DEBUG
    if (gs_debug['H']) {
	int i;

	dprintf4("[H] 0x%lx: width=%d height=%d raster=%d\n",
		 (ulong)tile->data, tile->size.x, tile->size.y, tile->raster);
	for (i = 0; i < tile->raster * tile->size.y; i++)
	    dprintf1(" %02x", tile->data[i]);
	dputc('\n');
    }
#endif
    XSetTile(xdev->dpy, xdev->gc, xdev->ht.no_pixmap);	/* *** X bug *** */
    set_function(GXcopy);
    put_image(xdev->dpy, xdev->ht.pixmap, xdev->gc, &xdev->image,
	      0, 0, 0, 0, tile->size.x, tile->size.y);
    XSetTile(xdev->dpy, xdev->gc, xdev->ht.pixmap);
    xdev->ht.id = tile->id;
    return xdev->useXSetTile;
}

/* Copy a monochrome bitmap. */
private int
x_copy_mono(register gx_device *dev,
	    const byte *base, int sourcex, int raster, gx_bitmap_id id,
	    int x, int y, int w, int h,
	    gx_color_index zero, gx_color_index one)
/*
 * X doesn't directly support the simple operation of writing a color
 * through a mask specified by an image.  The plot is the following:
 *  If neither color is gx_no_color_index ("transparent"),
 *	use XPutImage with the "copy" function as usual.
 *  If the color either bitwise-includes or is bitwise-included-in
 *	every color written to date
 *	(a special optimization for writing black/white on color displays),
 *	use XPutImage with an appropriate Boolean function.
 *  Otherwise, do the following complicated stuff:
 *	Create pixmap of depth 1 if necessary.
 *	If foreground color is "transparent" then
 *	  invert the raster data.
 *	Use XPutImage to copy the raster image to the newly
 *	  created Pixmap.
 *	Install the Pixmap as the clip_mask in the X GC and
 *	  tweak the clip origin.
 *	Do an XFillRectangle, fill style=solid, specifying a
 *	  rectangle the same size as the original raster data.
 *	De-install the clip_mask.
 */
{
    gx_device_X *xdev = (gx_device_X *)dev;
    int function = GXcopy;

    x_pixel
	bc = zero,
	fc = one;

    fit_copy(dev, base, sourcex, raster, id, x, y, w, h);

    xdev->image.width = raster << 3;
    xdev->image.height = h;
    xdev->image.data = (char *)base;
    xdev->image.bytes_per_line = raster;
    set_fill_style(FillSolid);

    /* Check for null, easy 1-color, hard 1-color, and 2-color cases. */
    if (zero != gx_no_color_index) {
	if (one != gx_no_color_index) {
	    /* 2-color case. */
	    /* Simply replace existing bits with what's in the image. */
	} else if (!(~xdev->colors_and & bc)) {
	    function = GXand;
	    fc = ~(x_pixel) 0;
	} else if (!(~bc & xdev->colors_or)) {
	    function = GXor;
	    fc = 0;
	} else {
	    goto hard;
	}
    } else {
	if (one == gx_no_color_index) {	/* no-op */
	    return 0;
	} else if (!(~xdev->colors_and & fc)) {
	    function = GXand;
	    bc = ~(x_pixel) 0;
	} else if (!(~fc & xdev->colors_or)) {
	    function = GXor;
	    bc = 0;
	} else {
	    goto hard;
	}
    }
    xdev->image.format = XYBitmap;
    set_function(function);
    if (bc != xdev->back_color) {
	XSetBackground(xdev->dpy, xdev->gc, (xdev->back_color = bc));
    }
    if (fc != xdev->fore_color) {
	XSetForeground(xdev->dpy, xdev->gc, (xdev->fore_color = fc));
    }
    if (zero != gx_no_color_index)
	note_color(zero);
    if (one != gx_no_color_index)
	note_color(one);
    put_image(xdev->dpy, xdev->dest, xdev->gc, &xdev->image,
	      sourcex, 0, x, y, w, h);

    goto out;

hard:	/* Handle the hard 1-color case. */
    if (raster > xdev->cp.raster || h > xdev->cp.height) {
	/* Must allocate a new pixmap and GC. */
	/* Release the old ones first. */
	free_cp(dev);

	/* Create the clipping pixmap, depth must be 1. */
	xdev->cp.pixmap =
	    XCreatePixmap(xdev->dpy, xdev->win, raster << 3, h, 1);
	if (xdev->cp.pixmap == (Pixmap) 0) {
	    lprintf("x_copy_mono: can't allocate pixmap\n");
	    exit(1);
	}
	xdev->cp.gc = XCreateGC(xdev->dpy, xdev->cp.pixmap, 0, 0);
	if (xdev->cp.gc == (GC) 0) {
	    lprintf("x_copy_mono: can't allocate GC\n");
	    exit(1);
	}
	xdev->cp.raster = raster;
	xdev->cp.height = h;
    }
    /* Initialize static mask image params */
    xdev->image.format = XYBitmap;
    set_function(GXcopy);

    /* Select polarity based on fg/bg transparency. */
    if (one == gx_no_color_index) {	/* invert */
	XSetBackground(xdev->dpy, xdev->cp.gc, (x_pixel) 1);
	XSetForeground(xdev->dpy, xdev->cp.gc, (x_pixel) 0);
	set_fore_color(zero);
    } else {
	XSetBackground(xdev->dpy, xdev->cp.gc, (x_pixel) 0);
	XSetForeground(xdev->dpy, xdev->cp.gc, (x_pixel) 1);
	set_fore_color(one);
    }
    put_image(xdev->dpy, xdev->cp.pixmap, xdev->cp.gc,
	      &xdev->image, sourcex, 0, 0, 0, w, h);

    /* Install as clipmask. */
    XSetClipMask(xdev->dpy, xdev->gc, xdev->cp.pixmap);
    XSetClipOrigin(xdev->dpy, xdev->gc, x, y);

    /*
     * Draw a solid rectangle through the raster clip mask.
     * Note fill style is guaranteed to be solid from above.
     */
    XFillRectangle(xdev->dpy, xdev->dest, xdev->gc, x, y, w, h);

    /* Tidy up.  Free the pixmap if it's big. */
    XSetClipMask(xdev->dpy, xdev->gc, None);
    if (raster * h > max_temp_pixmap)
	free_cp(dev);

out:if (xdev->bpixmap != (Pixmap) 0) {
	/* We wrote to the pixmap, so update the display now. */
	x_update_add(dev, x, y, w, h);
    }
    return 0;
}

/* Internal routine to free the GC and pixmap used for copying. */
private void
free_cp(register gx_device *dev)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    if (xdev->cp.gc != NULL) {
	XFreeGC(xdev->dpy, xdev->cp.gc);
	xdev->cp.gc = NULL;
    }
    if (xdev->cp.pixmap != (Pixmap) 0) {
	XFreePixmap(xdev->dpy, xdev->cp.pixmap);
	xdev->cp.pixmap = (Pixmap) 0;
    }
    xdev->cp.raster = -1;	/* mark as unallocated */
}

/* Copy a color bitmap. */
private int
x_copy_color(register gx_device *dev,
	     const byte *base, int sourcex, int raster, gx_bitmap_id id,
	     int x, int y, int w, int h)
{	gx_device_X *xdev = (gx_device_X *)dev;
	int depth = dev->color_info.depth;

	fit_copy(dev, base, sourcex, raster, id, x, y, w, h);
	set_fill_style(FillSolid);
	set_function(GXcopy);

	/* Filling with a colored halftone often gives rise to */
	/* copy_color calls for a single pixel.  Check for this now. */

	if ( h == 1 && w == 1 )
	  {	uint sbit = sourcex * depth;
		const byte *ptr = base + (sbit >> 3);
		x_pixel pixel;
		if ( depth < 8 )
		  pixel = (byte)(*ptr << (sbit & 7)) >> (8 - depth);
		else
		  { pixel = *ptr++;
		    while ( (depth -= 8) > 0 )
		      pixel = (pixel << 8) + *ptr++;
		  }
		set_fore_color(pixel);
		XDrawPoint(xdev->dpy, xdev->dest, xdev->gc, x, y);
	  }
	else
	  {    xdev->image.width = raster << 3;
	       xdev->image.height = h;
	       xdev->image.format = ZPixmap;
	       xdev->image.data = (char *)base;
	       xdev->image.depth = depth;
	       xdev->image.bytes_per_line = raster;
	       xdev->image.bits_per_pixel = depth;
	       XPutImage(xdev->dpy, xdev->dest, xdev->gc, &xdev->image,
			 sourcex, 0, x, y, w, h);
	       xdev->image.depth = xdev->image.bits_per_pixel = 1;
	  }
	if (xdev->bpixmap != (Pixmap) 0)
	  x_update_add(dev, x, y, w, h);
#ifdef DEBUG
	if (gs_debug['F'])
	  dprintf4("[F] copy_color (%d,%d):(%d,%d)\n",
		   x, y, w, h);
#endif
	return 0;
}

/* Draw a line */
private int
x_draw_line(register gx_device *dev,
	    int x0, int y0, int x1, int y1, gx_color_index color)
{
    gx_device_X *xdev = (gx_device_X *)dev;

    set_fore_color(color);
    set_fill_style(FillSolid);
    set_function(GXcopy);
    XDrawLine(xdev->dpy, xdev->dest, xdev->gc, x0, y0, x1, y1);
    if (xdev->bpixmap != (Pixmap) 0) {
	int x = x0, y = y0, w = x1 - x0, h = y1 - y0;

	if (w < 0) x = x1, w = -w;
	if (h < 0) y = y1, h = -h;
	w++;
	h++;
	fit_fill(dev, x, y, w, h);
	x_update_add(dev, x, y, w, h);
    }
    return 0;
}

/* Set the device parameters.  We reimplement this so we can resize */
/* the window and avoid closing and reopening the device. */
private int
x_put_params(gx_device *dev, gs_param_list *plist)
{	gx_device_X *xdev = (gx_device_X *)dev;
	bool is_open = dev->is_open;
	int width = dev->width;
	int height = dev->height;
	long pwin = (long)xdev->pwin;
	int ecode = 0, code;

	/* Handle extra parameters */
	switch ( code = param_read_long(plist, "WindowID", &pwin) )
	{
	case 0:
	case 1:
		break;
	default:
		ecode = code;
		param_signal_error(plist, "GSVIEW", ecode);
	}

	if ( ecode < 0 )
	  return ecode;

	/* Unless we specified a new window ID, */
	/* prevent gx_default_put_params from closing the device. */
	if ( pwin == (long)xdev->pwin )
	  dev->is_open = false;
	code = gx_default_put_params(dev, plist);
	dev->is_open = is_open;
	if ( code < 0 )
	  return code;

	if ( pwin != (long)xdev->pwin )
	  {	if ( xdev->is_open )
		  gs_closedevice(dev);
		xdev->pwin = (Window)pwin;
	  }

	/* If the device is open, resize the window. */
	/* Don't do this if Ghostview is active. */
	if ( xdev->is_open && !xdev->ghostview &&
	      (dev->width != width || dev->height != height)
	   )
	  {	XResizeWindow(xdev->dpy, xdev->win, dev->width, dev->height);
		if (xdev->bpixmap != (Pixmap) 0)
		  {	XFreePixmap(xdev->dpy, xdev->bpixmap);
			xdev->bpixmap = (Pixmap) 0;
		  }
		xdev->dest = 0;
		gdev_x_clear_window(xdev);
	  }

	return 0;
}


/* ------ Screen update procedures ------ */

/* Flush updates to the screen if needed. */
private void
update_do_flush(register gx_device *dev)
{
    gx_device_X *xdev = (gx_device_X *)dev;
    int xo = xdev->update.xo, yo = xdev->update.yo;

    set_function(GXcopy);
    XCopyArea(xdev->dpy, xdev->bpixmap, xdev->win, xdev->gc,
	      xo, yo, xdev->update.xe - xo, xdev->update.ye - yo,
	      xo, yo);
    update_init(dev);
}

/* Add a region to be updated. */
/* This is only called if xdev->bpixmap != 0. */
void
x_update_add(gx_device *dev, int xo, int yo, int w, int h)
{
	register gx_device_X *xdev = (gx_device_X *)dev;
	int xe = xo + w, ye = yo + h;
	long new_area;

#if ALWAYS_UPDATE		/**************** ****************/

	update_do_flush(dev);
	new_area = (long)w * h;

#else			/**************** ****************/

	if ( ++xdev->up_count >= 200 || xdev->up_area == 0 )
	  {	if ( xdev->up_area != 0 )
		  update_do_flush(dev);
		new_area = (long)w * h;
	  }
	else
	  {	/* See whether adding this rectangle */
		/* would result in too much being copied unnecessarily. */
		long old_area = xdev->up_area;
		long new_up_area;
		rect u;

		u.xo = min(xo, xdev->update.xo);
		u.yo = min(yo, xdev->update.yo);
		u.xe = max(xe, xdev->update.xe);
		u.ye = max(ye, xdev->update.ye);
		new_up_area = (long)(u.xe - u.xo) * (u.ye - u.yo);
		/* The fraction of new_up_area used in the following test */
		/* is not particularly critical; using a denominator */
		/* that is a power of 2 eliminates a divide. */
		if ( new_up_area > 100 &&
		     old_area + (new_area = (long)w * h) <
		      new_up_area - (new_up_area >> 2)
		   )
		  update_do_flush(dev);
		else
		  {	xdev->update = u;
			xdev->up_area = new_up_area;
			return;
		  }
	  }

#endif			/**************** ****************/

	xdev->update.xo = xo;
	xdev->update.yo = yo;
	xdev->update.xe = xe;
	xdev->update.ye = ye;
	xdev->up_area = new_area;
}

/* ------ Internal procedures ------ */

/* Substitute for XPutImage using XFillRectangle. */
/* This is a total hack to get around an apparent bug */
/* in some X servers.  It only works with the specific */
/* parameters (bit/byte order, padding) used above. */
private void
alt_put_image(gx_device *dev, Display *dpy, Drawable win, GC gc,
	  XImage *pi, int sx, int sy, int dx, int dy, unsigned w, unsigned h)
{
    int raster = pi->bytes_per_line;
    byte *data = (byte *) pi->data + sy * raster + (sx >> 3);
    int init_mask = 0x80 >> (sx & 7);
    int invert = 0;
    int yi;

#define nrects 40
    XRectangle rects[nrects];
    XRectangle *rp = rects;

    XGCValues gcv;

    XGetGCValues(dpy, gc, (GCFunction | GCForeground | GCBackground), &gcv);

    if (gcv.function == GXcopy) {
	XSetForeground(dpy, gc, gcv.background);
	XFillRectangle(dpy, win, gc, dx, dy, w, h);
	XSetForeground(dpy, gc, gcv.foreground);
    } else if (gcv.function == GXand) {
	if (gcv.background != ~(x_pixel) 0) {
	    XSetForeground(dpy, gc, gcv.background);
	    invert = 0xff;
	}
    } else if (gcv.function == GXor) {
	if (gcv.background != 0) {
	    XSetForeground(dpy, gc, gcv.background);
	    invert = 0xff;
	}
    } else {
	lprintf("alt_put_image: unimplemented function.\n");
	exit(1);
    }

    for (yi = 0; yi < h; yi++, data += raster) {
	register int mask = init_mask;
	register byte *dp = data;
	register int xi = 0;

	while (xi < w) {
	    if ((*dp ^ invert) & mask) {
		int xleft = xi;

		if (rp == &rects[nrects]) {
		    XFillRectangles(dpy, win, gc, rects, nrects);
		    rp = rects;
		}
		/* Scan over a run of 1-bits */
		rp->x = dx + xi, rp->y = dy + yi;
		do {
		    if (!(mask >>= 1))
			mask = 0x80, dp++;
		    xi++;
		} while (xi < w && (*dp & mask));
		rp->width = xi - xleft, rp->height = 1;
		rp++;
	    } else {
		if (!(mask >>= 1))
		    mask = 0x80, dp++;
		xi++;
	    }
	}
    }
    XFillRectangles(dpy, win, gc, rects, rp - rects);
    if (invert) XSetForeground(dpy, gc, gcv.foreground);
}
