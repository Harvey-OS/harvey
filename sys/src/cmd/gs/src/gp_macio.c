/* Copyright (C) 1989 - 1995, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_macio.c,v 1.2.4.1 2002/01/25 06:33:09 rayjj Exp $ */

//#include "MacHeaders"
#include <Palettes.h>
#include <Aliases.h>
#include <Quickdraw.h>
#include <QDOffscreen.h>
#include <AppleEvents.h>
#include <Fonts.h>
#include <Controls.h>
#include <Script.h>
#include <Timer.h>
#include <Folders.h>
#include <Resources.h>
#include <Sound.h>
#include <ToolUtils.h>
#include <Menus.h>
#include <LowMem.h>
#include <Devices.h>
#include <Scrap.h>
#include <StringCompare.h>
#include <Gestalt.h>
#include <Folders.h>
#include <Fonts.h>
#include <FixMath.h>
#include <Resources.h>
#include "math_.h"
#include <string.h>
#include <stdlib.h>

//#include <stdio.h>
//#include <cstdio.h>

#include "sys/stat.h"
#include "stdio_.h"
#include <stdlib.h>
#include "gx.h"
#include "gp.h"
#include "gxdevice.h"
#include <stdarg.h>
#include <console.h>
#include "gp_mac.h"

#include "stream.h"
#include "gxiodev.h"			/* must come after stream.h */
//#include "gp_macAE.h"
#include "gsdll.h"

//HWND hwndtext;


extern void
convertSpecToPath(FSSpec * s, char * p, int pLen)
{
	OSStatus	err = noErr;
	DirInfo		block;
	Str255		dirName;
	int			totLen = 0, dirLen = 0;

	memcpy(p, s->name + 1, s->name[0]);
	totLen += s->name[0];
	
	block.ioNamePtr = dirName;
	block.ioVRefNum = s->vRefNum;
	block.ioDrParID = s->parID;
	block.ioFDirIndex = -1;
	
	do {
		block.ioDrDirID = block.ioDrParID;
		err = PBGetCatInfoSync((CInfoPBPtr)&block);
		
		if ((err != noErr) || (totLen + dirName[0] + 2 > pLen)) {
			p[0] = 0;
			return;
		}
		
		dirName[++dirName[0]] = ':';
		memmove(p + dirName[0], p, totLen);
		memcpy(p, dirName + 1, dirName[0]);
		totLen += dirName[0];
	} while (block.ioDrParID != fsRtParID);
	
	p[totLen] = 0;
	
	return;
}



/* ------ File name syntax ------ */

/* Define the character used for separating file names in a list. */
const char gp_file_name_list_separator = ',';

/* Define the default scratch file name prefix. */
const char gp_scratch_file_name_prefix[] = "tempgs_";

/* Define the name of the null output file. */
const char gp_null_file_name[] = "????";

/* Define the name that designates the current directory. */
extern const char gp_current_directory_name[] = ":";

int fake_stdin = 0;


/* Do platform-dependent initialization */

void
setenv(const char * env, char *p) {
//	if ( strcmp(env,"outfile") == 0) {
//	   sprintf((char *)&g_fcout[0],"%s",p);
//	}
}



char *
getenv(const char * env) {

	char 			*p;
	FSSpec			pFile;
	OSErr			err = 0;
	char			fpath[256]="";
	
	if ( strcmp(env,"GS_LIB") == 0) {
	
	    	pFile.name[0] = 0;
	    	err = FindFolder(kOnSystemDisk, kApplicationSupportFolderType, kDontCreateFolder,
			 					&pFile.vRefNum, &pFile.parID);
			
			if (err != noErr) goto failed;

//		FSMakeFSSpec(pFile.vRefNum, pFile.parID,thepfname, &pfile);
		convertSpecToPath(&pFile, fpath, 256);
//		sprintf(fpath,"%s",fpath);
		p = (char*)gs_malloc(1, (size_t) ( 4*strlen(fpath) + 40), "getenv");
		sprintf(p,"%s,%sGhostscript:lib,%sGhostscript:fonts",
						(char *)&fpath[0],(char *)&fpath[0],
						(char *)&fpath[0] );

		return p;
failed:
		
		return NULL;
	} else
	    return NULL;

}


/* ====== Substitute for stdio ====== */

/* Forward references */
private void mac_std_init(void);
private stream_proc_process(mac_stdin_read_process);
private stream_proc_process(mac_stdout_write_process);
private stream_proc_process(mac_stderr_write_process);
private stream_proc_available(mac_std_available);

/* Use a pseudo IODevice to get mac_stdio_init called at the right time. */
/* This is bad architecture; we'll fix it later. */
private iodev_proc_init(mac_stdio_init);
const gx_io_device gs_iodev_macstdio =
{
    "macstdio", "Special",
    {mac_stdio_init, iodev_no_open_device,
     iodev_no_open_file, iodev_no_fopen, iodev_no_fclose,
     iodev_no_delete_file, iodev_no_rename_file,
     iodev_no_file_status, iodev_no_enumerate_files
    }
};

/* Do one-time initialization */
private int
mac_stdio_init(gx_io_device * iodev, gs_memory_t * mem)
{
    mac_std_init();		/* redefine stdin/out/err to our window routines */
    return 0;
}

/* Define alternate 'open' routines for our stdin/out/err streams. */

extern const gx_io_device gs_iodev_stdin;
private int
mac_stdin_open(gx_io_device * iodev, const char *access, stream ** ps,
	       gs_memory_t * mem)
{
    int code = gs_iodev_stdin.procs.open_device(iodev, access, ps, mem);
    stream *s = *ps;

    if (code != 1)
	return code;
    s->procs.process = mac_stdin_read_process;
    s->procs.available = mac_std_available;
    s->file = NULL;
    return 0;
}

extern const gx_io_device gs_iodev_stdout;
private int
mac_stdout_open(gx_io_device * iodev, const char *access, stream ** ps,
		gs_memory_t * mem)
{
    int code = gs_iodev_stdout.procs.open_device(iodev, access, ps, mem);
    stream *s = *ps;

    if (code != 1)
	return code;
    s->procs.process = mac_stdout_write_process;
    s->procs.available = mac_std_available;
    s->file = NULL;
    return 0;
}

extern const gx_io_device gs_iodev_stderr;
private int
mac_stderr_open(gx_io_device * iodev, const char *access, stream ** ps,
		gs_memory_t * mem)
{
    int code = gs_iodev_stderr.procs.open_device(iodev, access, ps, mem);
    stream *s = *ps;

    if (code != 1)
	return code;
    s->procs.process = mac_stderr_write_process;
    s->procs.available = mac_std_available;
    s->file = NULL;
    return 0;
}

/* Patch stdin/out/err to use our windows. */
private void
mac_std_init(void)
{
    /* If stdxxx is the console, replace the 'open' routines, */
    /* which haven't gotten called yet. */

//    if (gp_file_is_console(gs_stdin))
	gs_findiodevice((const byte *)"%stdin", 6)->procs.open_device =
	    mac_stdin_open;

//    if (gp_file_is_console(gs_stdout))
	gs_findiodevice((const byte *)"%stdout", 7)->procs.open_device =
	    mac_stdout_open;

//    if (gp_file_is_console(gs_stderr))
	gs_findiodevice((const byte *)"%stderr", 7)->procs.open_device =
	    mac_stderr_open;
}


private int
mac_stdin_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{
    uint count;
/* callback to get more input */
    count = (*pgsdll_callback) (GSDLL_STDIN, (char*)pw->ptr + 1, count);
	pw->ptr += count;	
	return 1;
}


private int
mac_stdout_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;
 
    (*pgsdll_callback) (GSDLL_STDOUT, (char *)(pr->ptr + 1), count);
	pr->ptr = pr->limit;
	return 0;
}

private int
mac_stderr_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;

    (*pgsdll_callback) (GSDLL_STDOUT, (char *)(pr->ptr + 1), count);
	pr->ptr = pr->limit;
	return 0;
}

private int
mac_std_available(register stream * s, long *pl)
{
    *pl = -1;		// EOF, since we can't do it
    return 0;		// OK
}

/* ====== Substitute for stdio ====== */

/* These are used instead of the stdio version. */
/* The declarations must be identical to that in <stdio.h>. */
int
fprintf(FILE *file, const char *fmt, ...)
{
	int		count;
	va_list	args;
	char	buf[1024];
    int i;
	
	va_start(args,fmt);
	
	if (file != stdout  &&  file != stderr) {
		count = vfprintf(file, fmt, args);
	}
	else {
		count = vsprintf(buf, fmt, args);
		return fwrite(buf, strlen(buf), 1, file);
	}
	
	va_end(args);
	return count;
}

int
fputs(const char *string, FILE *file)
{
	int i,count;
	char buf[1024];
	
	if (file != stdout  &&  file != stderr) {
		return fwrite(string, strlen(string), 1, file);
	}
	else {
		return fwrite(string, strlen(string), 1, file);
	}
}

/* ------ Printer accessing ------ */

/* These should NEVER be called. */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* "|command" opens an output pipe. */
/* Return NULL if the connection could not be opened. */

FILE *
gp_open_printer (char *fname, int binary_mode)
{
	if (strlen(fname) == 1  &&  fname[0] == '-')
		return stdout;
	else if (strlen(fname) == 0)
		return gp_open_scratch_file(gp_scratch_file_name_prefix, fname, binary_mode ? "wb" : "w");
	else
		return gp_fopen(fname, binary_mode ? "wb" : "b");
}

/* Close the connection to the printer. */

void
gp_close_printer (FILE *pfile, const char *fname)
{
	fclose(pfile);
}



/* Define whether case is insignificant in file names. */
/* OBSOLETE
const int gp_file_names_ignore_case = 1;
*/

/* Define the string to be concatenated with the file mode */
/* for opening files without end-of-line conversion. */
const char gp_fmode_binary_suffix[] = "b";

/* Define the file modes for binary reading or writing. */
const char gp_fmode_rb[] = "rb";
const char gp_fmode_wb[] = "wb";


/* Set a file into binary or text mode. */
int
gp_setmode_binary(FILE *pfile, bool binary)
{	return 0;	/* Noop under VMS */
}


/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */

FILE *
gp_open_scratch_file (const char *prefix, char *fname, const char *mode)
{
    char thefname[256];
    Str255 thepfname;
	OSErr myErr;
	short foundVRefNum;
	long foundDirID;
	FSSpec fSpec;
	strcpy (fname, (char *) prefix);
	{
		char newName[50];

		tmpnam (newName);
		strcat (fname, newName);
	}

   if ( strrchr(fname,':') == NULL ) {
       memcpy((char*)&thepfname[1],(char *)&fname[0],strlen(fname));
	   thepfname[0]=strlen(fname);
		myErr = FindFolder(kOnSystemDisk,kTemporaryFolderType,kCreateFolder,
			&foundVRefNum, &foundDirID);
		if ( myErr != noErr ) {
			fprintf(stderr,"Can't find temp folder.\n");
			return;
		}
		FSMakeFSSpec(foundVRefNum, foundDirID,thepfname, &fSpec);
		convertSpecToPath(&fSpec, thefname, sizeof(thefname) - 1);
		sprintf(fname,"%s",thefname);
   } else {
       sprintf((char*)&thefname[0],"%s\0",fname);
       memcpy((char*)&thepfname[1],(char *)&thefname[0],strlen(thefname));
	   thepfname[0]=strlen(thefname);
   }

	return gp_fopen (thefname, mode);
}

/*
{
    char thefname[256];
	strcpy (fname, (char *) prefix);
	{
		char newName[50];

		tmpnam (newName);
		strcat (fname, newName);
	}

   if ( strrchr(fname,':') == NULL ) 
//      sprintf((char *)&thefname[0],"%s%s\0",g_homeDir,fname);
      sprintf((char *)&thefname[0],"%s%s\0","",fname);
   else
       sprintf((char*)&thefname[0],"%s\0",fname);

	return gp_fopen (thefname, mode);
}
*/

/* Answer whether a file name contains a directory/device specification, */
/* i.e. is absolute (not directory- or device-relative). */

	int
gp_file_name_is_absolute (const char *fname, register uint len)

{
	if (len /* > 0 */)
	{
		if (*fname == ':')
		{
			return 0;
		}
		else
		{
			register char  *p;
			register char	lastWasColon;
			register char	sawColon;


			for (len, p = (char *) fname, lastWasColon = 0, sawColon = 0;
				 len /* > 0 */;
				 len--, p++)
			{
				if (*p == ':')
				{
					sawColon = 1;

					if (lastWasColon /* != 0 */)
					{
						return 0;
					}
					else
					{
						lastWasColon = 1;
					}
				}
				else
				{
					lastWasColon = 0;
				}
			}

			return sawColon;
		}
	}
	else
	{
		return 0;
	}
}

/* Answer whether the file_name references the directory	*/
/* containing the specified path (parent). 			*/
bool
gp_file_name_references_parent(const char *fname, unsigned len)
{
    int i = 0, last_sep_pos = -1;

    /* A file name references its parent directory if it starts */
    /* with ..: or ::  or if one of these strings follows : */
    while (i < len) {
	if (fname[i] == ':') {
	    if (last_sep_pos == i - 1)
	        return true;	/* also returns true is starts with ':' */
	    last_sep_pos = i++;
	    continue;
	}
	if (fname[i++] != '.')
	    continue;
        if (i > last_sep_pos + 2 || (i < len && fname[i] != '.'))
	    continue;
	i++;
	/* have separator followed by .. */
	if (i < len && (fname[i] == ':'))
	    return true;
    }
    return false;
}

/* Answer the string to be used for combining a directory/device prefix */
/* with a base file name. The prefix directory/device is examined to	*/
/* determine if a separator is needed and may return an empty string	*/
const char *
gp_file_name_concat_string (const char *prefix, uint plen)
{
	if ( plen > 0 && prefix[plen - 1] == ':' )
		return "";
	return ":";
}



/* ------ File enumeration ------ */

/****** THIS IS NOT SUPPORTED ON MACINTOSH SYSTEMS. ******/

struct file_enum_s {
	char *pattern;
	int first_time;
	gs_memory_t *memory;
};

/* Initialize an enumeration.  NEEDS WORK ON HANDLING * ? \. */

file_enum *
gp_enumerate_files_init (const char *pat, uint patlen, gs_memory_t *memory)

{	file_enum *pfen = 
		(file_enum *)gs_alloc_bytes(memory, sizeof(file_enum), "gp_enumerate_files");
	char *pattern;
	if ( pfen == 0 ) return 0;
	pattern = 
		(char *)gs_alloc_bytes(memory, patlen + 1, "gp_enumerate_files(pattern)");
	if ( pattern == 0 ) return 0;
	memcpy(pattern, pat, patlen);
	pattern[patlen] = 0;
	pfen->pattern = pattern;
	pfen->memory = memory;
	pfen->first_time = 1;
	return pfen;
}

/* Enumerate the next file. */

uint
gp_enumerate_files_next (file_enum *pfen, char *ptr, uint maxlen)

{	if ( pfen->first_time )
	   {	pfen->first_time = 0;
	   }
	return -1;
}

/* Clean up the file enumeration. */

void
gp_enumerate_files_close (file_enum *pfen)

{	
	gs_free_object(pfen->memory, pfen->pattern, "gp_enumerate_files_close(pattern)");
	gs_free_object(pfen->memory, (char *)pfen, "gp_enumerate_files_close");
}

FILE * 
gp_fopen (const char * fname, const char * mode ) {

   char thefname[256];
   FILE *fid;
   int ans;

//sprintf((char*)&thefname[0],"\n%s\n",fname);
//(*pgsdll_callback) (GSDLL_STDOUT, thefname, strlen(fname));
   if ( strrchr(fname,':') == NULL ) 
//      sprintf((char *)&thefname[0],"%s%s\0",g_homeDir,fname);
      sprintf((char *)&thefname[0],"%s%s\0","",fname);
   else
       sprintf((char*)&thefname[0],"%s\0",fname);
       
   fid = fopen(thefname,mode);
   
   return fid;
   
}

FILE * 
popen (const char * fname, const char * mode ) {
return gp_fopen (fname,  mode );
}

int
pclose (FILE * pipe ) {
return fclose (pipe );
}
