/* Copyright (C) 1989, 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gs.c,v 1.6 2001/04/08 08:43:24 ghostgum Exp $ */
/* 'main' program for Ghostscript */
#include "ghost.h"
#include "imain.h"
#include "imainarg.h"
#include "iapi.h"
#include "iminst.h"
#include "errors.h"

/* Define an optional array of strings for testing. */
/*#define RUN_STRINGS */
#ifdef RUN_STRINGS
private const char *run_strings[] =
{
    "2 vmreclaim /SAVE save def 2 vmreclaim",
    "(saved\n) print flush",
    "SAVE restore (restored\n) print flush 2 vmreclaim",
    "(done\n) print flush quit",
    0
};

#endif

int
main(int argc, char *argv[])
{
    int exit_status = 0;
    gs_main_instance *minst = gs_main_instance_default();
    int code = gs_main_init_with_args(minst, argc, argv);

#ifdef RUN_STRINGS
    {				/* Run a list of strings (for testing). */
	const char **pstr = run_strings;

	for (; *pstr; ++pstr) {
	    int exit_code;
	    ref error_object;
	    int code;

	    fprintf(stdout, "{%s} =>\n", *pstr);
	    fflush(stdout);
	    code = gs_main_run_string(minst, *pstr, 0,
				      &exit_code, &error_object);
	    zflush(osp);
	    fprintf(stdout, " => code = %d\n", code);
	    fflush(stdout);
	    if (code < 0) {
		gs_exit(1);
		return 1;
	    }
	}
    }
#endif

    if (code >= 0)
	code = gs_main_run_start(minst);

    exit_status = 0;
    switch (code) {
	case 0:
	case e_Info:
	case e_Quit:
	    break;
	case e_Fatal:
	    exit_status = 1;
	    break;
	default:
	    exit_status = 255;
    }

    gs_exit_with_code(exit_status, code);

    return exit_status;
}
