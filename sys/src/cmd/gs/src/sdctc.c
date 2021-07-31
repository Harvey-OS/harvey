/* Copyright (C) 1994, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sdctc.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Code common to DCT encoding and decoding streams */
#include "stdio_.h"
#include "jpeglib_.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "strimpl.h"
#include "sdct.h"

public_st_DCT_state();

/* Set the defaults for the DCT filters. */
void
s_DCT_set_defaults(stream_state * st)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;

    ss->jpeg_memory = &gs_memory_default;
    ss->data.common = 0;
	/****************
	  ss->data.common->Picky = 0;
	  ss->data.common->Relax = 0;
	 ****************/
    ss->ColorTransform = -1;
    ss->QFactor = 1.0;
    /* Clear pointers */
    ss->Markers.data = 0;
    ss->Markers.size = 0;
}
