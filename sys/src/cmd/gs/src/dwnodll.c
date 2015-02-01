/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1996-2000 Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* dwnodll.c */
/* $Id: dwnodll.c,v 1.6 2004/04/08 16:18:25 giles Exp $ */

#define STRICT
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "stdpre.h"
#include "gpgetenv.h"
#include "gscdefs.h"
#define GSREVISION gs_revision

#include "dwdll.h"

/* We are static linking, so just store the function addresses */
int load_dll(GSDLL *gsdll, char *last_error, int len)
{
    gsdll->new_instance = &gsapi_new_instance;
    gsdll->delete_instance = &gsapi_delete_instance;
    gsdll->set_stdio = &gsapi_set_stdio;
    gsdll->set_poll = &gsapi_set_poll;
    gsdll->set_display_callback = &gsapi_set_display_callback;
    gsdll->init_with_args = &gsapi_init_with_args;
    gsdll->run_string = &gsapi_run_string;
    gsdll->exit = &gsapi_exit;
    gsdll->set_visual_tracer = &gsapi_set_visual_tracer;
    return 0;
}

void unload_dll(GSDLL *gsdll)
{
}
