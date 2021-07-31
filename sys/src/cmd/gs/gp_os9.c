/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_os9.c */
/* OSK-specific routines for Ghostscript */
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "time_.h"
#include <signal.h>
#include <sys/param.h>	/* for MAXPATHLEN */

/* popen isn't POSIX-standard, so we declare it here. */
extern FILE *popen();
extern int pclose();

int interrupted;

/* Forward declarations */
private void signalhandler(P1(int));
private FILE *rbfopen(P2(char*, char*));

/* Do platform-dependent initialization */
void
gp_init(void)
{	intercept(signalhandler);
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

private void
signalhandler(int sig)
{	clearerr(stdin);
	switch(sig) {
		case SIGINT:
		case SIGQUIT:
			interrupted = 1;
			break;
		case SIGFPE:
			interrupted = 2;
			break;
		default:
			break;
	}
}

/* ------ Date and time ------ */

/* Read the current date (in days since Jan. 1, 1980) */
/* and time (in milliseconds since midnight). */
#define PS_YEAR_0 80
#define PS_MONTH_0 1
#define PS_DAY_0 1
void
gp_get_clock(long *pdt)
{
	long date, time, pstime, psdate, tick;
	short day;

	_sysdate(0, &time, &date, &day, &tick);
	_julian(&time, &date);
        
	pstime = 0;
	psdate = (PS_YEAR_0 << 16) + (PS_MONTH_0 << 8) + PS_DAY_0;
	_julian(&pstime, &psdate);
        
	pdt[0] = date - psdate;
	pdt[1] = time * 1000;
	
#ifdef DEBUG_CLOCK
	printf("pdt[0] = %ld  pdt[1] = %ld\n", pdt[0], pdt[1]);
#endif
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* "|command" opens an output pipe. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char *fname, int binary_mode)
{	return
	  (strlen(fname) == 0 ?
	   gp_open_scratch_file(gp_scratch_file_name_prefix, fname, "w") :
	   fname[0] == '|' ?
	   popen(fname + 1, "w") :
	   rbfopen(fname, "w"));
}

FILE *
rbfopen(char *fname, char *perm)
{	FILE *file = fopen(fname, perm);
	file->_flag |= _RBF;
	return file;
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE *pfile, const char *fname)
{	if ( fname[0] == '|' )
		pclose(pfile);
	else
		fclose(pfile);
}
