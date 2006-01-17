/* Copyright (C) 1992, 1994, 1996 artofcode LLC.  All rights reserved.
  
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

/* $Id: gserver.c,v 1.7 2002/06/16 05:48:55 lpd Exp $ */
/* Server front end for Ghostscript, replacing gs.c. */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "imemory.h"		/* for iutil.h */
#include "interp.h"		/* for gs_interp_reset */
#include "iutil.h"		/* for obj_cvs */
#include "main.h"
#include "ostack.h"
#include "store.h"
#include "gspaint.h"		/* for gs_erasepage */

/*
 * This file provides a very simple procedural interface to the Ghostscript
 * PostScript/PDF language interpreter, with a rudimentary provision for
 * saving and restoring state around "jobs".
 * See below for the descriptions of individual procedures.
 *
 * All routines in this file return an integer value which is 0 if the
 * routine completed successfully, non-0 if an error occurred.
 */

/* ------ Public interface ------ */

/*
 * Initialize Ghostscript.  fno_stdin, fno_stdout, and fno_stderr are
 * file handles that Ghostscript will use in place of stdin, stdout,
 * and stderr respectively.  This routine should be called once at the
 * beginning of execution, and not after that.
 *
 * This routine establishes a "baseline" initial state for Ghostscript,
 * which includes reading in the standard Ghostscript initialization files
 * such as gs_init.ps and gs_fonts.ps.  This baseline is always used as the
 * starting state for gs_server_run_string and gs_server_run_files, unless
 * modified as described below.
 *
 * This routine 'opens' the default driver.
 */

int gs_server_initialize(int fno_stdin, int fno_stdout, int fno_stderr,
			 const char *init_str);

/*
 * Execute a string containing PostScript code.  The effects of this code
 * do modify the baseline state for future calls of ...run_string and
 * ...run_files.  There are four cases of return values:
 *      value = 0: normal return.
 *      value = e_Quit: the PostScript code executed a `quit'.
 *      value = e_Fatal: the PostScript code encountered a fatal error.
 *              *exit_code_ptr holds the C exit code.
 *      other value: the PostScript code encountered a PostScript error
 *              while processing another error, or some other fatal
 *              PostScript error.
 *
 * errstr points to a string area of length errstr_max_len for reporting
 * the PostScript object causing the error.  In the case of an error return,
 * the characters from errstr[0] through errstr[*errstr_len_ptr-1] will
 * contain a representation of the error object.  *errstr_len_ptr will not
 * exceed errstr_max_len.
 */

int gs_server_run_string(const char *str, int *exit_code_ptr,
			 char *errstr, int errstr_max_len,
			 int *errstr_len_ptr);

/*
 * Run a sequence of files containing PostScript code.  If permanent is 0,
 * the files do not affect the baseline state; if permanent is 1, they do
 * affect the baseline state, just like ...run_string.  The returned value,
 * exit code, and error string are the same as for gs_server_run_string.
 *
 * If permanent is 0, the output page buffer is cleared before running the
 * first file (equivalent to `erasepage').
 */

int gs_server_run_files(const char **file_names, int permanent,
			int *exit_code_ptr, char *errstr,
			int errstr_max_len, int *errstr_len_ptr);

/*
 * Terminate Ghostscript.  Ghostscript will release all memory and close
 * all files it has opened, including the ones referenced by fno_stdin,
 * fno_stdout, and fno_stderr.
 *
 * This routine 'closes' the default driver.
 */

int gs_server_terminate();

/* ------ Example of use ------ */

/* To run this example, change the 0 to 1 in the following line. */
#if 0

/*
 * This example predefines the name fubar, prints out its value,
 * and then renders the golfer art file supplied with Ghostscript.
 */

#include <fcntl.h>
#include <sys/stat.h>
int
main(int argc, char *argv[])
{
    int code, exit_code;

#define emax 50
    char errstr[emax + 1];
    int errlen;
    static const char *fnames[] =
    {"golfer.eps", 0};
    FILE *cin = fopen("stdin.tmp", "w+");
    int sout = open("stdout.tmp", O_WRONLY | O_CREAT | O_TRUNC,
		    S_IREAD | S_IWRITE);
    int serr = open("stderr.tmp", O_WRONLY | O_CREAT | O_TRUNC,
		    S_IREAD | S_IWRITE);

    code = gs_server_initialize(fileno(cin), sout, serr,
				"/fubar 42 def");
    fprintf(stdout, "init: code %d\n", code);
    if (code < 0)
	goto x;
    code = gs_server_run_string("fubar == flush", &exit_code,
				errstr, emax, &errlen);
    fprintf(stdout, "print: code %d\n", code);
    if (code < 0)
	goto x;
    code = gs_server_run_files(fnames, 0, &exit_code,
			       errstr, emax, &errlen);
    fprintf(stdout, "golfer: code %d\n", code);
    if (code < 0)
	goto x;
    errlen = 0;
    code = gs_server_run_string("fubar 0 div", &exit_code,
				errstr, emax, &errlen);
    errstr[errlen] = 0;
    fprintf(stdout, "0 div: code %d object %s\n", code, errstr);
    errlen = 0;
    code = gs_server_run_string("xxx", &exit_code,
				errstr, emax, &errlen);
    errstr[errlen] = 0;
    fprintf(stdout, "undef: code %d object %s\n", code, errstr);
  x:code = gs_server_terminate();
    fprintf(stdout, "end: code %d\n", code);
    fflush(stdout);
    close(serr);
    close(sout);
    fclose(cin);
    return code;
}

#endif

/* ------ Private definitions ------ */

/* Forward references */
private int job_begin(void);
private int job_end(void);
private void errstr_report(ref *, char *, int, int *);

/* ------ Public routines ------ */

/* Initialize Ghostscript. */

int
gs_server_initialize(int fno_stdin, int fno_stdout, int fno_stderr,
		     const char *init_str)
{
    int code, exit_code;	/* discard exit_code for now */
    int errstr_len;		/* discard */
    FILE *c_stdin, *c_stdout, *c_stderr;

    /* Establish C-compatible files for stdout and stderr. */
    c_stdin = fdopen(fno_stdin, "r");
    if (c_stdin == NULL)
	return -1;
    c_stdout = fdopen(fno_stdout, "w");
    if (c_stdout == NULL)
	return -1;
    c_stderr = fdopen(fno_stderr, "w");
    if (c_stderr == NULL)
	return -1;
    /* Initialize the Ghostscript interpreter. */
    if ((code = gs_init0(c_stdin, c_stdout, c_stderr, 0)) < 0 ||
	(code = gs_init1()) < 0 ||
	(code = gs_init2()) < 0
	)
	return code;
    code = gs_server_run_string("/QUIET true def /NOPAUSE true def",
				&exit_code,
				(char *)0, 0, &errstr_len);
    if (code < 0)
	return code;
    return (init_str == NULL ? 0 :
	    gs_server_run_string(init_str, &exit_code,
				 (char *)0, 0, &errstr_len));
}

/* Run a string. */

int
gs_server_run_string(const char *str, int *exit_code_ptr,
		     char *errstr, int errstr_max_len, int *errstr_len_ptr)
{
    ref error_object;
    int code;

    make_tasv(&error_object, t_string, 0, 0, bytes, 0);
    code = gs_run_string(str, 0, exit_code_ptr, &error_object);
    if (code < 0)
	errstr_report(&error_object, errstr, errstr_max_len,
		      errstr_len_ptr);
    return code;
}

/* Run files. */

int
gs_server_run_files(const char **file_names, int permanent,
  int *exit_code_ptr, char *errstr, int errstr_max_len, int *errstr_len_ptr)
{
    int code = 0;
    ref error_object;
    const char **pfn;

    if (!permanent)
	job_begin();
    make_tasv(&error_object, t_string, 0, 0, bytes, 0);
    for (pfn = file_names; *pfn != NULL && code == 0; pfn++)
	code = gs_run_file(*pfn, 0, exit_code_ptr, &error_object);
    if (!permanent)
	job_end();
    if (code < 0)
	errstr_report(&error_object, errstr, errstr_max_len,
		      errstr_len_ptr);
    return code;
}

/* Terminate Ghostscript. */

int
gs_server_terminate()
{
    gs_finit(0, 0);
    return 0;
}

/* ------ Private routines ------ */

private ref job_save;		/* 'save' object for baseline state */

extern int zsave(os_ptr), zrestore(os_ptr);

/* Start a 'job' by restoring the baseline state. */

private int
job_begin()
{
    int code;

    /* Ghostscript doesn't provide erasepage as an operator. */
    /* However, we can get the same effect by calling gs_erasepage. */
    extern gs_state *igs;

    if ((code = gs_erasepage(igs)) < 0)
	return code;
    code = zsave(osp);
    if (code == 0)
	job_save = *osp--;
    return code;
}

/* End a 'job'. */

private int
job_end()
{
    gs_interp_reset();
    *++osp = job_save;
    return zrestore(osp);
}

/* Produce a printable representation of an error object. */

private void
errstr_report(ref * perror_object, char *errstr, int errstr_max_len,
	      int *errstr_len_ptr)
{
    int code = obj_cvs(perror_object, (byte *) errstr,
		       (uint) errstr_max_len, (uint *) errstr_len_ptr,
		       false);

    if (code < 0) {
	const char *ustr = "[unprintable]";
	int len = min(strlen(ustr), errstr_max_len);

	memcpy(errstr, ustr, len);
	*errstr_len_ptr = len;
    }
}
