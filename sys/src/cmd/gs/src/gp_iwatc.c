/* Copyright (C) 1991, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_iwatc.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Intel processor, Watcom C-specific routines for Ghostscript */
#include "dos_.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "stat_.h"
#include "string_.h"
#include "gx.h"
#include "gp.h"

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));
extern char *mktemp(P1(char *));	/* in gp_mktmp.c */

/* Define a substitute for stdprn (see below). */
private FILE *gs_stdprn;

/* Forward declarations */
private void handle_FPE(P1(int));

/* Do platform-dependent initialization. */
void
gp_init(void)
{
    gs_stdprn = 0;
    /* Set up the handler for numeric exceptions. */
    signal(SIGFPE, handle_FPE);
    gp_init_console();
}

/* Trap numeric exceptions.  Someday we will do something */
/* more appropriate with these. */
private void
handle_FPE(int sig)
{
    eprintf("Numeric exception:\n");
    exit(1);
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

/* Exit the program. */
void
gp_do_exit(int exit_status)
{
    exit(exit_status);
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
extern void gp_set_file_binary(P2(int, int));
FILE *
gp_open_printer(char fname[gp_file_name_sizeof], int binary_mode)
{
    FILE *pfile;

    if (strlen(fname) == 0 || !strcmp(fname, "PRN")) {
#ifdef stdprn
	if (!binary_mode)
	    return stdprn;
	if (gs_stdprn == 0) {
	    /* We have to effectively reopen the printer, */
	    /* because the Watcom library does \n -> \r\n */
	    /* substitution on the stdprn stream. */
	    int fno = dup(fileno(stdprn));

	    setmode(fno, O_BINARY);
	    gs_stdprn = fdopen(fno, "wb");
	}
	pfile = gs_stdprn;
#else	/* WATCOM doesn't know about stdprn device */
	pfile = fopen("PRN", (binary_mode ? "wb" : "w"));
	if (pfile == NULL)
	    return NULL;
#endif	/* defined(stdprn) */
    } else {
	pfile = fopen(fname, (binary_mode ? "wb" : "w"));
	if (pfile == NULL)
	    return NULL;
    }
    gp_set_file_binary(fileno(pfile), binary_mode);
    return pfile;
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE * pfile, const char *fname)
{
#ifdef stdprn
    if (pfile != stdprn)
#endif	/* defined(stdprn) */
	fclose(pfile);
    if (pfile == gs_stdprn)
	gs_stdprn = 0;
}

/* ------ File naming and accessing ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{	      /* The -7 is for XXXXXXX */
    int len = gp_file_name_sizeof - strlen(prefix) - 7;

    if (gp_getenv("TEMP", fname, &len) != 0)
	*fname = 0;
    else {
	char *temp;

	/* Prevent X's in path from being converted by mktemp. */
	for (temp = fname; *temp; temp++)
	    *temp = tolower(*temp);
	if (strlen(fname) && (fname[strlen(fname) - 1] != '\\'))
	    strcat(fname, "\\");
    }
    strcat(fname, prefix);
    strcat(fname, "XXXXXX");
    mktemp(fname);
    return fopen(fname, mode);
}


/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
    return fopen(fname, mode);
}
