/* Copyright (C) 1996-2000 Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* dwnodll.c */

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
    return 0;
}

void unload_dll(GSDLL *gsdll)
{
}
