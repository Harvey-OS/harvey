/* Copyright (C) 1990, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/*
 * gdev8510.c
 *
 * C.Itoh M8510 printer driver for ghostscript.
 * by Bob Smith <bob@snuffy.penfield.ny.us>
 */

#include "gdevprn.h"

/* The device descriptor */
private dev_proc_print_page(m8510_print_page);
gx_device_printer far_data gs_m8510_device =
	prn_device(prn_std_procs, "m8510",
		85,				/* width_10ths, 8.5" */
		110,				/* height_10ths, 11" */
		160,				/* x_dpi */
		144,				/* y_dpi */
		0,0,0.5,0,			/* left, bottom, right, and top margins */
		1, m8510_print_page);

/* ------ forward declarations ------ */

private void m8510_output_run(P4(gx_device_printer *pdev,
	byte *out, int pass, FILE *prn_stream));

/* ------ internal routines ------ */

/* Send the page to the printer. */
private int
m8510_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	byte *in1 = (byte *) gs_malloc(8, line_size, "m8510_print_page(in1)");
	byte *in2 = (byte *) gs_malloc(8, line_size, "m8510_print_page(in2)");
	byte *out = (byte *) gs_malloc(8, line_size, "m8510_print_page(out)");
	int lnum = 0;
	int code = 0;
	byte *inp, *in_end, *outp;
	int i;

	if (in1 == 0 || in2 == 0 || out == 0) {
		code = gs_error_VMerror;
		gs_note_error(code);
		goto out;
	}

	/*
	 * Initialize the printer.
	 * NLQ mode, proportional print (160x144 dpi).
	 * and 16/144" linefeeds.
	 */
	fwrite("\033m2\033P\033T16", 1, 9, prn_stream);

	/* Transfer pixels to printer */
	while ( lnum < pdev->height ) {
		/* get a raster */
		for (i = 7; i >= 0; i--) {
			gdev_prn_copy_scan_lines(pdev, lnum, &in1[i*line_size], line_size);
			lnum++;
			gdev_prn_copy_scan_lines(pdev, lnum, &in2[i*line_size], line_size);
			lnum++;
		}

		/* Transpose the 1st pass of data. */
		in_end = in1 + line_size;
		for (inp = in1, outp = out; inp < in_end; inp++, outp += 8)
			gdev_prn_transpose_8x8(inp, line_size, outp, 1);

		/* send the 1st line */
		m8510_output_run(pdev, out, 0, prn_stream);

		/* Transpose the 2nd pass of data. */
		in_end = in2 + line_size;
		for (inp = in2, outp = out; inp < in_end; inp++, outp += 8)
			gdev_prn_transpose_8x8(inp, line_size, outp, 1);

		/* send the 2nd line */
		m8510_output_run(pdev, out, 1, prn_stream);
	}

	/* reset the printer. */
	fwrite("\033c1", 1, 3, prn_stream);
	fflush(prn_stream);

out:;
	if (out) gs_free((char *) out, 8, line_size, "m8510_print_page(out)");
	if (in2) gs_free((char *) in2, 8, line_size, "m8510_print_page(in2)");
	if (in1) gs_free((char *) in1, 8, line_size, "m8510_print_page(in1)");

	return code;
}

private void
m8510_output_run(gx_device_printer *pdev,
	byte *out, int pass, FILE *prn_stream)
{
	byte *out_end = out + pdev->width;
	char tmp[10];
	int count;

	/*
	 * Remove trailing 0s.
	 * out must be a multiple of 8 bytes.
	 */
	while (out_end > out
		&& out_end[-1] == 0
		&& out_end[-2] == 0
		&& out_end[-3] == 0
		&& out_end[-4] == 0
		&& out_end[-5] == 0
		&& out_end[-6] == 0
		&& out_end[-7] == 0
		&& out_end[-8] == 0)
			out_end -= 8;

	/* Transfer the line of data. */
	count = out_end - out;
	if (count) {
		sprintf(tmp, "\033g%03d", count/8);
		fwrite(tmp, 1, 5, prn_stream);
		fwrite(out, 1, count, prn_stream);
		fwrite("\r", 1, 1, prn_stream);
	}

	if (pass) fwrite("\n", 1, 1, prn_stream);
}
