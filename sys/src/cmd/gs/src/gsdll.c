/* Copyright (C) 1989, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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
/* Portions Copyright (C) 1994, 1995, 1996, Russell Lang.  All rights reserved. */


/*$Id: gsdll.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Dynamic Link Library interface for OS/2 and MS-Windows Ghostscript */
/* front end to gs.c */

/* HOW MANY OF THESE INCLUDES ARE REALLY NEEDED? */

#include "ctype_.h"
#include "memory_.h"
#include <stdarg.h>
#include "string_.h"

#include "ghost.h"
#include "gscdefs.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "stream.h"
#include "errors.h"
#include "estack.h"
#include "ialloc.h"
#include "ivmspace.h"
#include "sfilter.h"		/* for iscan.h */
#include "ostack.h"		/* must precede iscan.h */
#include "iscan.h"
#include "main.h"
#include "imainarg.h"
#include "store.h"
#include "files.h"		/* requires stream.h */
#include "interp.h"

/*
 * The following block of platform-specific conditionals is contrary to
 * Ghostscript's portability policy.  It is here because it doesn't
 * represent any useful abstraction, so isn't a good candidate for
 * encapsulation in a header file either.
 */

#ifdef _Windows
#include "windows_.h"
#ifndef __WIN32__
#define GSDLLEXPORT _export
#endif
#else  /* !_Windows */

#ifdef __OS2__
#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#else  /* !__OS2__ */

#ifdef __MACINTOSH__
/* Nothing special for the Mac. */
#endif  /* !__MACINTOSH__ */

#endif  /* !__OS2__ */

#endif  /* !_Windows */

/*
 * End of platform-specific conditionals.
 */

#include "gsdll.h"		/* header for DLLs */
#include <setjmp.h>

jmp_buf gsdll_env;		/* used by gp_do_exit in DLL configurations */

private int gsdll_usage;	/* should be needed only for 16-bit SHARED DATA */

/****** SINGLE-INSTANCE HACK ******/
private gs_main_instance *gsdll_minst;	/* instance data */
extern HWND hwndtext;		/* in gp_mswin.c, gp_os2.c, or gp_mac.c */
GSDLL_CALLBACK pgsdll_callback;	/* callback for messages and stdio to caller */


/* ---------- DLL exported functions ---------- */

/* arguments are:
 * 1. callback function for stdio and for notification of 
 *   sync_output, output_page and resize events
 * 2. window handle, used as parent.  Use NULL if you have no window.
 * 3. argc
 * 4. argv
 */
int GSDLLAPI
gsdll_init(GSDLL_CALLBACK callback, HWND hwnd, int argc, char GSFAR * argv[])
{
    if (gsdll_usage) {
	return GSDLL_INIT_IN_USE;	/* DLL can't be used by multiple programs under Win16 */
    }
    gsdll_usage++;

    if (setjmp(gsdll_env)) {
	gsdll_usage--;
	if (gs_exit_status)
	    return gs_exit_status;	/* error */
	return GSDLL_INIT_QUIT;	/* not an error */
    }
/****** SINGLE-INSTANCE HACK ******/
    pgsdll_callback = callback;
    hwndtext = hwnd;

/****** SINGLE-INSTANCE HACK ******/
    gsdll_minst = gs_main_instance_default();

    /* in gs.c */
    gs_main_init_with_args(gsdll_minst, argc, argv);

    return 0;
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_exit, then unload library */
int GSDLLAPI
gsdll_execute_begin(void)
{
    int exit_code;
    ref error_object;
    int code;

    if (!gsdll_usage)
	return -1;
    if (setjmp(gsdll_env))
	return gs_exit_status;	/* error */
    code = gs_main_run_string_begin(gsdll_minst, 0, &exit_code, &error_object);
    return code;
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_execute_end, then gsdll_exit, then unload library */
int GSDLLAPI
gsdll_execute_cont(const char GSFAR * str, int len)
{
    int exit_code;
    ref error_object;
    int code;

    if (!gsdll_usage)
	return -1;
    if (setjmp(gsdll_env))
	return gs_exit_status;	/* error */
    code = gs_main_run_string_continue(gsdll_minst, str, len, 0, &exit_code, &error_object);
    if (code == e_NeedInput)
	code = 0;		/* this is not an error */
    return code;
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_exit, then unload library */
int GSDLLAPI
gsdll_execute_end(void)
{
    int exit_code;
    ref error_object;
    int code;

    if (!gsdll_usage)
	return -1;
    if (setjmp(gsdll_env))
	return gs_exit_status;	/* error */
    code = gs_main_run_string_end(gsdll_minst, 0, &exit_code, &error_object);
    return code;
}

int GSDLLAPI
gsdll_exit(void)
{
    if (!gsdll_usage)
	return -1;
    gsdll_usage--;

    if (setjmp(gsdll_env))
	return gs_exit_status;	/* error */
    /* don't call gs_exit() since this would cause caller to exit */
    gs_finit(0, 0);
    pgsdll_callback = (GSDLL_CALLBACK) NULL;
    return 0;
}

/* return revision numbers and strings of Ghostscript */
/* Used for determining if wrong GSDLL loaded */
/* this may be called before gsdll_init */
int GSDLLAPI
gsdll_revision(char GSFAR ** product, char GSFAR ** copyright,
	       long GSFAR * revision, long GSFAR * revisiondate)
{
    if (product)
	*product = (char *)gs_product;
    if (copyright)
	*copyright = (char *)gs_copyright;
    if (revision)
	*revision = gs_revision;
    if (revisiondate)
	*revisiondate = gs_revisiondate;
    return 0;
}

/* end gsdll.c */
