/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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
/* Portions Copyright (C) 1994, Russell Lang.  All rights reserved. */


/* gsdll.c */
/* Dynamic Link Library interface for OS/2 and MS-Windows Ghostscript */
/* derived from gs.c */

/* Define PROGRAM_NAME before we include std.h */
#define PROGRAM_NAME gs_product
#include "ctype_.h"
#include "memory_.h"
#include <stdarg.h>
#include "string_.h"
/* Capture stdin/out/err before gs.h redefines them. */
#include <stdio.h>
static FILE *real_stdin, *real_stdout, *real_stderr;
static void
get_real(void)
{	real_stdin = stdin, real_stdout = stdout, real_stderr = stderr;
}
#include "ghost.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "stream.h"
#include "errors.h"
#include "estack.h"
#include "ialloc.h"
#include "sfilter.h"		/* for iscan.h */
#include "ostack.h"		/* must precede iscan.h */
#include "iscan.h"
#include "main.h"
#include "store.h"
#include "files.h"				/* requires stream.h */
#include "interp.h"
#include "gsdll.h"		/* header for DLLs */

#ifndef GS_LIB
#  define GS_LIB "GS_LIB"
#endif

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));

/* Other imported data */
extern const char **gs_lib_paths;
extern const char *gs_product;
extern const char *gs_copyright;
extern const long gs_revision;
extern const long gs_revisiondate;

/* Forward references */
int gsdll_dodef(char sw, char *arg);

/* callback for messages and stdio to caller */
GSDLL_CALLBACK pgsdll_callback;
int gsdll_usage;


/* ---------- DLL exported functions ---------- */

/* arguments are:
 * callback function for stdio and for notification of 
 *   sync_output, output_page and resize events
 * a string containing a subset of Ghostscript command line options,
 *   each separated by a \0 and terminated by \0\0
 */
int DLL_EXPORT 
gsdll_init(GSDLL_CALLBACK callback, char *str)
{	
int argc = 0;
char *p;
#if defined(_Windows) && !defined(__WIN32__)
	if (gsdll_usage)
	    return -1;	/* DLL can't be used by multiple programs under Win16 */
#endif
	gsdll_usage++;

	pgsdll_callback = callback;

	p = str;
	while (*p) {	/* count arguments */
	    argc++;
	    p += strlen(p) + 1;
	}

	get_real();
	gs_init0(real_stdin, real_stdout, real_stderr, argc);
	   {	char *lib = getenv(GS_LIB);
		if ( lib != 0 ) 
		   {	int len = strlen(lib);
			gs_lib_env_path = gs_malloc(len + 1, 1, "GS_LIB");
			strcpy(gs_lib_env_path, lib);
		   }
	   }
	/* defines that must be made before gs_init.ps */
	{
	    char sw;
	    char *next;
	    p = str;
	    while (*p) {
	        next = p + strlen(p) + 1;
		if (*p == '-') {
		    sw = *(p+1);
		    if (sw=='d' || sw=='D' || sw=='s' || sw=='S')
		        gsdll_dodef(sw, p+2);
		    else if (sw=='I') {
			char *path = gs_malloc(strlen(p+2)+ 1, 1, "gsdll_init");
			strcpy(path, p+2);
			gs_add_lib_path(path);
		    }
		    else if (sw=='Z') {
			char *arg = p+2;
			while (*arg)
			    gs_debug[*arg++ & 127] = 0xff;
		    }
		    else
		        fprintf(stdout, "gsdll_init: unknown argument. Only -I -d -D -s -S supported.\n");
		}
		else {
		    fprintf(stdout, "gsdll_init: arguments must begin with '-'\n");
		}
		p = next;
	    }
	}

	gs_init2();

	return 0;
}


/* if return value < 0, then error occured and caller should call */
/* gsdll_exit, then unload library */
int DLL_EXPORT
gsdll_execute(char *str)
{	int exit_code;
	ref error_object;
	int code;
	code = gs_run_string(str, gs_user_errors, &exit_code, &error_object);
	return code;
}

int DLL_EXPORT
gsdll_exit(void)
{
#if defined(_Windows) && !defined(__WIN32__)
	/* don't alter gsdll_usage */
	/* DLL must be unloaded before it can be used again */
#else
	gsdll_usage--;
#endif
	/* don't call gs_exit() since this would cause caller to exit */
	gs_finit(0, 0);
	pgsdll_callback = (GSDLL_CALLBACK)NULL;
	return 0;
}

/* return revision numbers and strings of Ghostscript */
/* Used for determining if wrong GSDLL loaded */
/* this may be called before gsdll_init */
int DLL_EXPORT
gsdll_revision(char **product, char **copyright, long *revision, long *revisiondate)
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

/* ---------- DLL internal functions ---------- */

int
gsdll_dodef(char sw, char *arg)
{
char *adef;
char *eqp;
bool isd;
ref value;
    if (*arg == '\0')
	return 0;
    /* copy argument to heap */
    adef = gs_malloc(strlen(arg) + 1, 1, "gsdll_dodef");
    if (adef == (char *)NULL) {
	lprintf("Out of memory!\n");
	return -1;
    }
    strcpy(adef, arg);
    eqp = strchr(adef, '=');
    isd = (sw == 'D' || sw == 'd');
	
		if ( eqp == NULL )
			eqp = strchr(adef, '#');
		/* Initialize the object memory, scanner, and */
		/* name table now if needed. */
		gs_init1();
		if ( eqp == adef )
		   {
			fprintf(stdout, "gsdll_dodef: Usage: dname, dname=token, sname=string\n");
			return -1;
		   }
		if ( eqp == NULL )
		   {
			if ( isd )
				make_true(&value);
			else
				make_string(&value, a_readonly, 0, NULL);
		   }
		else
		   {	int code;
			*eqp++ = 0;
			if ( isd )
			   {	stream astream;
				scanner_state state;
				sread_string(&astream,
					     (const byte *)eqp, strlen(eqp));
				scanner_state_init(&state, false);
				code = scan_token(&astream, &value, &state);
				if ( code )
				   {	fprintf(stdout, "gsdll_dodef: dname= must be followed by a valid token\n");
					return -1;
				   }
			   }
			else
			   {	int len = strlen(eqp);
				char *str = gs_malloc((uint)len, 1, "-s");
				if ( str == 0 )
				   {	lprintf("Out of memory!\n");
					return -1;
				   }
				memcpy(str, eqp, len);
				make_const_string(&value, a_readonly + a_foreign, len, (const byte *)str);
			   }
		   }
		/* Enter the name in systemdict. */
		initial_enter_name(adef, &value);
	return 0;
}

/* end gsdll.c */
