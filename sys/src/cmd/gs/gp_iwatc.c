/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_iwatc.c */
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

/* Define a substitute for stdprn (see below). */
private FILE *gs_stdprn;

/* Forward declarations */
private void handle_FPE(P1(int));

/* Do platform-dependent initialization. */
void
gp_init(void)
{	gs_stdprn = 0;
	/* Set up the handler for numeric exceptions. */
	signal(SIGFPE, handle_FPE);
	gp_init_console();
}

/* Trap numeric exceptions.  Someday we will do something */
/* more appropriate with these. */
private void
handle_FPE(int sig)
{	eprintf("Numeric exception:\n");
	exit(1);
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
extern void gp_set_printer_binary(P2(int, int));
FILE *
gp_open_printer(char *fname, int binary_mode)
{	FILE *pfile;
	if ( strlen(fname) == 0 || !strcmp(fname, "PRN") )
	{	if ( !binary_mode )
			return stdprn;
		if ( gs_stdprn == 0 )
		{	/* We have to effectively reopen the printer, */
			/* because the Watcom library does \n -> \r\n */
			/* substitution on the stdprn stream. */
			int fno = dup(fileno(stdprn));
			setmode(fno, O_BINARY);
			gs_stdprn = fdopen(fno, "wb");
		}
		pfile = gs_stdprn;
	}
	else
	{	pfile = fopen(fname, (binary_mode ? "wb" : "w"));
		if ( pfile == NULL )
			return NULL;
	}
	gp_set_printer_binary(fileno(pfile), binary_mode);
	return pfile;
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE *pfile, const char *fname)
{	if ( pfile != stdprn )
		fclose(pfile);
	if ( pfile == gs_stdprn )
		gs_stdprn = 0;
}

/* ------ File names ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{	/* Unfortunately, Watcom C doesn't provide mktemp, */
	/* so we have to simulate it ourselves. */
	char *temp;
	struct stat fst;
	char *end;
	if ( (temp = getenv("TEMP")) == NULL )
		*fname = 0;
	else
	{	char last = '\\';
		strcpy(fname, temp);
		/* Prevent X's in path from being converted by mktemp. */
		for ( temp = fname; *temp; temp++ )
			*temp = last = tolower(*temp);
		switch ( last )
		{
		default:
			strcat(fname, "\\");
		case ':': case '\\':
			;
		}
	}
	strcat(fname, prefix);
	strcat(fname, "AA.AAA");
	end = fname + strlen(fname) - 1;
	while ( stat(fname, &fst) == 0 )
	   {	char *inc = end;
		while ( *inc == 'Z' || *inc == '.' )
		   {	if ( *inc == 'Z' ) *inc = 'A';
			inc--;
			if ( end - inc == 6 ) return 0;
		   }
		++*inc;
	   }
	return fopen(fname, mode);
}
