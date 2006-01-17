/* Copyright (C) 1989, 1995, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_dvx.c,v 1.11 2004/01/15 09:27:10 giles Exp $ */
/* Desqview/X-specific routines for Ghostscript */
#include "string_.h"
#include "gx.h"
#include "gsexit.h"
#include "gp.h"
#include "time_.h"

/* Do platform-dependent initialization. */
void
gp_init(void)
{
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

/* ------ Miscellaneous ------ */

/* Get the string corresponding to an OS error number. */
/* All reasonable compilers support it. */
const char *
gp_strerror(int errnum)
{
    return strerror(errnum);
}

/* ------ Date and time ------ */

/* Read the current time (in seconds since Jan. 1, 1970) */
/* and fraction (in nanoseconds). */
void
gp_get_realtime(long *pdt)
{
    struct timeval tp;
    struct timezone tzp;

    if (gettimeofday(&tp, &tzp) == -1) {
	lprintf("Ghostscript: gettimeofday failed!\n");
	tp.tv_sec = tp.tv_usec = 0;
    }
    /* tp.tv_sec is #secs since Jan 1, 1970 */
    pdt[0] = tp.tv_sec;
    pdt[1] = tp.tv_usec * 1000;

#ifdef DEBUG_CLOCK
    printf("tp.tv_sec = %d  tp.tv_usec = %d  pdt[0] = %ld  pdt[1] = %ld\n",
	   tp.tv_sec, tp.tv_usec, pdt[0], pdt[1]);
#endif
}

/* Read the current user CPU time (in seconds) */
/* and fraction (in nanoseconds).  */
void
gp_get_usertime(long *pdt)
{
    gp_get_realtime(pdt);	/* Use an approximation for now.  */
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
    if (strlen(fname) == 0 || !strcmp(fname, "PRN")) {
	if (binary_mode)
	    gp_set_file_binary(fileno(stdprn), 1);
	stdprn->_flag = _IOWRT;	/* Make stdprn buffered to improve performance */
	return stdprn;
    } else
	return fopen(fname, (binary_mode ? "wb" : "w"));
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE * pfile, const char *fname)
{
    if (pfile == stdprn)
	fflush(pfile);
    else
	fclose(pfile);
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
