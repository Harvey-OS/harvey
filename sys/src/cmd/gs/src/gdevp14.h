/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/*$Id: gdevp14.h,v 1.1 2001/04/01 00:30:41 raph Exp $ */
/* Definitions and interface for PDF 1.4 rendering device */

#ifndef gdevp14_INCLUDED
#  define gdevp14_INCLUDED

int
gs_pdf14_device_filter(gs_device_filter_t **pdf, int depth, gs_memory_t *mem);

#endif /* gdevp14_INCLUDED */
