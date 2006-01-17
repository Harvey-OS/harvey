/* Copyright (C) 1991, 1995, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_iwatc.c,v 1.17 2004/01/15 09:27:10 giles Exp $ */
/* Intel processor, Watcom C-specific routines for Ghostscript */
#include "dos_.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "stat_.h"
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gpmisc.h"

/* Library routines not declared in a standard header */
extern char *mktemp(char *);	/* in gp_mktmp.c */

/* Define a substitute for stdprn (see below). */
private FILE *gs_stdprn;

/* Forward declarations */
private void handle_FPE(int);

/* Do platform-dependent initialization. */
void
gp_init(void)
{
    gs_stdprn = 0;
    /* Set up the handler for numeric exceptions. */
    signal(SIGFPE, handle_FPE);
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

/* ------ Persistent data cache ------*/
  
/* insert a buffer under a (type, key) pair */
int gp_cache_insert(int type, byte *key, int keylen, void *buffer, int buflen)
{ 
    /* not yet implemented */
    return 0;
} 
 
/* look up a (type, key) in the cache */
int gp_cache_query(int type, byte* key, int keylen, void **buffer,
    gp_cache_alloc alloc, void *userdata)
{
    /* not yet implemented */
    return -1;
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
extern void gp_set_file_binary(int, int);
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
    int prefix_length = strlen(prefix);
    int len = gp_file_name_sizeof - prefix_length - 7;
    FILE *f;

    if (gp_file_name_is_absolute(prefix, prefix_length) ||
	gp_gettmpdir(fname, &len) != 0
	)
	*fname = 0;
    else {
	char *temp;

	/* Prevent X's in path from being converted by mktemp. */
	for (temp = fname; *temp; temp++)
	    *temp = tolower(*temp);
	if (strlen(fname) && (fname[strlen(fname) - 1] != '\\'))
	    strcat(fname, "\\");
    }
    if (strlen(fname) + prefix_length + 7 >= gp_file_name_sizeof)
	return 0;		/* file name too long */
    strcat(fname, prefix);
    strcat(fname, "XXXXXX");
    mktemp(fname);
    f = gp_fopentemp(fname, mode);
    if (f == NULL)
	eprintf1("**** Could not open temporary file %s\n", fname);
    return f;
}


/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
    return fopen(fname, mode);
}

/* ------ Font enumeration ------ */
 
 /* This is used to query the native os for a list of font names and
  * corresponding paths. The general idea is to save the hassle of
  * building a custom fontmap file.
  */
 
void *gp_enumerate_fonts_init(gs_memory_t *mem)
{
    return NULL;
}
         
int gp_enumerate_fonts_next(void *enum_state, char **fontname, char **path)
{
    return 0;
}
                         
void gp_enumerate_fonts_free(void *enum_state)
{
}           
