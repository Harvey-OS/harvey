/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevfax.h,v 1.2 2000/09/19 19:00:13 lpd Exp $ */
/* Definitions and interface for fax devices */

#ifndef gdevfax_INCLUDED
#  define gdevfax_INCLUDED

/* Define the default device parameters. */
#define X_DPI 204
#define Y_DPI 196

/* Define the structure for fax devices. */
/* Precede this by gx_device_common and gx_prn_device_common. */
#define gx_fax_device_common\
    int AdjustWidth		/* 0 = no adjust, 1 = adjust to fax values */
typedef struct gx_device_fax_s {
    gx_device_common;
    gx_prn_device_common;
    gx_fax_device_common;
} gx_device_fax;

#define FAX_DEVICE_BODY(dtype, procs, dname, print_page)\
    prn_device_std_body(dtype, procs, dname,\
			DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,\
			X_DPI, Y_DPI,\
			0, 0, 0, 0,	/* margins */\
			1, print_page),\
    1				/* AdjustWidth */

/* Procedures defined in gdevfax.c */

/* Driver procedures */
dev_proc_open_device(gdev_fax_open);
dev_proc_get_params(gdev_fax_get_params); /* adds AdjustWidth */
dev_proc_put_params(gdev_fax_put_params); /* adds AdjustWidth */
extern const gx_device_procs gdev_fax_std_procs;

/* Other procedures */
void gdev_fax_init_state(P2(stream_CFE_state *ss, const gx_device_fax *fdev));
void gdev_fax_init_fax_state(P2(stream_CFE_state *ss,
				const gx_device_fax *fdev));
int gdev_fax_print_strip(P7(gx_device_printer * pdev, FILE * prn_stream,
			    const stream_template * temp, stream_state * ss,
			    int width, int row_first,
			    int row_end /* last + 1 */));
int gdev_fax_print_page(P3(gx_device_printer *pdev, FILE *prn_stream,
			   stream_CFE_state *ss));

#endif /* gdevfax_INCLUDED */
