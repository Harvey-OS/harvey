/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevtfax.h,v 1.4 2000/09/19 19:00:23 lpd Exp $ */
/* Entry points to the TIFF/fax writing driver */

#ifndef gdevtfax_INCLUDED
#  define gdevtfax_INCLUDED

int gdev_fax_print_page_stripped(P4(gx_device_printer *pdev, FILE *prn_stream,
				    stream_CFE_state *ss, long rows_per_strip));

#endif /* gdevtfax_INCLUDED */
