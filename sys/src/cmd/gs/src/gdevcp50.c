/* Copyright (C) 1991, 1994, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcp50.c,v 1.7 2005/01/19 00:24:07 dan Exp $*/
/* Mitsubishi CP50 color printer driver */
#include "gdevprn.h"
#define ppdev ((gx_device_printer *)pdev)

/***
 *** Note: this driver was contributed by a user.  Please contact
 ***       Michael Hu (michael@ximage.com) if you have questions.
 ***/

/* The value of X_PIXEL and Y_PIXEL is gained by experiment */
#define X_PIXEL   474
#define Y_PIXEL   800

/* The value of FIRST_LINE and LAST_LINE is gained by experiment */
/* Note: LAST-LINE - FIRST_LINE + 1 should close to Y_PIXEL */
#define FIRST_LINE 140
#define LAST_LINE  933

/* The value of FIRST is gained by experiment */
/* There are 60 pixel(RGB) in the right clipped margin */
#define FIRST_COLUMN    180

/* The value of X_DPI and Y_DPI is gained by experiment */
#define X_DPI 154		/* pixels per inch */ 
#define Y_DPI 187		/* pixels per inch */

/* The device descriptor */
private dev_proc_print_page(cp50_print_page);
private dev_proc_output_page(cp50_output_page);

private dev_proc_map_rgb_color(cp50_rgb_color);
private dev_proc_map_color_rgb(cp50_color_rgb);

private gx_device_procs cp50_procs =
  prn_color_procs(gdev_prn_open, cp50_output_page, gdev_prn_close,
    cp50_rgb_color, cp50_color_rgb);

const gx_device_printer far_data gs_cp50_device =
  prn_device(cp50_procs, "cp50",
	39,				/* width_10ths, 100mm */
	59,				/* height_10ths,150mm  */
	X_DPI, Y_DPI,
	0.39, 0.91, 0.43, 0.75,		/* margins */
	24, cp50_print_page);

int copies;

/* ------ Internal routines ------ */


/* Send the page to the printer. */
private int
cp50_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	byte *out = (byte *)gs_malloc(pdev->memory, line_size, 1, "cp50_print_page(out)");
    byte *r_plane = (byte *)gs_malloc(pdev->memory, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(r_plane)");
    byte *g_plane = (byte *)gs_malloc(pdev->memory, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(g_plane)");
    byte *b_plane = (byte *)gs_malloc(pdev->memory, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(b_plane)");
    byte *t_plane = (byte *)gs_malloc(pdev->memory, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(t_plane)");
	int lnum = FIRST_LINE;
	int last = LAST_LINE;
    int lines = X_PIXEL;
    byte hi_lines, lo_lines;
    byte num_copies;
    int i,j;


/*fprintf(prn_stream, "%d,%d,%d,", pdev->width, pdev->height, line_size);*/

	/* Check allocations */
	if ( out == 0 || r_plane == 0 || g_plane == 0 || b_plane == 0 || 
         t_plane == 0)
	{	if ( out )
			gs_free(pdev->memory, (char *)out, line_size, 1,
				"cp50_print_page(out)");
        if (r_plane)
            gs_free(pdev->memory, (char *)r_plane, X_PIXEL*Y_PIXEL, 1,
                "cp50_print_page(r_plane)");
        if (g_plane)  
            gs_free(pdev->memory, (char *)g_plane, X_PIXEL*Y_PIXEL, 1, 
                "cp50_print_page(g_plane)");
        if (b_plane)  
            gs_free(pdev->memory, (char *)b_plane, X_PIXEL*Y_PIXEL, 1, 
                "cp50_print_page(b_plane)");
        if (t_plane)
            gs_free(pdev->memory, (char *)t_plane, X_PIXEL*Y_PIXEL, 1, 
                "cp50_print_page(t_plane)");
		return -1;
	}

    /* set each plane as white */
    memset(r_plane, -1, X_PIXEL*Y_PIXEL);
    memset(g_plane, -1, X_PIXEL*Y_PIXEL);
    memset(b_plane, -1, X_PIXEL*Y_PIXEL);
    memset(t_plane, -1, X_PIXEL*Y_PIXEL);

	/* Initialize the printer */ /* see programmer manual for CP50 */
	fprintf(prn_stream,"\033\101");
    fprintf(prn_stream,"\033\106\010\001");
    fprintf(prn_stream,"\033\106\010\003");

    /* set number of copies */
    fprintf(prn_stream,"\033\116");
    num_copies = copies & 0xFF;
    fwrite(&num_copies, sizeof(char), 1, prn_stream);

    /* download image */
    hi_lines = lines >> 8;
    lo_lines = lines & 0xFF;

    fprintf(prn_stream,"\033\123\062");
    fwrite(&hi_lines, sizeof(char), 1, prn_stream);
    fwrite(&lo_lines, sizeof(char), 1, prn_stream);
    fprintf(prn_stream,"\001"); /* dummy */

	/* Print lines of graphics */
	while ( lnum <= last )
	   {
        int i, col;
		gdev_prn_copy_scan_lines(pdev, lnum, (byte *)out, line_size);
		/*fwrite(out, sizeof(char), line_size, prn_stream);*/
        for(i=0; i<X_PIXEL; i++)
        {
          col = (lnum-FIRST_LINE) * X_PIXEL + i;
          r_plane[col] = out[i*3+FIRST_COLUMN];
          g_plane[col] = out[i*3+1+FIRST_COLUMN];
          b_plane[col] = out[i*3+2+FIRST_COLUMN];
        }
		lnum ++;
	   }

    /* rotate each plane and download it */
    for(i=0;i<X_PIXEL;i++)
      for(j=Y_PIXEL-1;j>=0;j--)
        t_plane[(Y_PIXEL-1-j)+i*Y_PIXEL] = r_plane[i+j*X_PIXEL];
    fwrite(t_plane, sizeof(char), X_PIXEL*Y_PIXEL, prn_stream);

    for(i=0;i<X_PIXEL;i++)
      for(j=Y_PIXEL-1;j>=0;j--)
        t_plane[(Y_PIXEL-1-j)+i*Y_PIXEL] = g_plane[i+j*X_PIXEL]; 
    fwrite(t_plane, sizeof(char), X_PIXEL*Y_PIXEL, prn_stream);

    for(i=0;i<X_PIXEL;i++)
      for(j=Y_PIXEL-1;j>=0;j--)
        t_plane[(Y_PIXEL-1-j)+i*Y_PIXEL] = b_plane[i+j*X_PIXEL]; 
    fwrite(t_plane, sizeof(char), X_PIXEL*Y_PIXEL, prn_stream);


	gs_free(pdev->memory, (char *)out, line_size, 1, "cp50_print_page(out)");
    gs_free(pdev->memory, (char *)r_plane, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(r_plane)");
    gs_free(pdev->memory, (char *)g_plane, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(g_plane)");
    gs_free(pdev->memory, (char *)b_plane, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(b_plane)");
    gs_free(pdev->memory, (char *)t_plane, X_PIXEL*Y_PIXEL, 1, "cp50_print_page(t_plane)");

	return 0;
}

int private 
cp50_output_page(gx_device *pdev, int num_copies, int flush)
{   int code, outcode, closecode;

    code = gdev_prn_open_printer(pdev, 1);
    if ( code < 0 ) return code;

    copies = num_copies; /* using global variable to pass */

    /* Print the accumulated page description. */
    outcode = (*ppdev->printer_procs.print_page)(ppdev, ppdev->file);
    if ( code < 0 ) return code;

    closecode = gdev_prn_close_printer(pdev);
    if ( code < 0 ) return code;

    if ( ppdev->buffer_space ) /* reinitialize clist for writing */
      code = (*gs_clist_device_procs.output_page)(pdev, num_copies, flush);
 
    if ( outcode < 0 ) return outcode;
    if ( closecode < 0 ) return closecode;
    if ( code < 0 ) return code;
    return gx_finish_output_page(pdev, num_copies, flush);
}


/* 24-bit color mappers (taken from gdevmem2.c). */
/* Note that Windows expects RGB values in the order B,G,R. */
 
/* Map a r-g-b color to a color index. */
private gx_color_index
cp50_rgb_color(gx_device *dev, const gx_color_value cv[])
{   
    gx_color_value red, green, blue;

    red = cv[0]; green = cv[1]; blue = cv[2];
    return ((ulong)gx_color_value_to_byte(red) << 16)+
           ((uint)gx_color_value_to_byte(green) << 8) +
           gx_color_value_to_byte(blue);
}
 
/* Map a color index to a r-g-b color. */
private int
cp50_color_rgb(gx_device *dev, gx_color_index color,
  gx_color_value prgb[3])
{   prgb[2] = gx_color_value_from_byte(color & 0xff);
    prgb[1] = gx_color_value_from_byte((color >> 8) & 0xff);
    prgb[0] = gx_color_value_from_byte(color >> 16);
    return 0;
}
