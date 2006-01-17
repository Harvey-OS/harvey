/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevl31s.c,v 1.5 2004/09/02 21:30:53 giles Exp $ */
/*
 * H-P LaserJet 3100 driver
 *
 * This is a driver for use with the H-P LaserJet 3100 Software.
 * It requires installed H-P LaserJet 3100 Software to print.
 * It can be used with smbclient to print from an UNIX box
 * to a LaserJet 3100 printer attached to a MS-Windows box.
 *
 * Written by Ulrich Schmid, uschmid@mail.hh.provi.de.
 */

#include "gdevprn.h"
#include "gdevmeds.h"

#define XCORRECTION 0.11
#define YCORRECTION 0.12

/* order matters!             0       1        2        3       4      5     6       7           8 */
const char *media[10]   =  {"a4", "letter", "legal", "com10", "c5",  "dl", "b5", "monarch", "executive", 0};
const int height[2][10] = {{3447,     3240,    4140,    5587, 2644,  5083, 2975,      4387,        3090, 0},
			   {6894,     6480,    8280,   11167, 5288, 10159, 5950,      8767,        6180, 0}};
const int width[2]      = {2528,
			   5056};
#define LARGEST_MEDIUM 2 /* legal */

/* These codes correspond to sequences of pixels with the same color.
 * After the code for a sequence < 64 pixels the color changes.
 * After the code for a sequence with 64 pixels the previous color continues. */ 
private struct {
	uint bits;
	uint length; /* number of valid bits */
} code[2][65] =
/* White */
{{{0x0ac,  8}, {0x038,  6}, {0x00e,  4}, {0x001,  4}, {0x00d,  4}, {0x003,  4}, {0x007,  4}, {0x00f,  4},
  {0x019,  5}, {0x005,  5}, {0x01c,  5}, {0x002,  5}, {0x004,  6}, {0x030,  6}, {0x00b,  6}, {0x02b,  6},
  {0x015,  6}, {0x035,  6}, {0x072,  7}, {0x018,  7}, {0x008,  7}, {0x074,  7}, {0x060,  7}, {0x010,  7},
  {0x00a,  7}, {0x06a,  7}, {0x064,  7}, {0x012,  7}, {0x00c,  7}, {0x040,  8}, {0x0c0,  8}, {0x058,  8},
  {0x0d8,  8}, {0x048,  8}, {0x0c8,  8}, {0x028,  8}, {0x0a8,  8}, {0x068,  8}, {0x0e8,  8}, {0x014,  8},
  {0x094,  8}, {0x054,  8}, {0x0d4,  8}, {0x034,  8}, {0x0b4,  8}, {0x020,  8}, {0x0a0,  8}, {0x050,  8},
  {0x0d0,  8}, {0x04a,  8}, {0x0ca,  8}, {0x02a,  8}, {0x0aa,  8}, {0x024,  8}, {0x0a4,  8}, {0x01a,  8},
  {0x09a,  8}, {0x05a,  8}, {0x0da,  8}, {0x052,  8}, {0x0d2,  8}, {0x04c,  8}, {0x0cc,  8}, {0x02c,  8},
  {0x01b,  5}},
/* Black */
 {{0x3b0, 10}, {0x002,  3}, {0x003,  2}, {0x001,  2}, {0x006,  3}, {0x00c,  4}, {0x004,  4}, {0x018,  5},
  {0x028,  6}, {0x008,  6}, {0x010,  7}, {0x050,  7}, {0x070,  7}, {0x020,  8}, {0x0e0,  8}, {0x030,  9},
  {0x3a0, 10}, {0x060, 10}, {0x040, 10}, {0x730, 11}, {0x0b0, 11}, {0x1b0, 11}, {0x760, 11}, {0x0a0, 11},
  {0x740, 11}, {0x0c0, 11}, {0x530, 12}, {0xd30, 12}, {0x330, 12}, {0xb30, 12}, {0x160, 12}, {0x960, 12},
  {0x560, 12}, {0xd60, 12}, {0x4b0, 12}, {0xcb0, 12}, {0x2b0, 12}, {0xab0, 12}, {0x6b0, 12}, {0xeb0, 12},
  {0x360, 12}, {0xb60, 12}, {0x5b0, 12}, {0xdb0, 12}, {0x2a0, 12}, {0xaa0, 12}, {0x6a0, 12}, {0xea0, 12},
  {0x260, 12}, {0xa60, 12}, {0x4a0, 12}, {0xca0, 12}, {0x240, 12}, {0xec0, 12}, {0x1c0, 12}, {0xe40, 12},
  {0x140, 12}, {0x1a0, 12}, {0x9a0, 12}, {0xd40, 12}, {0x340, 12}, {0x5a0, 12}, {0x660, 12}, {0xe60, 12},
  {0x3c0, 10}}}; 

/* Define the default, maximum resolutions. */
#ifndef X_DPI
#  define X_DPI 600
#endif
#ifndef Y_DPI
#  define Y_DPI 600
#endif

/* The device descriptors */
private dev_proc_print_page_copies(lj3100sw_print_page_copies);
private dev_proc_close_device(lj3100sw_close);

private gx_device_procs prn_lj3100sw_procs = 
    prn_params_procs(gdev_prn_open, gdev_prn_output_page, lj3100sw_close,
	     gdev_prn_get_params, gdev_prn_put_params);

/* workaround to emulate the missing prn_device_margins_copies macro */
#define gx_default_print_page_copies lj3100sw_print_page_copies
gx_device_printer far_data gs_lj3100sw_device =
    prn_device_margins/*_copies*/(prn_lj3100sw_procs, "lj3100sw",
	     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	     X_DPI, Y_DPI,
	     XCORRECTION, YCORRECTION,
	     0.25, 0.2, 0.25, 0.2,
	     1, 0 /* lj3100sw_print_page_copies */);
#undef gx_default_print_page_copies

#define ppdev ((gx_device_printer *)pdev)

#define BUFFERSIZE 0x1000

private void
lj3100sw_output_section_header(FILE *prn_stream, int type, int arg1, int arg2)
{
	fputc(type      & 0xff, prn_stream);
	fputc(type >> 8 & 0xff, prn_stream);
	fputc(arg1      & 0xff, prn_stream);
	fputc(arg1 >> 8 & 0xff, prn_stream);
	fputc(arg2      & 0xff, prn_stream);
	fputc(arg2 >> 8 & 0xff, prn_stream);
}

private void
lj3100sw_flush_buffer(FILE *prn_stream, char *buffer, char **pptr)
{
	int size = *pptr - buffer;
	if (size) {
		lj3100sw_output_section_header(prn_stream, 0, size, 0);
		fwrite(buffer, 1, size, prn_stream);
		*pptr = buffer;
	}
}

private void
lj3100sw_output_data_byte(FILE *prn_stream, char *buffer, char **pptr, int val)
{
	if (*pptr >= buffer + BUFFERSIZE)
		lj3100sw_flush_buffer(prn_stream, buffer, pptr);
	*(*pptr)++ = val;
}

private void
lj3100sw_output_repeated_data_bytes(FILE *prn_stream, char *buffer, char **pptr, int val, int num)
{
	int size;
	while (num) {
		if (*pptr >= buffer + BUFFERSIZE)
			lj3100sw_flush_buffer(prn_stream, buffer, pptr);
		size = min(num, buffer + BUFFERSIZE - *pptr);
		memset(*pptr, val, size);
		*pptr += size;
		num -= size;
	}
}

private void
lj3100sw_output_newline(FILE *prn_stream, char *buffer, char **pptr)
{
	lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0);
	lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0);
	lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x80);
}

private void
lj3100sw_output_empty_line(FILE *prn_stream, char *buffer, char **pptr, bool high_resolution)
{
	if (high_resolution) {
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x80);
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x0f);
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x78);
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0xac);
	} else {
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x80);
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x87);
		lj3100sw_output_data_byte(prn_stream, buffer, pptr, 0x0d);
	}
}

private int
lj3100sw_print_page_copies(gx_device_printer *pdev, FILE *prn_stream, int num_copies /* ignored */)
{
	int i, j;
	char buffer[BUFFERSIZE], *ptr = buffer;
	int medium_index = select_medium(pdev, media, LARGEST_MEDIUM);
	bool high_resolution = (pdev->x_pixels_per_inch > 300);
	int printer_height = height[high_resolution ? 1 : 0][medium_index];
	int printer_width  = width[high_resolution ? 1 : 0];
	int paper_height = pdev->height;
	int paper_width  = pdev->width;
	int line_size = gdev_prn_raster(pdev);
	gs_memory_t *mem = pdev->memory;
	byte *in = (byte *)gs_malloc(mem, line_size, 1, "lj3100sw_print_page");
	byte *data;
	if (in == 0)
		return_error(gs_error_VMerror);
	if (gdev_prn_file_is_new(pdev)) {
		lj3100sw_output_section_header(prn_stream, 1, 0, 0);
		lj3100sw_output_repeated_data_bytes(prn_stream, buffer, &ptr, 0x1b, 12);
		ptr += sprintf(ptr, "\r\nBD");
		lj3100sw_output_repeated_data_bytes(prn_stream, buffer, &ptr, 0, 5520);
		ptr += sprintf(ptr, "%s\r\n%s %d\r\n%s %d\r\n%s %d\r\n%s %d\r\n%s %d\r\n%s %d\r\n",
			       "NJ",
			       "PQ", -1,
			       "RE",  high_resolution ? 6 : 2,
			       "SL",  printer_width,
			       "LM",  0,
			       "PS",  medium_index,
			       "PC",  0);
		lj3100sw_flush_buffer(prn_stream, buffer, &ptr);
	}

	lj3100sw_output_section_header(prn_stream, 3, ppdev->NumCopies, 0);
	ptr += sprintf(ptr, "%s %d\r\n%s\r\n",
		       "CM", 1,
		       "PD");
	*ptr++ = 0;
	lj3100sw_output_newline(prn_stream, buffer, &ptr);

	for (i = 0; i < printer_height; i++) {
		if (i < paper_height) {
			int color = 0; /* white */
			int count = 0;
			int bit_index = 0;
			uint tmp = 0;
			gdev_prn_get_bits(pdev, i, in, &data);
			for (j = 0; j <= printer_width; j++) {
				int xoffset = (printer_width - paper_width) / 2;
				int newcolor = 0;
				if (j >= xoffset && j <  xoffset + paper_width)
					newcolor = (data[(j - xoffset) / 8] >> (7 - (j - xoffset) % 8)) & 1;
				if (j == printer_width)
					newcolor = !color; /* force output */
				if (newcolor == color)
					count++;
				else if (count == printer_width && color == 0) /* implies j == printer_width */
					lj3100sw_output_empty_line(prn_stream, buffer, &ptr, high_resolution);
				else    /* print a sequence of pixels with a uniform color */
					while (newcolor != color) {
						int size = min(count, 64);
						tmp |= code[color][size].bits << bit_index;
						bit_index += code[color][size].length;
						while (bit_index >= 8)	{
							lj3100sw_output_data_byte(prn_stream, buffer, &ptr, tmp & 0xff);
							tmp >>= 8;
							bit_index -= 8;
						}
						if (size == 64)
							count -= 64;
						else {
							color = newcolor;
							count = 1;
						}
					}
			}
			if (bit_index)
				lj3100sw_output_data_byte(prn_stream, buffer, &ptr, tmp & 0xff);
		}
		else
			lj3100sw_output_empty_line(prn_stream, buffer, &ptr, high_resolution);
		lj3100sw_output_newline(prn_stream, buffer, &ptr);
	}

	for (i = 0; i < 3; i++ ) {
		lj3100sw_output_data_byte(prn_stream, buffer, &ptr, 0x00);
		lj3100sw_output_data_byte(prn_stream, buffer, &ptr, 0x08);
		lj3100sw_output_data_byte(prn_stream, buffer, &ptr, 0x80);
	}
	lj3100sw_output_repeated_data_bytes(prn_stream, buffer, &ptr, 0, 520);
	lj3100sw_flush_buffer(prn_stream, buffer, &ptr);

	lj3100sw_output_section_header(prn_stream, 4, 0, 0);
	for (i = 0; i < 4 * ppdev->NumCopies; i++)
		lj3100sw_output_section_header(prn_stream, 54, 0, 0);

	gs_free(mem, (char *)in, line_size, 1, "lj3100sw_print_page");
	return 0;
}

private int
lj3100sw_close(gx_device *pdev)
{
	int i;
	FILE *prn_stream = ((gx_device_printer *)pdev)->file;

	lj3100sw_output_section_header(prn_stream, 0, 4, 0);
	fputs("XX\r\n", prn_stream);
	for (i = 0; i < 4 * ppdev->NumCopies; i++)
		lj3100sw_output_section_header(prn_stream, 54, 0, 0);
	lj3100sw_output_section_header(prn_stream, 2, 0, 0);

	return gdev_prn_close(pdev);
}
