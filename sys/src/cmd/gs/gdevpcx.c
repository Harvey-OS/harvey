/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevpcx.c */
/* PCX file format drivers */
#include "gdevprn.h"
#include "gdevpccm.h"
#include "gxlum.h"

/* Thanks to Phil Conrad for donating the original version */
/* of these drivers to Aladdin Enterprises. */

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

/* Monochrome. */

private dev_proc_print_page(pcxmono_print_page);

/* Use the default RGB->color map, so we get black=0, white=1. */
private gx_device_procs pcxmono_procs =
  prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
    gx_default_map_rgb_color, gx_default_map_color_rgb);
gx_device_printer far_data gs_pcxmono_device =
  prn_device(pcxmono_procs, "pcxmono",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, pcxmono_print_page);

/* Chunky 8-bit gray scale. */

private dev_proc_print_page(pcx256_print_page);

private gx_device_procs pcxgray_procs =
  prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
    gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb);
gx_device_printer far_data gs_pcxgray_device =
  prn_device(pcxgray_procs, "pcxgray",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	8, pcx256_print_page);

/* 4-bit planar (EGA/VGA-style) color. */

private dev_proc_print_page(pcx16_print_page);

private gx_device_procs pcx16_procs =
  prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
    pc_4bit_map_rgb_color, pc_4bit_map_color_rgb);
gx_device_printer far_data gs_pcx16_device =
  prn_device(pcx16_procs, "pcx16",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, pcx16_print_page);

/* Chunky 8-bit (SuperVGA-style) color. */
/* (Uses a fixed palette of 3,3,2 bits.) */

private gx_device_procs pcx256_procs =
  prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
    pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
gx_device_printer far_data gs_pcx256_device =
  prn_device(pcx256_procs, "pcx256",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	8, pcx256_print_page);

/* 24-bit color, 3 8-bit planes. */

private dev_proc_print_page(pcx24b_print_page);

private gx_device_procs pcx24b_procs =
  prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
    gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);
gx_device_printer far_data gs_pcx24b_device =
  prn_device(pcx24b_procs, "pcx24b",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	24, pcx24b_print_page);

/* ------ Private definitions ------ */

/* All two-byte quantities are stored LSB-first! */
#if arch_is_big_endian
#  define assign_ushort(a,v) a = ((v) >> 8) + ((v) << 8)
#else
#  define assign_ushort(a,v) a = (v)
#endif

typedef struct pcx_header_s {
	byte 	manuf;		/* always 0x0a */
	byte	version;	/* version info = 0,2,3,5 */
	byte	encoding;	/* 1=RLE */
	byte	bpp;		/* bits per pixel */
	ushort	x1;		/* picture dimensions */
	ushort	y1;
	ushort	x2;
	ushort	y2;
	ushort	hres;		/* horz. resolution */
	ushort	vres;		/* vert. resolution */
	byte	palette[16*3];	/* color palette */
	byte	reserved;
	byte	nplanes;	/* number of color planes */
	ushort	bpl;		/* number of bytes per line (uncompressed) */
	ushort	palinfo;	/* palette info 1=color, 2=grey */
	byte	xtra[58];	/* fill out header to 128 bytes */
} pcx_header;

/* 
** version info for PCX is as follows 
**
** 0 == 2.5
** 2 == 2.8 w/palette info
** 3 == 2.8 without palette info
** 5 == 3.0 (includes palette)
**
*/

/* Forward declarations */
private void pcx_write_rle(P3(const byte *, const byte *, FILE *));
private int pcx_write_page(P4(gx_device_printer *, FILE *, pcx_header _ss *, int));

/* Write a monochrome PCX page. */
private int
pcxmono_print_page(gx_device_printer *pdev, FILE *file)
{	pcx_header header;
	header.bpp = 1;
	header.nplanes = 1;
	/* Set the first two entries of the short palette. */
	memcpy((byte *)header.palette, "\000\000\000\377\377\377", 6);
	return pcx_write_page(pdev, file, &header, 0);
}

/* Write an "old" PCX page. */
static const byte ega_palette[16*3] = {
  0x00,0x00,0x00,  0x00,0x00,0xaa,  0x00,0xaa,0x00,  0x00,0xaa,0xaa,
  0xaa,0x00,0x00,  0xaa,0x00,0xaa,  0xaa,0xaa,0x00,  0xaa,0xaa,0xaa,
  0x55,0x55,0x55,  0x55,0x55,0xff,  0x55,0xff,0x55,  0x55,0xff,0xff,
  0xff,0x55,0x55,  0xff,0x55,0xff,  0xff,0xff,0x55,  0xff,0xff,0xff
};
private int
pcx16_print_page(gx_device_printer *pdev, FILE *file)
{	pcx_header header;
	header.bpp = 1;
	header.nplanes = 4;
	/* Fill the EGA palette appropriately. */
	memcpy((byte *)header.palette, ega_palette, sizeof(ega_palette));
	return pcx_write_page(pdev, file, &header, 1);
}

/* Write a "new" PCX page. */
private int
pcx256_print_page(gx_device_printer *pdev, FILE *file)
{	pcx_header header;
	int code;
	header.bpp = 8;
	header.nplanes = 1;
	/* Clear the EGA palette */
	memset((byte *)header.palette, 0, sizeof(header.palette));
	code = pcx_write_page(pdev, file, &header, 0);
	if ( code >= 0 )
	{	/* Write out the palette. */
		fputc(0x0c, file);
		code = pc_write_palette((gx_device *)pdev, 256, file);
	}
	return code;
}

/* Write a 24-bit color PCX page. */

private int
pcx24b_print_page(gx_device_printer *pdev, FILE *file)
{	pcx_header header;
	header.bpp = 8;
	header.nplanes = 3;
	/* Clear the EGA palette */
	memset((byte *)header.palette, 0, sizeof(header.palette));
	return pcx_write_page(pdev, file, &header, 1);
}

/* Write out a page in PCX format. */
/* This routine is used for all formats. */
private int
pcx_write_page(gx_device_printer *pdev, FILE *file, pcx_header _ss *phdr,
  int planar)
{	int raster = gdev_prn_raster(pdev);
	uint rsize = round_up((pdev->width * phdr->bpp + 7) >> 3, 2);	/* PCX format requires even */
	int height = pdev->height;
	int depth = pdev->color_info.depth;
	uint lsize = raster + rsize;
	byte *line = (byte *)gs_malloc(lsize, 1, "pcx file buffer");
	byte *plane = line + raster;
	int y;
	int code = 0;			/* return code */
	if ( line == 0 )		/* can't allocate line buffer */
		return_error(gs_error_VMerror);

	/* Set up the header struct. */

	phdr->manuf = 10;
	phdr->version = 5;
	phdr->encoding = 1;	/* 1 for rle 8-bit encoding */
	/* bpp was set by the caller */
	phdr->x1 = 0;
	phdr->y1 = 0;
	assign_ushort(phdr->x2, pdev->width-1);
	assign_ushort(phdr->y2, height-1);
	assign_ushort(phdr->hres, (int)pdev->x_pixels_per_inch);
	assign_ushort(phdr->vres, (int)pdev->y_pixels_per_inch);
	phdr->reserved = 0;
	/* nplanes was set by the caller */
	assign_ushort(phdr->bpl, (planar || depth == 1 ? rsize : raster));
	assign_ushort(phdr->palinfo, (gx_device_has_color(pdev) ? 1 : 2));
	/* Zero out the unused area. */
	memset(&phdr->xtra[0], 0, sizeof(phdr->xtra));

	/* Write the header. */

	if ( fwrite((const char *)phdr, 1, 128, file) < 128 )
	{	code = gs_error_ioerror;
		goto pcx_done;
	}

	/* Dump the contents of the image. */
	for ( y = 0; y < height; y++ )
	{	byte *row;
		byte *end;
		int code = gdev_prn_get_bits(pdev, y, line, &row);
		if ( code < 0 ) break;
		end = row + raster;
		if ( !planar )
		{	/* Just write the bits. */
			end += raster & 1;	/* round to even */
			pcx_write_rle(row, end, file);
		}
		else
		  switch ( depth )
		{

		case 8:
		{	register int shift;

			if ( !gx_device_has_color(pdev) )
			   {	/* Can't map gray scale */
				code = gs_error_undefinedresult;
				goto pcx_done;
			   }

			for ( shift = 0; shift < 4; shift++ )
			{	register byte *from, *to;
				register int bmask = 1 << shift;
				for ( from = row, to = plane;
				      from < end; from += 8
				    )
				{	*to++ =
					  ((((uint)from[0] & bmask) << 7) +
					   (((uint)from[1] & bmask) << 6) +
					   (((uint)from[2] & bmask) << 5) +
					   (((uint)from[3] & bmask) << 4) +
					   (((uint)from[4] & bmask) << 3) +
					   (((uint)from[5] & bmask) << 2) +
					   (((uint)from[6] & bmask) << 1) +
					   (((uint)from[7] & bmask)))
					  >> shift;
				}
				pcx_write_rle(plane, plane + rsize, file);
			}
		}
			break;

		case 24:
		{	byte *crow = row;
			for ( ; crow != row + 3; crow++ )
			{	register byte *from, *to;
				for ( from = crow, to = plane;
				      from < end; to += 2, from += 6
				    )
				  *to = *from,
				  to[1] = from[3];
				if ( raster & 1 )
				  to[-1] = to[-2];	/* as good as any */
				pcx_write_rle(plane, plane + rsize, file);
			}
		}
			break;

		default:
			  code = gs_error_undefinedresult;
			  goto pcx_done;

		}
	}

pcx_done:
	gs_free((char *)line, lsize, 1, "pcx file buffer");

	return code;
}

/* ------ Internal routines ------ */

/* Write one line in PCX run-length-encoded format. */
private void
pcx_write_rle(const byte *from, const byte *end, FILE *file)
{	while ( from < end )
	{	byte data = *from++;
		if ( data != *from || from == end )
		  {	if ( data >= 0xc0 )
			  putc(0xc1, file);
		  }
		else
		  {	const byte *next = (end - from > 62 ? from + 62 : end);
			const byte *start = from;
			while ( (from < next) && (*from == data) )
			  from++;
			putc(from - start + 0xc1, file);
		  }
		putc(data, file);
	}
}
