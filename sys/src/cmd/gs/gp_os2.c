/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_os2.c */
/* Common platform-specific routines for OS/2 and MS-DOS */
/* compiled with GCC/EMX */

#define INCL_DOS
#include <os2.h>

#include "stdio_.h"
#include "string_.h"
#include <fcntl.h>

#ifdef __IBMC__
#define popen fopen	/* doesn't support popen */
#else
#include <dos.h>
#endif
/* Define the regs union tag for short registers. */
#  define rshort x
#define intdos(a,b) _int86(0x21, a, b)

#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gsexit.h"
#include "gsmemory.h"
#include "gsstruct.h"
#include "gp.h"
#include "gsutil.h"
#include "stdlib.h"	/* need _osmode */
#include "time_.h"
#include <time.h>		/* should this be in time_.h? */
#include "gdevpm.h"
#ifdef __EMX__
#include <sys/emxload.h>
#endif

/* ------ Miscellaneous ------ */

/* Get the string corresponding to an OS error number. */
/* All reasonable compilers support it. */
const char *
gp_strerror(int errnum)
{	return strerror(errnum);
}

/* use Unix version for date and time */
/* ------ Date and time ------ */

/* Read the current date (in days since Jan. 1, 1980) */
/* and time (in milliseconds since midnight). */
void
gp_get_clock(long *pdt)
{	long secs_since_1980;
	struct timeval tp;
	struct timezone tzp;
	time_t tsec;
	struct tm *tm, *localtime();

	if ( gettimeofday(&tp, &tzp) == -1 )
	   {	lprintf("Ghostscript: gettimeofday failed!\n");
		gs_exit(1);
	   }

	/* tp.tv_sec is #secs since Jan 1, 1970 */

	/* subtract off number of seconds in 10 years */
	/* leap seconds are not accounted for */
	secs_since_1980 = tp.tv_sec - (long)(60 * 60 * 24 * 365.25 * 10);
 
	/* adjust for timezone */
	secs_since_1980 -= (tzp.tz_minuteswest * 60);

	/* adjust for daylight savings time - assume dst offset is 1 hour */
	tsec = tp.tv_sec;
	tm = localtime(&tsec);
	if ( tm->tm_isdst )
		secs_since_1980 += (60 * 60);

	/* divide secs by #secs/day to get #days (integer division truncates) */
	pdt[0] = secs_since_1980 / (60 * 60 * 24);
	/* modulo + microsecs/1000 gives number of millisecs since midnight */
	pdt[1] = (secs_since_1980 % (60 * 60 * 24)) * 1000 + tp.tv_usec / 1000;
#ifdef DEBUG_CLOCK
	printf("tp.tv_sec = %d  tp.tv_usec = %d  pdt[0] = %ld  pdt[1] = %ld\n",
		tp.tv_sec, tp.tv_usec, pdt[0], pdt[1]);
#endif
}


/* ------ Console management ------ */

/* Answer whether a given file is the console (input or output). */
/* This is not a standard gp procedure, */
/* but the MS Windows configuration needs it, */
/* and other MS-DOS configurations might need it someday. */
/* Don't know if it is needed for OS/2. */
bool
gp_file_is_console(FILE *f)
{
#ifndef __DLL__
    if (_osmode == DOS_MODE) {
	union REGS regs;
	if ( f == NULL )
		return false;
	regs.h.ah = 0x44;	/* ioctl */
	regs.h.al = 0;		/* get device info */
	regs.rshort.bx = fileno(f);
	intdos(&regs, &regs);
	return ((regs.h.dl & 0x80) != 0 && (regs.h.dl & 3) != 0);
    }
#endif
    if ( (f==stdin) || (f==stdout) || (f==stderr) )
	return true;
    return false;
}

/* ------ File names ------ */

/* Define the character used for separating file names in a list. */
const char gp_file_name_list_separator = ';';

/* Define the default scratch file name prefix. */
const char gp_scratch_file_name_prefix[] = "gs";

/* Define the name of the null output file. */
const char gp_null_file_name[] = "nul";

/* Define the name that designates the current directory. */
const char gp_current_directory_name[] = ".";

/* Define the string to be concatenated with the file mode */
/* for opening files without end-of-line conversion. */
const char gp_fmode_binary_suffix[] = "b";
/* Define the file modes for binary reading or writing. */
const char gp_fmode_rb[] = "rb";
const char gp_fmode_wb[] = "wb";

/* Answer whether a file name contains a directory/device specification, */
/* i.e. is absolute (not directory- or device-relative). */
bool
gp_file_name_is_absolute(const char *fname, uint len)
{	/* A file name is absolute if it contains a drive specification */
	/* (second character is a :) or if it start with / or \. */
	return ( len >= 1 && (*fname == '/' || *fname == '\\' ||
		(len >= 2 && fname[1] == ':')) );
}

/* Answer the string to be used for combining a directory/device prefix */
/* with a base file name.  The file name is known to not be absolute. */
const char *
gp_file_name_concat_string(const char *prefix, uint plen,
  const char *fname, uint len)
{	if ( plen > 0 )
	  switch ( prefix[plen - 1] )
	   {	case ':': case '/': case '\\': return "";
	   };
	return "\\";
}


/* ------ File enumeration ------ */


struct file_enum_s {
	FILEFINDBUF3 findbuf;
	HDIR hdir;
	char *pattern;
	int patlen;			/* orig pattern length */
	int pat_size;			/* allocate space for pattern */
	int head_size;			/* pattern length through last */
					/* :, / or \ */
	int first_time;
	gs_memory_t *memory;
};
gs_private_st_ptrs1(st_file_enum, struct file_enum_s, "file_enum",
  file_enum_enum_ptrs, file_enum_reloc_ptrs, pattern);

/* Initialize an enumeration.  may NEED WORK ON HANDLING * ? \. */
file_enum *
gp_enumerate_files_init(const char *pat, uint patlen, gs_memory_t *mem)
{	file_enum *pfen = gs_alloc_struct(mem, file_enum, &st_file_enum, "gp_enumerate_files");
	int pat_size = 2 * patlen + 1;
	char *pattern;
	int hsize = 0;
	int i;
	if ( pfen == 0 ) return 0;

	/* pattern could be allocated as a string, */
	/* but it's simpler for GC and freeing to allocate it as bytes. */

	pattern = (char *)gs_alloc_bytes(mem, pat_size,
					 "gp_enumerate_files(pattern)");
	if ( pattern == 0 ) return 0;
	memcpy(pattern, pat, patlen);
	/* find directory name = header */
	for ( i = 0; i < patlen; i++ )
	{	switch ( pat[i] )
		{
		case '\\':
			if ( i + 1 < patlen && pat[i + 1] == '\\' )
				i++;
			/* falls through */
		case ':':
		case '/':
			hsize = i + 1;
		}
	}
	pattern[patlen] = 0;
	pfen->pattern = pattern;
	pfen->patlen = patlen;
	pfen->pat_size = pat_size;
	pfen->head_size = hsize;
	pfen->memory = mem;
	pfen->first_time = 1;
	pfen->hdir = HDIR_CREATE;
	return pfen;
}

/* Enumerate the next file. */
uint
gp_enumerate_files_next(file_enum *pfen, char *ptr, uint maxlen)
{
    APIRET rc;
    ULONG cFilenames = 1;
#ifndef __DLL__
    if (_osmode == DOS_MODE) {
	/* CAN'T DO IT SO JUST RETURN THE PATTERN. */
	if ( pfen->first_time )
	{	char *pattern = pfen->pattern;
		uint len = strlen(pattern);
		pfen->first_time = 0;
		if ( len > maxlen )
			return maxlen + 1;
		strcpy(ptr, pattern);
		return len;
	}
	return -1;
    }
#endif

    /* else OS/2 */
    if ( pfen->first_time ) {
	rc = DosFindFirst(pfen->pattern, &pfen->hdir, FILE_NORMAL,
		&pfen->findbuf, sizeof(pfen->findbuf), 
		&cFilenames, FIL_STANDARD);
	pfen->first_time = 0;
    }
    else {
	rc = DosFindNext(pfen->hdir, &pfen->findbuf, sizeof(pfen->findbuf),
		&cFilenames);
    }
    if (rc)
	return -1;
    
    if (pfen->head_size + pfen->findbuf.cchName < maxlen) {
        memcpy(ptr, pfen->pattern, pfen->head_size);
	strcpy(ptr + pfen->head_size, pfen->findbuf.achName);
	return pfen->head_size + pfen->findbuf.cchName;
    }

    if (pfen->head_size >= maxlen)
	return 0;	/* no hope at all */

    memcpy(ptr, pfen->pattern, pfen->head_size);
    strncpy(ptr + pfen->head_size, pfen->findbuf.achName, 
	    maxlen - pfen->head_size - 1);
    return maxlen;
}

/* Clean up the file enumeration. */
void
gp_enumerate_files_close(file_enum *pfen)
{
	gs_memory_t *mem = pfen->memory;
#ifndef __DLL__
	if (_osmode == OS2_MODE)
		DosFindClose(pfen->hdir);
#endif
	gs_free_object(mem, pfen->pattern,
		       "gp_enumerate_files_close(pattern)");
	gs_free_object(mem, pfen, "gp_enumerate_files_close");
}

/*************************************************************/
/* from gp_iwatc.c and gp_itbc.c */

/* Intel processor, EMX/GCC specific routines for Ghostscript */
#include <signal.h>
#include "stat_.h"
#include "string_.h"

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));

/* Forward declarations */
private void handle_FPE(P1(int));

/* Do platform-dependent initialization. */
void
gp_init(void)
{
	/* keep gsos2.exe in memory for number of minutes specified in */
	/* environment variable GS_LOAD */
#ifdef __EMX__
	_emxload_env("GS_LOAD");
#endif
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

extern int gs_exit_status;
/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
	if (exit_status && (_osmode == OS2_MODE))
		DosSleep(2000);
}

/* ------ Printer accessing ------ */

/* Put a printer file (which might be stdout) into binary or text mode. */
/* This is not a standard gp procedure, */
/* but all MS-DOS configurations need it. */
void
gp_set_printer_binary(int prnfno, int binary)
{
#ifndef __IBMC__
	union REGS regs;
	regs.h.ah = 0x44;	/* ioctl */
	regs.h.al = 0;		/* get device info */
	regs.rshort.bx = prnfno;
	intdos(&regs, &regs);
	if ( ((regs.rshort.flags)&1) != 0 || !(regs.h.dl & 0x80) )
		return;		/* error, or not a device */
	if ( binary )
		regs.h.dl |= 0x20;	/* binary (no ^Z intervention) */
	else
		regs.h.dl &= ~0x20;	/* text */
	regs.h.dh = 0;
	regs.h.ah = 0x44;	/* ioctl */
	regs.h.al = 1;		/* set device info */
	intdos(&regs, &regs);
#endif
}

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* "|command" opens an output pipe. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char *fname, int binary_mode)
{	FILE *pfile;
	if ( strlen(fname) == 0 ) 
		pfile = fopen("PRN", (binary_mode ? "wb" : "w"));
#ifdef __DLL__
	else if (fname[0] == '|')
#else
	else if ( (_osmode == OS2_MODE) && (fname[0] == '|') )
#endif
		pfile = popen(fname+1, (binary_mode ? "wb" : "w"));
	else
		pfile = fopen(fname, (binary_mode ? "wb" : "w"));
	if ( pfile == (FILE *)NULL )
		return (FILE *)NULL;
#ifndef __DLL__
	if (_osmode == DOS_MODE)
	    gp_set_printer_binary(fileno(pfile), binary_mode);
#endif
	return pfile;
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE *pfile, const char *fname)
{	
#ifdef __DLL__
	if (fname[0] == '|')
#else
	if ( (_osmode == OS2_MODE) && (fname[0] == '|') )
#endif
		pclose(pfile);
	else
		fclose(pfile);
}

/* ------ File names ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{	char *temp;
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
	strcat(fname, "XXXXXX");
	mktemp(fname);
	return fopen(fname, mode);
}

#ifdef __DLL__
/* The DLL version must not be allowed direct access to stdin and stdout */
/* Instead these are redirected to the gsdll_callback */
#include "gsdll.h"
#include <stdarg.h>

/* for redirecting stdin/out/err */
#include "stream.h"
#include "gxiodev.h"			/* must come after stream.h */
extern stream *gs_stream_stdin;		/* from ziodev.c */
extern stream *gs_stream_stdout;	/* from ziodev.c */
extern stream *gs_stream_stderr;	/* from ziodev.c */


/* ====== Substitute for stdio ====== */

/* this code has been derived from gp_mswin.c */

/* Forward references */
private void pm_std_init(void);
private void pm_pipe_init(void);
private stream_proc_process(pm_std_read_process);
private stream_proc_process(pm_std_write_process);

/* Use a pseudo IODevice to get pm_stdio_init called at the right time. */
/* This is bad architecture; we'll fix it later. */
private iodev_proc_init(pm_stdio_init);
gx_io_device gs_iodev_wstdio = {
	"wstdio", "Special",
	{ pm_stdio_init, iodev_no_open_device,
	  iodev_no_open_file, iodev_no_fopen, iodev_no_fclose,
	  iodev_no_delete_file, iodev_no_rename_file,
	  iodev_no_file_status, iodev_no_enumerate_files
	}
};

/* Discard the contents of the buffer when reading. */
void
pm_std_read_reset(stream *s)
{	s_std_read_reset(s);
	s->end_status = 0;
}

/* Do one-time initialization */
private int
pm_stdio_init(gx_io_device *iodev, gs_memory_t *mem)
{
/* reinitialize stdin/out/err to use callback */
/* assume stream has already been initialized for the real stdin */
	if ( gp_file_is_console(gs_stream_stdin->file) )
	{	/* Allocate a real buffer for stdin. */
		/* The size must not exceed the size of the */
		/* lineedit buffer.  (This is a hack.) */
#define pm_stdin_buf_size 160
		static const stream_procs pin =
		{	s_std_noavailable, s_std_noseek, pm_std_read_reset,
			s_std_read_flush, s_std_close,
			pm_std_read_process
		};
		byte *buf = gs_alloc_bytes(gs_stream_stdin->memory,
					   pm_stdin_buf_size,
					   "pm_stdin_init");
		s_std_init(gs_stream_stdin, buf, pm_stdin_buf_size,
			   &pin, s_mode_read);
		gs_stream_stdin->file = NULL;
	}

	{	static const stream_procs pout =
		{	s_std_noavailable, s_std_noseek, s_std_write_reset,
			s_std_write_flush, s_std_close,
			pm_std_write_process
		};
		if ( gp_file_is_console(gs_stream_stdout->file) )
		{	gs_stream_stdout->procs = pout;
			gs_stream_stdout->file = NULL;
		}
		if ( gp_file_is_console(gs_stream_stderr->file) )
		{	gs_stream_stderr->procs = pout;
			gs_stream_stderr->file = NULL;
		}
	}
	return 0;
}


/* We should really use a private buffer for line reading, */
/* because we can't predict the size of the supplied input area.... */
private int
pm_std_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{
	int count = pw->limit - pw->ptr;

	if ( count == 0 ) 		/* empty buffer */
		return 1;

/* callback to get more input */
	count = (*pgsdll_callback)(GSDLL_STDIN, pw->ptr+1, count);
	if (count == 0) {
	    /* EOF */
	    /* what should we do? */
	    return EOFC;
	}

	pw->ptr += count;
	return 1;
}

private int
pm_std_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;
	(*pgsdll_callback)(GSDLL_STDOUT, (char *)(pr->ptr + 1), count);
	pr->ptr = pr->limit;
	return 0;
}

/* This is used instead of the stdio version. */
/* The declaration must be identical to that in <stdio.h>. */
int 
fprintf(FILE *file, const char *fmt, ...)
{
int count;
va_list args;
	va_start(args,fmt);
	if ( gp_file_is_console(file) ) {
		char buf[1024];
		count = vsprintf(buf,fmt,args);
		(*pgsdll_callback)(GSDLL_STDOUT, buf, count);
	}
	else {
		count = vfprintf(file, fmt, args);
	}
	va_end(args);
	return count;
}
#endif /* __DLL__ */

