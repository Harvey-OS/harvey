/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevpbm.c */
/* Portable Bit/Gray/PixMap drivers */
#include "gdevprn.h"
#include "gscdefs.h"
#include "gxlum.h"

/* Thanks are due to Jos Vos (jos@bull.nl) for an earlier P*M driver, */
/* on which this one is based. */

/*
 * The PGM and PPM drivers have an "optimize" flag, which causes them to
 * fall back to PBM or (for PPM) PGM if the page can be represented that way.
 * This should be a device property, but for now, it's a compile-time option.
 */
#define OPTIMIZE 1

/*
 * The code here is designed to work with variable depths for PGM and PPM.
 * The restrictions on depth are as follows.  Numbers in square brackets
 * give the actual restrictions, but the graphics library requires that
 * depth be a power of 2 or be 24.
 *	pgm: 1, 2, 4, 8, 16.  [1-16]
 *	pgmraw: 1, 2, 4, 8.  [1-8]
 *	ppm: 4(3x1), 8(3x2), 16(3x5), 24(3x8), 32(3x10).  [3-32]
 *	ppmraw: 4(3x1), 8(3x2), 16(3x5), 24(3x8).  [3-24]
 */

/* Structure for P*M devices, which extend the generic printer device. */

#define MAX_COMMENT 70			/* max user-supplied comment */
struct gx_device_pbm_s {
	gx_device_common;
	gx_prn_device_common;
	/* Additional state for P*M devices */
	char magic;			/* n for "Pn" */
	char comment[MAX_COMMENT + 1];	/* comment for head of file */
	byte is_raw;			/* 1 if raw format, 0 if plain */
	byte optimize;			/* 1 if optimization OK, 0 if not */
	byte uses_color;		/* 0 if image is black and white, */
					/* 1 if gray (PGM or PPM only), */
					/* 2 or 3 if colored (PPM only) */
};
typedef struct gx_device_pbm_s gx_device_pbm;

#define bdev ((gx_device_pbm *)pdev)

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

/* Macro for generating P*M device descriptors. */
#define pbm_prn_device(procs, dev_name, magic, is_raw, num_comp, depth, max_gray, max_rgb, print_page)\
{	prn_device_body(gx_device_pbm, procs, dev_name,\
	  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS, X_DPI, Y_DPI,\
	  0, 0, 0, 0,\
	  num_comp, depth, max_gray, max_rgb, max_gray + 1, max_rgb + 1,\
	  print_page),\
	magic,\
	 { 0 },\
	is_raw,\
	OPTIMIZE\
}

#ifdef Plan9
/* Plan 9 Dec/94: use 100 DPI for Plan 9 bitmaps */
/* Macro for generating Plan 9 device descriptors. */
#define plan9bm_prn_device(procs, dev_name, magic, is_raw, num_comp, depth, max_gray, max_rgb, print_page)\
{	prn_device_body(gx_device_pbm, procs, dev_name,\
	  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS, 100, 100,\
	  0, 0, 0, 0,\
	  num_comp, depth, max_gray, max_rgb, max_gray + 1, max_rgb + 1,\
	  print_page),\
	magic,\
	 { 0 },\
	is_raw,\
	OPTIMIZE\
}
#endif

/* For PGM and PPM, we need our own color mapping procedures. */
private dev_proc_map_rgb_color(pgm_map_rgb_color);
private dev_proc_map_rgb_color(ppm_map_rgb_color);
private dev_proc_map_color_rgb(pgm_map_color_rgb);
private dev_proc_map_color_rgb(ppm_map_color_rgb);

/* We need to initialize uses_color when opening the device. */
private dev_proc_open_device(ppm_open);

/* And of course we need our own print-page routines. */
private dev_proc_print_page(pbm_print_page);
private dev_proc_print_page(pgm_print_page);
private dev_proc_print_page(ppm_print_page);

/* The device procedures */
private gx_device_procs pbm_procs =
    prn_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close);
private gx_device_procs pgm_procs =
    prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
	pgm_map_rgb_color, pgm_map_color_rgb);
private gx_device_procs ppm_procs =
    prn_color_procs(ppm_open, gdev_prn_output_page, gdev_prn_close,
	ppm_map_rgb_color, ppm_map_color_rgb);

/* The device descriptors themselves */
gx_device_pbm far_data gs_pbm_device =
  pbm_prn_device(pbm_procs, "pbm", '1', 0, 1, 1, 1, 0,
	  pbm_print_page);
gx_device_pbm far_data gs_pbmraw_device =
  pbm_prn_device(pbm_procs, "pbmraw", '4', 1, 1, 1, 1, 1,
	  pbm_print_page);
gx_device_pbm far_data gs_pgm_device =
  pbm_prn_device(pgm_procs, "pgm", '2', 0, 1, 8, 255, 0,
	  pgm_print_page);
gx_device_pbm far_data gs_pgmraw_device =
  pbm_prn_device(pgm_procs, "pgmraw", '5', 1, 1, 8, 255, 0,
	  pgm_print_page);
gx_device_pbm far_data gs_ppm_device =
  pbm_prn_device(ppm_procs, "ppm", '3', 0, 3, 24, 255, 255,
	  ppm_print_page);
gx_device_pbm far_data gs_ppmraw_device =
  pbm_prn_device(ppm_procs, "ppmraw", '6', 1, 3, 24, 255, 255,
	  ppm_print_page);
#ifdef Plan9
/* Plan 9 Sep/94: Plan 9 bitmaps are just like a pbmraw with different header */
gx_device_pbm gs_plan9bm_device =
  plan9bm_prn_device(pbm_procs, "plan9bm", '9', 1, 1, 1, 1, 1,
	  pbm_print_page);
#endif

/* ------ Initialization ------ */

private int
ppm_open(gx_device *pdev)
{	bdev->uses_color = 0;
	return gdev_prn_open(pdev);
}

/* ------ Color mapping routines ------ */

/* Map an RGB color to a PGM gray value. */
/* Keep track of whether the image is black-and-white or gray. */
private gx_color_index
pgm_map_rgb_color(gx_device *pdev, ushort r, ushort g, ushort b)
{	/* We round the value rather than truncating it. */
	gx_color_value gray =
		((r * (ulong)lum_red_weight) +
		 (g * (ulong)lum_green_weight) +
		 (b * (ulong)lum_blue_weight) +
		 (lum_all_weights / 2)) / lum_all_weights
		* pdev->color_info.max_gray / gx_max_color_value;
	if ( !(gray == 0 || gray == pdev->color_info.max_gray) )
		bdev->uses_color = 1;
	return gray;
}

/* Map a PGM gray value back to an RGB color. */
private int
pgm_map_color_rgb(gx_device *dev, gx_color_index color, ushort prgb[3])
{	gx_color_value gray =
		color * gx_max_color_value / dev->color_info.max_gray;
	prgb[0] = gray;
	prgb[1] = gray;
	prgb[2] = gray;
	return 0;
}

/* Map an RGB color to a PPM color tuple. */
/* Keep track of whether the image is black-and-white, gray, or colored. */
private gx_color_index
ppm_map_rgb_color(gx_device *pdev, ushort r, ushort g, ushort b)
{	ushort bitspercolor = pdev->color_info.depth / 3;
	ulong max_value = (1 << bitspercolor) - 1;
	gx_color_value rc = r * max_value / gx_max_color_value;
	gx_color_value gc = g * max_value / gx_max_color_value;
	gx_color_value bc = b * max_value / gx_max_color_value;
	if ( rc == gc && gc == bc )		/* black-and-white or gray */
	{	if ( !(rc == 0 || rc == max_value) )
			bdev->uses_color |= 1;		/* gray */
	}
	else						/* color */
		bdev->uses_color = 2;
	return ((((ulong)rc << bitspercolor) + gc) << bitspercolor) + bc;
}

/* Map a PPM color tuple back to an RGB color. */
private int
ppm_map_color_rgb(gx_device *dev, gx_color_index color, ushort prgb[3])
{	ushort bitspercolor = dev->color_info.depth / 3;
	ushort colormask = (1 << bitspercolor) - 1;

	prgb[0] = ((color >> (bitspercolor * 2)) & colormask) *
		(ulong)gx_max_color_value / colormask;
	prgb[1] = ((color >> bitspercolor) & colormask) *
		(ulong)gx_max_color_value / colormask;
	prgb[2] = (color & colormask) *
		(ulong)gx_max_color_value / colormask;
	return 0;
}

/* ------ Internal routines ------ */

/* Print a page using a given row printing routine. */
private int
pbm_print_page_loop(gx_device_printer *pdev, char magic, FILE *pstream,
  int (*row_proc)(P4(gx_device_printer *, byte *, int, FILE *)))
{	uint raster = gdev_prn_raster(pdev);
	byte *data = (byte *)gs_malloc(raster, 1, "pbm_begin_page");
	int lnum = 0;
	int code = 0;
	if ( data == 0 )
		return_error(gs_error_VMerror);
#ifdef Plan9
	if(magic == '9')
		fprintf(pstream, "%11d %11d %11d %11d %11d ",
			0, 0, 0, pdev->width, pdev->height);
	else {
#else
	fprintf(pstream, "P%c\n", magic);
	if ( bdev->comment[0] )
		fprintf(pstream, "# %s\n", bdev->comment);
	else
		fprintf(pstream, "# Image generated by %s (device=%s)\n",
			gs_product, pdev->dname);
	fprintf(pstream, "%d %d\n", pdev->width, pdev->height);
#endif
#ifdef Plan9
	}
#endif
	switch ( magic )
	{
	case '1':		/* pbm */
	case '4':		/* pbmraw */
#ifdef Plan9
	case '9':		/* plan9bm */
#endif
		break;
	default:
		fprintf(pstream, "%d\n", pdev->color_info.max_gray);
	}
	for ( ; lnum < pdev->height; lnum++ )
	{	byte *row;
		code = gdev_prn_get_bits(pdev, lnum, data, &row);
		if ( code < 0 ) break;
		code = (*row_proc)(pdev, row, pdev->color_info.depth, pstream);
		if ( code < 0 ) break;
	}
	gs_free((char *)data, raster, 1, "pbm_print_page_loop");
	return (code < 0 ? code : 0);
}

/* ------ Individual page printing routines ------ */

/* Print a monobit page. */
private int
pbm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream)
{	if ( bdev->is_raw )
		fwrite(data, 1, (pdev->width + 7) >> 3, pstream);
	else
	{	byte *bp;
		uint x, mask;
		for ( bp = data, x = 0, mask = 0x80; x < pdev->width; )
		{	putc((*bp & mask ? '1' : '0'), pstream);
			if ( ++x == pdev->width || !(x & 63) )
			  putc('\n', pstream);
			if ( (mask >>= 1) == 0 )
			  bp++, mask = 0x80;
		}
	}
	return 0;
}
private int
pbm_print_page(gx_device_printer *pdev, FILE *pstream)
{	return pbm_print_page_loop(pdev, bdev->magic, pstream, pbm_print_row);
}

/* Print a gray-mapped page. */
private int
pgm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream)
{	/* Note that bpp <= 8 for raw format, bpp <= 16 for plain. */
	uint mask = (1 << depth) - 1;
	byte *bp;
	uint x;
	int shift;
	if ( bdev->is_raw && depth == 8 )
		fwrite(data, 1, pdev->width, pstream);
	else
	  for ( bp = data, x = 0, shift = 8 - depth; x < pdev->width; )
	{	uint pixel;
		if ( shift < 0 )	/* bpp = 16 */
		   {	pixel = ((uint)*bp << 8) + bp[1];
			bp += 2;
		   }
		else
		   {	pixel = (*bp >> shift) & mask;
			if ( (shift -= depth) < 0 )
				bp++, shift += 8;
		   }
		++x;
		if ( bdev->is_raw )
			putc(pixel, pstream);
		else
			fprintf(pstream, "%d%c", pixel,
				(x == pdev->width || !(x & 15) ?
				 '\n' : ' '));
	}
	return 0;
}
private int
pxm_pbm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream)
{	/* Compress a PGM or PPM row to a PBM row. */
	/* This doesn't have to be very fast. */
	/* Note that we have to invert the data as well. */
	int delta = (depth + 7) >> 3;
	byte *src = data + delta - 1;		/* always big-endian */
	byte *dest = data;
	int x;
	byte in_mask = (depth >= 8 ? 1 : 0x100 >> depth);
	byte out_mask = 0x80;
	byte out = 0;
	for ( x = 0; x < pdev->width; x++ )
	{	if ( !(*src & in_mask) )
			out |= out_mask;
		if ( depth >= 8 )
			src += delta;
		else
		{	in_mask >>= depth;
			if ( !in_mask )
				in_mask = 0x100 >> depth,
				src++;
		}
		out_mask >>= 1;
		if ( !out_mask )
			out_mask = 0x80,
			*dest++ = out,
			out = 0;
	}
	if ( out_mask != 0x80 )
		*dest = out;
	return pbm_print_row(pdev, data, 1, pstream);
}
private int
pgm_print_page(gx_device_printer *pdev, FILE *pstream)
{	return (bdev->uses_color == 0 && bdev->optimize ?
		pbm_print_page_loop(pdev, bdev->magic - 1, pstream,
				    pxm_pbm_print_row) :
		pbm_print_page_loop(pdev, bdev->magic, pstream,
				    pgm_print_row) );
}

/* Print a color-mapped page. */
private int
ppgm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream, int color)
{	/* If color=0, write only one value per pixel; */
	/* if color=1, write 3 values per pixel. */
	/* Note that depth <= 24 for raw format, depth <= 32 for plain. */
	uint bpe = depth / 3;	/* bits per r/g/b element */
	uint mask = (1 << bpe) - 1;
	byte *bp;
	uint x;
	uint eol_mask = (color ? 7 : 15);
	int shift;
	if ( bdev->is_raw && depth == 24 && color )
		fwrite(data, 1, pdev->width * (depth / 8), pstream);
	else
	  for ( bp = data, x = 0, shift = 8 - depth; x < pdev->width; )
	{	bits32 pixel = 0;
		uint r, g, b;
		switch ( depth >> 3 )
		   {
		case 4:
			pixel = (bits32)*bp << 24; bp++;
			/* falls through */
		case 3:
			pixel += (bits32)*bp << 16; bp++;
			/* falls through */
		case 2:
			pixel += (uint)*bp << 8; bp++;
			/* falls through */
		case 1:
			pixel += *bp; bp++;
			break;
		case 0:			/* bpp == 4, bpe == 1 */
			pixel = *bp >> shift;
			if ( (shift -= depth) < 0 )
				bp++, shift += 8;
			break;
		   }
		++x;
		b = pixel & mask;  pixel >>= bpe;
		g = pixel & mask;  pixel >>= bpe;
		r = pixel & mask;
		if ( bdev->is_raw )
		{	if ( color )
			{	putc(r, pstream);
				putc(g, pstream);
			}
			putc(b, pstream);
		}
		else
		{	if ( color )
				fprintf(pstream, "%d %d ", r, g);
			fprintf(pstream, "%d%c", b,
				(x == pdev->width || !(x & eol_mask) ?
				 '\n' : ' '));
		}
	}
	return 0;
}
private int
ppm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream)
{	return ppgm_print_row(pdev, data, depth, pstream, 1);
}
private int
ppm_pgm_print_row(gx_device_printer *pdev, byte *data, int depth,
  FILE *pstream)
{	return ppgm_print_row(pdev, data, depth, pstream, 0);
}
private int
ppm_print_page(gx_device_printer *pdev, FILE *pstream)
{	return (bdev->uses_color >= 2 || !bdev->optimize ?
		pbm_print_page_loop(pdev, bdev->magic, pstream,
				    ppm_print_row) :
		bdev->uses_color == 1 ?
		pbm_print_page_loop(pdev, bdev->magic - 1, pstream,
				    ppm_pgm_print_row) :
		pbm_print_page_loop(pdev, bdev->magic - 2, pstream,
				    pxm_pbm_print_row) );
}
