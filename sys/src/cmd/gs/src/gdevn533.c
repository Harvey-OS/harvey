/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1989, 1990, 1991, 1993, 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevn533.c,v 1.6 2004/08/10 13:02:36 stefan Exp $*/
/* Sony NWP-533 driver for GhostScript */
#include "gdevprn.h"
#define prn_dev ((gx_device_printer *)dev) /* needed in 5.31 et seq */
#include <sys/ioctl.h>
#include <newsiop/lbp.h>

/***
 *** Note: this driver was contributed by a user, Tero Kivinen:
 ***       please contact kivinen@joker.cs.hut.fi if you have questions.
 ***/

#define A4_PAPER 1

#ifdef A4_PAPER
#define PAPER_XDOTS A4_XDOTS
#define PAPER_YDOTS A4_YDOTS
#else
#define PAPER_XDOTS B4_XDOTS
#define PAPER_YDOTS B4_YDOTS
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* The device descriptor */
private
dev_proc_open_device(nwp533_open);
private
dev_proc_print_page(nwp533_print_page);
private
dev_proc_close_device(nwp533_close);
private
gx_device_procs nwp533_procs =
    prn_procs(nwp533_open, gdev_prn_output_page, nwp533_close);

const gx_device_printer far_data gs_nwp533_device =
    prn_device(nwp533_procs, "nwp533",
	       PAPER_XDOTS * 10.0 / DPI, /* width_10ths */
	       PAPER_YDOTS * 10.0 / DPI, /* height_10ths */
	       DPI,			 /* x_dpi */
	       DPI,			 /* y_dpi */
	       0, 0, 0, 0,		 /* margins */
	       1, nwp533_print_page);

/* return True if should retry - False if should quit */
private
int
analyze_error(int printer_file)
{
	struct lbp_stat status;
	char *detail = NULL, *old_detail = NULL;
	int waiting = TRUE;
	int retry_after_return = TRUE;

	if(ioctl(printer_file, LBIOCRESET, 0) < 0) {
		perror("ioctl(LBIOCRESET)");
		return FALSE;
	}
	if(ioctl(printer_file, LBIOCSTATUS, &status) < 0) {
		perror("ioctl(LBIOCSTATUS)");
		return FALSE;
	}

	do {
		/* Is there an error */
		if(status.stat[0] & (ST0_CALL | ST0_REPRINT_REQ | ST0_WAIT | ST0_PAUSE)) {
			if(status.stat[1] & ST1_NO_CARTRIGE) /* mispelled? */
				detail = "No cartridge - waiting";
			else if(status.stat[1] & ST1_NO_PAPER)
				detail = "Out of paper - waiting";
			else if(status.stat[1] & ST1_JAM)
				detail = "Paper jam - waiting";
			else if(status.stat[1] & ST1_OPEN)
				detail = "Door open - waiting";
			else if(status.stat[1] & ST1_TEST)
				detail = "Test printing - waiting";
			else {
				waiting = FALSE;
				retry_after_return = FALSE;

				if(status.stat[2] & ST2_FIXER)
					detail = "Fixer trouble - quiting";
				else if(status.stat[2] & ST2_SCANNER)
					detail = "Scanner trouble - quiting";
				else if(status.stat[2] & ST2_MOTOR)
					detail = "Scanner motor trouble - quiting";
				else if(status.stat[5] & ST5_NO_TONER)
					detail = "No toner - quiting";
			}
		} else {
			waiting = FALSE;
		}
		if(detail != NULL && detail != old_detail) {
			perror(detail);
			old_detail = detail;
		}
		if(waiting) {
			ioctl(1, LBIOCRESET, 0);
			sleep(5);
			ioctl(1, LBIOCSTATUS, &status);
		}
	} while(waiting);
	return retry_after_return;
}

private
int
nwp533_open(gx_device *dev)
{
	gx_device_printer *pdev = (gx_device_printer *)dev;

	if(pdev->fname[0] == '\0') {
		strcpy(pdev->fname, "/dev/lbp");
	}
	return gdev_prn_open(dev);
}

private
int
nwp533_close(gx_device *dev)
{
	if(((gx_device_printer *)dev)->file != NULL) {
		int printer_file;

		printer_file = fileno(((gx_device_printer *)dev)->file);
	restart2:
		if(ioctl(printer_file, LBIOCSTOP, 0) < 0) {
			if(analyze_error(printer_file))
				goto restart2;
			perror("Waiting for device");
			return_error(gs_error_ioerror);
		}
	}
	return gdev_prn_close(dev);
}

/* Send the page to the printer. */
private
int
nwp533_print_page(gx_device_printer *dev, FILE *prn_stream)
{
	int lnum;
	int line_size = gdev_mem_bytes_per_scan_line(dev);
	byte *in;
	int printer_file;
	printer_file = fileno(prn_stream);

	if(line_size % 4 != 0) {
		line_size += 4 - (line_size % 4);
	}
	in = (byte *)gs_malloc(dev->memory, line_size, 1, "nwp533_output_page(in)");
restart:
	if(ioctl(printer_file, LBIOCSTOP, 0) < 0) {
		if(analyze_error(printer_file))
			goto restart;
		perror("Waiting for device");
		return_error(gs_error_ioerror);
	}
	lseek(printer_file, 0, 0);

	for(lnum = 0; lnum < dev->height; lnum++) {
		gdev_prn_copy_scan_lines(prn_dev, lnum, in, line_size);
		if(write(printer_file, in, line_size) != line_size) {
			perror("Writting to output");
			return_error(gs_error_ioerror);
		}
	}
retry:
	if(ioctl(printer_file, LBIOCSTART, 0) < 0) {
		if(analyze_error(printer_file))
			goto retry;
		perror("Starting print");
		return_error(gs_error_ioerror);
	}
	gs_free(dev->memory, in, line_size, 1, "nwp533_output_page(in)");

	return 0;
}
