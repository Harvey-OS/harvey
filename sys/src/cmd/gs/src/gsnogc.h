/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsnogc.h,v 1.2 2000/09/19 19:00:30 lpd Exp $ */
/* Interface to non-tracing GC */

#ifndef gsnogc_INCLUDED
#  define gsnogc_INCLUDED

#include "gsgc.h"		/* for vm_reclaim_proc */

/* Declare the vm_reclaim procedure for the non-tracing GC. */
extern vm_reclaim_proc(gs_nogc_reclaim);

#endif /* gsnogc_INCLUDED */
