/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsalpha.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Graphics state alpha value access */
#include "gx.h"
#include "gsalpha.h"
#include "gxdcolor.h"
#include "gzstate.h"

/* setalpha */
int
gs_setalpha(gs_state * pgs, floatp alpha)
{
    pgs->alpha =
	(gx_color_value) (alpha < 0 ? 0 : alpha > 1 ? gx_max_color_value :
			  alpha * gx_max_color_value);
    gx_unset_dev_color(pgs);
    return 0;
}

/* currentalpha */
float
gs_currentalpha(const gs_state * pgs)
{
    return (float)pgs->alpha / gx_max_color_value;
}
