/* Copyright (C) 1989, 2001 artofcode, LLC.  All rights reserved.
  
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
/* Portions Copyright (C) 1994-2000 Ghostgum Software Pty Ltd.  All rights reserved. */


/*$Id: gsdll.c,v 1.6 2001/07/23 05:25:08 giles Exp $ */
/* Dynamic Link Library interface for OS/2 and MS-Windows Ghostscript */
/* front end to gs.c */

/* This has been reimplemented to call the new DLL interface in iapi.h */


#ifdef _Windows
#include <windows.h>
#endif
#ifdef __OS2__
#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#endif

#include "stdpre.h"
#include "iapi.h"	/* Ghostscript interpreter public interface */
#include "string_.h"
#include "errors.h"
#include "gscdefs.h"
#include "gstypes.h"
#include "iref.h"
#include "iminst.h"
#include "imain.h"

#include "gsdll.h"	/* old DLL public interface */

/* MacGSView still requires that hwnd be exported
   through the old dll interface. We do that here,
   but expect to remove it when that client has been
   ported to the gsapi interface. */
#ifdef __MACOS__
extern HWND hwndtext;
#endif

/****** SINGLE-INSTANCE HACK ******/
/* GLOBAL WARNING */
GSDLL_CALLBACK pgsdll_callback;	/* callback for messages and stdio to caller */
/****** SINGLE-INSTANCE HACK ******/


/* local functions */
private int GSDLLCALL gsdll_old_stdin(void *caller_handle, char *buf, int len);
private int GSDLLCALL gsdll_old_stdout(void *caller_handle, const char *str, int len);
private int GSDLLCALL gsdll_old_stderr(void *caller_handle, const char *str, int len);
private int GSDLLCALL gsdll_old_poll(void *caller_handle);


/* ---------- DLL exported functions ---------- */

/* arguments are:
 * 1. callback function for stdio and for notification of 
 *   sync_output, output_page and resize events
 * 2. window handle, used as parent.  Use NULL if you have no window.
 * 3. argc
 * 4. argv
 */
int GSDLLEXPORT GSDLLAPI
gsdll_init(GSDLL_CALLBACK callback, HWND hwnd, int argc, char * argv[])
{
    int code;
    gs_main_instance *minst;
    if ((code = gsapi_new_instance(&minst, (void *)1)) < 0)
	return -1;

    gsapi_set_stdio(minst, 
	gsdll_old_stdin, gsdll_old_stdout, gsdll_old_stderr);
    gsapi_set_poll(minst, gsdll_old_poll);
    /* ignore hwnd */

/* rest of MacGSView compatibilty hack */
#ifdef __MACOS__
	hwndtext=hwnd;
#endif

/****** SINGLE-INSTANCE HACK ******/
    pgsdll_callback = callback;
/****** SINGLE-INSTANCE HACK ******/

    code = gsapi_init_with_args(minst, argc, argv);
    if (code == e_Quit) {
	gsapi_exit(minst);
	return GSDLL_INIT_QUIT;
    }
    return code;
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_exit, then unload library */
int GSDLLEXPORT GSDLLAPI
gsdll_execute_begin(void)
{
    int exit_code;
    return gsapi_run_string_begin(gs_main_instance_default(), 0, &exit_code);
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_execute_end, then gsdll_exit, then unload library */
int GSDLLEXPORT GSDLLAPI
gsdll_execute_cont(const char * str, int len)
{
    int exit_code;
    int code = gsapi_run_string_continue(gs_main_instance_default(), str, len, 
	0, &exit_code);
    if (code == e_NeedInput)
	code = 0;		/* this is not an error */
    return code;
}

/* if return value < 0, then error occured and caller should call */
/* gsdll_exit, then unload library */
int GSDLLEXPORT GSDLLAPI
gsdll_execute_end(void)
{
    int exit_code;
    return gsapi_run_string_end(gs_main_instance_default(), 0, &exit_code);
}

int GSDLLEXPORT GSDLLAPI
gsdll_exit(void)
{
    int code = gsapi_exit(gs_main_instance_default());
    gsapi_delete_instance(gs_main_instance_default());
    return code;
}

/* Return revision numbers and strings of Ghostscript. */
/* Used for determining if wrong GSDLL loaded. */
/* This may be called before any other function. */
int GSDLLEXPORT GSDLLAPI
gsdll_revision(const char ** product, const char ** copyright,
	       long * revision, long * revisiondate)
{
    if (product)
	*product = gs_product;
    if (copyright)
	*copyright = gs_copyright;
    if (revision)
	*revision = gs_revision;
    if (revisiondate)
	*revisiondate = gs_revisiondate;
    return 0;
}


private int GSDLLCALL
gsdll_old_stdin(void *caller_handle, char *buf, int len)
{
    return (*pgsdll_callback)(GSDLL_STDIN, buf, len);
}
private int GSDLLCALL
gsdll_old_stdout(void *caller_handle, const char *str, int len)
{
    return (*pgsdll_callback)(GSDLL_STDOUT, (char *)str, len);
}

private int GSDLLCALL
gsdll_old_stderr(void *caller_handle, const char *str, int len)
{
    return (*pgsdll_callback)(GSDLL_STDOUT, (char *)str, len);
}

private int GSDLLCALL
gsdll_old_poll(void *caller_handle)
{
    return (*pgsdll_callback)(GSDLL_POLL, NULL, 0);
}

/* end gsdll.c */
