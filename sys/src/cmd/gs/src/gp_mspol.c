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

/*$Id: gp_mspol.c,v 1.1 2001/09/22 07:07:02 ghostgum Exp $ */
/*
 * Microsoft Windows platform polling support for Ghostscript.
 *
 */

#include "gx.h"
#include "gp.h"
#include "gpcheck.h"
#include "iapi.h"
#include "iref.h"
#include "iminst.h"
#include "imain.h"

/* ------ Process message loop ------ */
/* 
 * Check messages and interrupts; return true if interrupted.
 * This is called frequently - it must be quick!
 */
#ifdef CHECK_INTERRUPTS
int
gp_check_interrupts(void)
{
    gs_main_instance *minst = gs_main_instance_default();
    if (minst && minst->poll_fn)
	return (*minst->poll_fn)(minst->caller_handle);
    return 0;
}
#endif
