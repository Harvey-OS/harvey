/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevbmpc.c,v 1.8 2005/08/04 17:38:45 alexcher Exp $ */
/* .BMP file format driver utilities */
#include "gdevprn.h"
#include "gdevbmp.h"

/*
 * Define BMP file format structures.
 * All multi-byte quantities are stored LSB-first!
 */

typedef ushort word;
#if arch_sizeof_int == 4
typedef uint dword;
#else
#  if arch_sizeof_long == 4
typedef ulong dword;
#  endif
#endif
#if arch_is_big_endian
#  define BMP_ASSIGN_WORD(a,v) a = ((v) >> 8) + ((v) << 8)
#  define BMP_ASSIGN_DWORD(a,v)\
     a = ((v) >> 24) + (((v) >> 8) & 0xff00L) +\
	 (((dword)(v) << 8) & 0xff0000L) + ((dword)(v) << 24)
#else
#  define BMP_ASSIGN_WORD(a,v) a = (v)
#  define BMP_ASSIGN_DWORD(a,v) a = (v)
#endif

typedef struct bmp_file_header_s {

    /* BITMAPFILEHEADER */

    /*
     * This structure actually begins with two bytes
     * containing the characters 'BM', but we must omit them,
     * because some compilers would insert padding to force
     * the size member to a 32- or 64-bit boundary.
     */

    /*byte  typeB, typeM; *//* always 'BM' */
    dword size;			/* total size of file */
    word reserved1;
    word reserved2;
    dword offBits;		/* offset of bits from start of file */

} bmp_file_header;

#define sizeof_bmp_file_header (2 + sizeof(bmp_file_header))

typedef struct bmp_info_header_s {

    /* BITMAPINFOHEADER */

    dword size;			/* size of info header in bytes */
    dword width;		/* width in pixels */
    dword height;		/* height in pixels */
    word planes;		/* # of planes, always 1 */
    word bitCount;		/* bits per pixel */
    dword compression;		/* compression scheme, always 0 */
    dword sizeImage;		/* size of bits */
    dword xPelsPerMeter;	/* X pixels per meter */
    dword yPelsPerMeter;	/* Y pixels per meter */
    dword clrUsed;		/* # of colors used */
    dword clrImportant;		/* # of important colors */

    /* This is followed by (1 << bitCount) bmp_quad structures, */
    /* unless bitCount == 24. */

} bmp_info_header;

typedef struct bmp_quad_s {

    /* RGBQUAD */

    byte blue, green, red, reserved;

} bmp_quad;

/* Write the BMP file header. */
private int
write_bmp_depth_header(gx_device_printer *pdev, FILE *file, int depth,
		       const byte *palette /* [4 << depth] */,
		       int raster)
{
    /* BMP scan lines are padded to 32 bits. */
    ulong bmp_raster = raster + (-raster & 3);
    int height = pdev->height;
    int quads = (depth <= 8 ? sizeof(bmp_quad) << depth : 0);

    /* Write the file header. */

    fputc('B', file);
    fputc('M', file);
    {
	bmp_file_header fhdr;

	BMP_ASSIGN_DWORD(fhdr.size,
		     sizeof_bmp_file_header +
		     sizeof(bmp_info_header) + quads +
		     bmp_raster * height);
	BMP_ASSIGN_WORD(fhdr.reserved1, 0);
	BMP_ASSIGN_WORD(fhdr.reserved2, 0);
	BMP_ASSIGN_DWORD(fhdr.offBits,
		     sizeof_bmp_file_header +
		     sizeof(bmp_info_header) + quads);
	if (fwrite((const char *)&fhdr, 1, sizeof(fhdr), file) != sizeof(fhdr))
	    return_error(gs_error_ioerror);
    }

    /* Write the info header. */

    {
	bmp_info_header ihdr;

	BMP_ASSIGN_DWORD(ihdr.size, sizeof(ihdr));
	BMP_ASSIGN_DWORD(ihdr.width, pdev->width);
	BMP_ASSIGN_DWORD(ihdr.height, height);
	BMP_ASSIGN_WORD(ihdr.planes, 1);
	BMP_ASSIGN_WORD(ihdr.bitCount, depth);
	BMP_ASSIGN_DWORD(ihdr.compression, 0);
	BMP_ASSIGN_DWORD(ihdr.sizeImage, bmp_raster * height);
	/*
	 * Earlier versions of this driver set the PelsPerMeter values
	 * to zero.  At a user's request, we now set them correctly,
	 * but we suspect this will cause problems other places.
	 */
#define INCHES_PER_METER (100 /*cm/meter*/ / 2.54 /*cm/inch*/)
	BMP_ASSIGN_DWORD(ihdr.xPelsPerMeter,
		 (dword)(pdev->x_pixels_per_inch * INCHES_PER_METER + 0.5));
	BMP_ASSIGN_DWORD(ihdr.yPelsPerMeter,
		 (dword)(pdev->y_pixels_per_inch * INCHES_PER_METER + 0.5));
#undef INCHES_PER_METER
	BMP_ASSIGN_DWORD(ihdr.clrUsed, 0);
	BMP_ASSIGN_DWORD(ihdr.clrImportant, 0);
	if (fwrite((const char *)&ihdr, 1, sizeof(ihdr), file) != sizeof(ihdr))
	    return_error(gs_error_ioerror);
    }

    /* Write the palette. */

    if (depth <= 8)
	fwrite(palette, sizeof(bmp_quad), 1 << depth, file);

    return 0;
}

/* Write the BMP file header. */
int
write_bmp_header(gx_device_printer *pdev, FILE *file)
{
    int depth = pdev->color_info.depth;
    bmp_quad palette[256];

    if (depth <= 8) {
	int i;
	gx_color_value rgb[3];
	bmp_quad q;

	q.reserved = 0;
	for (i = 0; i != 1 << depth; i++) {
	    /* Note that the use of map_color_rgb is deprecated in
	       favor of decode_color. This should work, though, because
	       backwards compatibility is preserved. */
	    (*dev_proc(pdev, map_color_rgb))((gx_device *)pdev,
					     (gx_color_index)i, rgb);
	    q.red = gx_color_value_to_byte(rgb[0]);
	    q.green = gx_color_value_to_byte(rgb[1]);
	    q.blue = gx_color_value_to_byte(rgb[2]);
	    palette[i] = q;
	}
    }
    return write_bmp_depth_header(pdev, file, depth, (const byte *)palette,
				  gdev_prn_raster(pdev));
}

/* Write a BMP header for separated CMYK output. */
int
write_bmp_separated_header(gx_device_printer *pdev, FILE *file)
{
    int depth = pdev->color_info.depth;
    int plane_depth = depth / 4;
    bmp_quad palette[256];
    bmp_quad q;
    int i;

    q.reserved = 0;
    for (i = 0; i < 1 << plane_depth; i++) {
	q.red = q.green = q.blue =
	    255 - i * 255 / ((1 << plane_depth) - 1);
	palette[i] = q;
    }
    return write_bmp_depth_header(pdev, file, plane_depth,
				  (const byte *)palette,
				  (pdev->width*plane_depth + 7) >> 3);
}

/* 24-bit color mappers (taken from gdevmem2.c). */
/* Note that Windows expects RGB values in the order B,G,R. */

/* Map a r-g-b color to a color index. */
gx_color_index
bmp_map_16m_rgb_color(gx_device * dev, const gx_color_value cv[])
{

    gx_color_value r, g, b;
    r = cv[0]; g = cv[1]; b = cv[2];
    return gx_color_value_to_byte(r) +
	((uint) gx_color_value_to_byte(g) << 8) +
	((ulong) gx_color_value_to_byte(b) << 16);
}

/* Map a color index to a r-g-b color. */
int
bmp_map_16m_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
    prgb[2] = gx_color_value_from_byte(color >> 16);
    prgb[1] = gx_color_value_from_byte((color >> 8) & 0xff);
    prgb[0] = gx_color_value_from_byte(color & 0xff);
    return 0;
}
