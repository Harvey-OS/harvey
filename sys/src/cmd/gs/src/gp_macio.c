/* Copyright (C) 1989-2003 artofcode, LLC.  All rights reserved.
  
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

/* $Id: gp_macio.c,v 1.37 2004/12/09 03:47:52 giles Exp $ */

#ifndef __CARBON__
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
#include <Files.h>
#include <Fonts.h>
#include <FixMath.h>
#include <Resources.h>
#else
#include <Carbon.h>
#include <CoreServices.h>
#endif /* __CARBON__ */

#include "stdio_.h"
#include "math_.h"
#include "string_.h"
#include <stdlib.h>
#include <stdarg.h>
#include <console.h>

#include "gx.h"
#include "gp.h"
#include "gpmisc.h"
#include "gxdevice.h"

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
	CInfoPBRec	params;
	Str255		dirName;
	int		totLen = 0, dirLen = 0;

	memcpy(p, s->name + 1, s->name[0]);
	totLen += s->name[0];
	
	params.dirInfo.ioNamePtr = dirName;
	params.dirInfo.ioVRefNum = s->vRefNum;
	params.dirInfo.ioDrParID = s->parID;
	params.dirInfo.ioFDirIndex = -1;
	
	do {
		params.dirInfo.ioDrDirID = params.dirInfo.ioDrParID;
		err = PBGetCatInfoSync(&params);
		
		if ((err != noErr) || (totLen + dirName[0] + 2 > pLen)) {
			p[0] = 0;
			return;
		}
		
		dirName[++dirName[0]] = ':';
		memmove(p + dirName[0], p, totLen);
		memcpy(p, dirName + 1, dirName[0]);
		totLen += dirName[0];
	} while (params.dirInfo.ioDrParID != fsRtParID);
	
	p[totLen] = 0;
	
	return;
}

OSErr
convertPathToSpec(const char *path, const int pathlength, FSSpec * spec)
{
	Str255 filename;
	
	/* path must be shorter than 255 bytes */
	if (pathlength > 254) return bdNamErr;
	
	*filename = pathlength;
	memcpy(filename + 1, path, pathlength);
	
	return FSMakeFSSpec(0, 0, filename, spec);
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
		p = (char*)malloc((size_t) ( 4*strlen(fpath) + 40));
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
    uint count = pw->limit - pw->ptr;
    /* callback to get more input */
    if (pgsdll_callback == NULL) return EOFC;
    count = (*pgsdll_callback) (GSDLL_STDIN, (char*)pw->ptr + 1, count);
	pw->ptr += count;	
	return 1;
}


private int
mac_stdout_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;
 
    if (pgsdll_callback == NULL) return EOFC;
    (*pgsdll_callback) (GSDLL_STDOUT, (char *)(pr->ptr + 1), count);
	pr->ptr = pr->limit;
	return 0;
}

private int
mac_stderr_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	uint count = pr->limit - pr->ptr;

    if (pgsdll_callback == NULL) return EOFC;
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

/* ------ Printer accessing ------ */

/* These should NEVER be called. */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* "|command" opens an output pipe. */
/* Return NULL if the connection could not be opened. */

FILE *
gp_open_printer (char *fname, int binary_mode)
{
    if (strlen(fname) == 0)
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
gp_open_scratch_file (const char *prefix, char fname[gp_file_name_sizeof], const char *mode)
{
    char thefname[256];
    Str255 thepfname;
    OSErr myErr;
    short foundVRefNum;
    long foundDirID;
    FSSpec fSpec;
    FILE *f;
    int prefix_length = strlen(prefix);

    if (prefix_length > gp_file_name_sizeof) return NULL;
    strcpy (fname, (char *) prefix);
      {
	char newName[50];

	tmpnam (newName);
	if ( prefix_length + strlen(newName) > gp_file_name_sizeof ) return NULL;
	strcat (fname, newName);
      }

   if ( strlen(fname) > 255 ) return NULL;
   if ( strrchr(fname,':') == NULL ) {
       memmove((char*)&thepfname[1],(char *)&fname[0],strlen(fname));
	   thepfname[0]=strlen(fname);
		myErr = FindFolder(kOnSystemDisk,kTemporaryFolderType,kCreateFolder,
			&foundVRefNum, &foundDirID);
		if ( myErr != noErr ) {
			eprintf("Can't find temp folder.\n");
			return (NULL);
		}
		FSMakeFSSpec(foundVRefNum, foundDirID,thepfname, &fSpec);
		convertSpecToPath(&fSpec, thefname, sizeof(thefname) - 1);
		sprintf(fname,"%s",thefname);
   } else {
       sprintf((char*)&thefname[0],"%s\0",fname);
       memmove((char*)&thepfname[1],(char *)&thefname[0],strlen(thefname));
	   thepfname[0]=strlen(thefname);
   }

    f = gp_fopen (thefname, mode);
    if (f == NULL)
	eprintf1("**** Could not open temporary file %s\n", fname);
    return f;
}

/* read a resource and copy the data into a buffer */
/* we don't have access to an allocator, nor any context for local  */
/* storage, so we implement the following idiom: we return the size */
/* of the requested resource and copy the data into buf iff it's    */
/* non-NULL. Thus, the caller can pass NULL for buf the first time, */
/* allocate the appropriate sized buffer, and then call us a second */
/* time to actually transfer the data.                              */
int
gp_read_macresource(byte *buf, const char *fname, const uint type, const ushort id)
{
    Handle resource = NULL;
    SInt32 size = 0;
    FSSpec spec;
    SInt16 fileref;
    OSErr result;
    
    /* open file */
    result = convertPathToSpec(fname, strlen(fname), &spec);
    if (result != noErr) goto fin;
    fileref = FSpOpenResFile(&spec, fsRdPerm);
    if (fileref == -1) goto fin;
    
    if_debug1('s', "[s] loading resource from fileref %d\n", fileref);

    /* load resource */
    resource = Get1Resource((ResType)type, (SInt16)id);
    if (resource == NULL) goto fin;
          
    /* allocate res */
    /* GetResourceSize() is probably good enough */
    //size = GetResourceSizeOnDisk(resource);
    size = GetMaxResourceSize(resource);
    
    if_debug1('s', "[s] resource size on disk is %d bytes\n", size);
    
    /* if we don't have a buffer to fill, just return */
    if (buf == NULL) goto fin;

    /* otherwise, copy resource into res from handle */
    HLock(resource);
    memcpy(buf, *resource, size);
    HUnlock(resource);
    
fin:
    /* free resource, if necessary */
    ReleaseResource(resource);
    CloseResFile(fileref);
    
    return (size);
}

/* return a list of font names and corresponding paths from 
 * the native system locations
 */
int gp_native_fontmap(char *names[], char *paths[], int *count)
{
    return 0;
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
gp_fopen (const char * fname, const char * mode) {

   char thefname[256];
   FILE *fid;

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
	return gp_fopen (fname,  mode);
}

int
pclose (FILE * pipe ) {
	return fclose (pipe);
}

/* -------------- Helpers for gp_file_name_combine_generic ------------- */

#ifdef __CARBON__

/* compare an HFSUnitStr255 with a C string */
static int compare_UniStr(HFSUniStr255 u, const char *c, uint len)
{
	int i,searchlen,unichar;
	searchlen = min(len,u.length);
	for (i = 0; i < searchlen; i++) {
	  unichar = u.unicode[i];
	  /* punt on wide characters. we should really convert */
	  if (unichar & !0xFF) return -1;
	  /* otherwise return the the index of the first non-matching character */
	  if (unichar != c[i]) break;
	}
	/* return the offset iff we matched the whole volume name */
	return (i == u.length) ? i : 0;
}

uint gp_file_name_root(const char *fname, uint len)
{
	OSErr err = noErr;
   	HFSUniStr255 volumeName;
   	FSRef rootDirectory;
   	int index, match;
   	
    if (len > 0 && fname[0] == ':')
		return 0; /* A relative path, no root. */

	/* iterate over mounted volumes and compare our path */
	index = 1;
	while (err == noErr) {
		err = FSGetVolumeInfo (kFSInvalidVolumeRefNum, index,
			NULL, kFSVolInfoNone, NULL, /* not interested in these fields */
			&volumeName, &rootDirectory);
		if (err == nsvErr) return 0; /* no more volumes */
		if (err == noErr) {
			match = compare_UniStr(volumeName, fname, len);
			if (match > 0) {
    			/* include the separator if it's present  */
				if (fname[match] == ':') return match + 1;
				return match;
			}
		}
		index++;
	}
	
	/* nothing matched */
    return 0;
}

#else /* Classic MacOS */

/* FSGetVolumeInfo requires carbonlib or macos >= 9
   we essentially leave this unimplemented on Classic */
uint gp_file_name_root(const char *fname, uint len)
{
	return 0;
}
   
#endif /* __CARBON__ */


uint gs_file_name_check_separator(const char *fname, int len, const char *item)
{   if (len > 0) {
	if (fname[0] == ':') {
	    if (fname == item + 1 && item[0] == ':')
		return 1; /* It is a separator after parent. */
	    if (len > 1 && fname[1] == ':')
		return 0; /* It is parent, not a separator. */
	    return 1;
	}
    } else if (len < 0) {
	if (fname[-1] == ':')
	    return 1;
    }
    return 0;
}

bool gp_file_name_is_parent(const char *fname, uint len)
{   return len == 1 && fname[0] == ':';
}

bool gp_file_name_is_current(const char *fname, uint len)
{   return (len == 0) || (len == 1 && fname[0] == ':');
}

const char *gp_file_name_separator(void)
{   return ":";
}

const char *gp_file_name_directory_separator(void)
{   return ":";
}

const char *gp_file_name_parent(void)
{   return "::";
}

const char *gp_file_name_current(void)
{   return ":";
}

bool gp_file_name_is_partent_allowed(void)
{   return true;
}

bool gp_file_name_is_empty_item_meanful(void)
{   return true;
}

gp_file_name_combine_result
gp_file_name_combine(const char *prefix, uint plen, const char *fname, uint flen, 
		    bool no_sibling, char *buffer, uint *blen)
{
    return gp_file_name_combine_generic(prefix, plen, 
	    fname, flen, no_sibling, buffer, blen);
}

// FIXME: there must be a system util for this!
static char *MacStr2c(char *pstring)
{
	char *cstring;
	int len = (pstring[0] < 256) ? pstring[0] : 255;

	if (len == 0) return NULL;
	
	cstring = malloc(len + 1);
	if (cstring != NULL) {
		memcpy(cstring, &(pstring[1]), len);
		cstring[len] = '\0';
	}
	
	return(cstring);
}

/* ------ Font enumeration ------ */
                                                                                
 /* This is used to query the native os for a list of font names and
  * corresponding paths. The general idea is to save the hassle of
  * building a custom fontmap file
  */

typedef struct {
    int size, style, id;
} fond_entry;

typedef struct {
    int entries;
    fond_entry *refs;
} fond_table;

static fond_table *fond_table_new(int entries)
{
    fond_table *table = malloc(sizeof(fond_table));
    if (table != NULL) {
        table->entries = entries;
        table->refs = malloc(entries * sizeof(fond_entry));
        if (table->refs == NULL) { free(table); table = NULL; }
    }
    return table;
}

static void fond_table_free(fond_table *table)
{
    if (table != NULL) {
        if (table->refs) free(table->refs);
        free(table);
    }
}

static fond_table *fond_table_grow(fond_table *table, int entries)
{
    if (table == NULL) {
        table = fond_table_new(entries);
    } else {
        table->entries += entries;
        table->refs = realloc(table->refs, table->entries * sizeof(fond_entry));
    }
    return table;
}

static int get_int16(unsigned char *p) {
    return (p[0]&0xFF)<<8 | (p[1]&0xFF);
}

static int get_int32(unsigned char *p) {
    return (p[0]&0xFF)<<24 | (p[1]&0xFF)<<16 | (p[2]&0xFF)<<8 | (p[3]&0xFF);
}

/* parse and summarize FOND resource information */
static fond_table * parse_fond(FSSpec *spec)
{
    OSErr result = noErr;
    FSRef specref;
    SInt16 ref;
    Handle fond = NULL;
    unsigned char *res;
    fond_table *table = NULL;
    int i,j, count, n, start;
        
	/* FSpOpenResFile will fail for data fork resource (.dfont) files.
	   FSOpenResourceFile can open either, but cannot handle broken resource
	   maps, as often occurs in font files (the suitcase version of Arial,
	   for example) Thus, we try one, and then the other. */
	 
    result = FSpMakeFSRef(spec,&specref);
#ifdef __CARBON__
   	if (result == noErr)
   		result = FSOpenResourceFile(&specref, 0, NULL, fsRdPerm, &ref);
#else
	result = bdNamErr; /* simulate failure of the carbon routine above */
#endif
    if (result != noErr) {
	    ref = FSpOpenResFile(spec, fsRdPerm);
	    result = ResError();
	}
    if (result != noErr || ref <= 0) {
    	char path[256];
    	convertSpecToPath(spec, path, 256);
      	dlprintf2("unable to open resource file '%s' for font enumeration (error %d)\n",
      		path, result);
      	goto fin;
    }
    
    /* we've opened the font file, now loop over the FOND resource(s)
       and construct a table of the font references */
    
    start = 0; /* number of entries so far */
    UseResFile(ref);
    count = Count1Resources('FOND');
    for (i = 0; i < count; i++) {
        fond = Get1IndResource('FOND', i+1);
        if (fond == NULL) {
            result = ResError();
            goto fin;
        }
        
        /* The FOND resource structure corresponds to the FamRec and AsscEntry
           data structures documented in the FontManager reference. However,
           access to these types is deprecated in Carbon. We therefore access the
           data by direct offset in the hope that the resource format will not change
           even if api access to the in-memory versions goes away. */
        HLock(fond);
        res = *fond + 52; /* offset to association table */
        n = get_int16(res) + 1;	res += 2;
		table = fond_table_grow(table, n);
        for (j = start; j < start + n; j++ ) {
            table->refs[j].size = get_int16(res); res += 2;
            table->refs[j].style = get_int16(res); res += 2;
            table->refs[j].id = get_int16(res); res += 2;
        }
        start += n;
        HUnlock(fond);
    }
fin:
    CloseResFile(ref);
    return table;
}

/* FIXME: should check for uppercase as well */
static int is_ttf_file(const char *path)
{
    int len = strlen(path);
    return !memcmp(path+len-4,".ttf",4);
}
static int is_otf_file(const char *path)
{
    int len = strlen(path);
    return !memcmp(path+len-4,".otf",4);
}

static void strip_char(char *string, int len, const int c)
{
    char *bit;
    len += 1;
    while(bit = strchr(string,' ')) {
        memmove(bit, bit + 1, string + len - bit - 1);
    }
}

/* get the macos name for the font instance and mangle it into a PS
   fontname */
static char *makePSFontName(FMFontFamily Family, FMFontStyle Style)
{
	Str255 Name;
	OSStatus result;
	int length;
	char *stylename, *fontname;
	char *psname;
	
	result = FMGetFontFamilyName(Family, Name);
	if (result != noErr) return NULL;
	fontname = MacStr2c(Name);
	if (fontname == NULL) return NULL;
	strip_char(fontname, strlen(fontname), ' ');
	
	switch (Style) {
		case 0: stylename=""; break;;
		case 1: stylename="Bold"; break;;
		case 2: stylename="Italic"; break;;
		case 3: stylename="BoldItalic"; break;;
		default: stylename="Unknown"; break;;
	}
	
	length = strlen(fontname) + strlen(stylename) + 2;
	psname = malloc(length);
	if (Style != 0)
		snprintf(psname, length, "%s-%s", fontname, stylename);
	else
		snprintf(psname, length, "%s", fontname);
		
	free(fontname);
	
	return psname;	
}
                                             
typedef struct {
    int count;
    FMFontIterator Iterator;
    char *name;
    char *path;
    FSSpec last_container;
    char *last_container_path;
    fond_table *last_table;
} fontenum_t;
                                                                                
void *gp_enumerate_fonts_init(gs_memory_t *mem)
{
    fontenum_t *state = gs_alloc_bytes(mem, sizeof(fontenum_t),
	"macos font enumerator state");
	FMFontIterator *Iterator = &state->Iterator;
	OSStatus result;
    
    if (state != NULL) {
		state->count = 0;
		state->name = NULL;
		state->path = NULL;
		result = FMCreateFontIterator(NULL, NULL,
			kFMLocalIterationScope, Iterator);
		if (result != noErr) return NULL;
		memset(&state->last_container, 0, sizeof(FSSpec));
		state->last_container_path = NULL;
		state->last_table = NULL;
    }

    return (void *)state;
}

void gp_enumerate_fonts_free(void *enum_state)
{
    fontenum_t *state = (fontenum_t *)enum_state;
	FMFontIterator *Iterator = &state->Iterator;
	
	FMDisposeFontIterator(Iterator);
	
    /* free any malloc'd stuff here */
    if (state->name) free(state->name);
    if (state->path) free(state->path);
    if (state->last_container_path) free(state->last_container_path);
    if (state->last_table) fond_table_free(state->last_table);
    /* the garbage collector will take care of the struct itself */
    
}
                                   
int gp_enumerate_fonts_next(void *enum_state, char **fontname, char **path)
{
    fontenum_t *state = (fontenum_t *)enum_state;
	FMFontIterator *Iterator = &state->Iterator;
	FMFont Font;
	FourCharCode Format;
	FMFontFamily FontFamily;
	FMFontStyle Style;
	FSSpec FontContainer;
	char type[5];
	char fontpath[256];
	char *psname;
	fond_table *table = NULL;
	OSStatus result;
    	
	result = FMGetNextFont(Iterator, &Font);
    if (result != noErr) return 0; /* no more fonts */

	result = FMGetFontFormat(Font, &Format);
	type[0] = ((char*)&Format)[0];
	type[1] = ((char*)&Format)[1];
	type[2] = ((char*)&Format)[2];
	type[3] = ((char*)&Format)[3];
	type[4] = '\0';

 	FMGetFontFamilyInstanceFromFont(Font, &FontFamily, &Style);
    if (state->name) free (state->name);
    
    psname = makePSFontName(FontFamily, Style);
    if (psname == NULL) {
		state->name = strdup("GSPlaceHolder");
	} else {
		state->name = psname;
	}
    	
	result = FMGetFontContainer(Font, &FontContainer);
	if (!memcmp(&FontContainer, &state->last_container, sizeof(FSSpec))) {
		/* we have cached data on this file */
		strncpy(fontpath, state->last_container_path, 256);
		table = state->last_table;
	} else {
		convertSpecToPath(&FontContainer, fontpath, 256);
		if (!is_ttf_file(fontpath) && !is_otf_file(fontpath))
	    	table = parse_fond(&FontContainer);
	    /* cache data on the new font file */
	    memcpy(&state->last_container, &FontContainer, sizeof(FSSpec));
	    if (state->last_container_path) free (state->last_container_path);
		state->last_container_path = strdup(fontpath);
		if (state->last_table) fond_table_free(state->last_table);
		state->last_table = table;
	}
	
	if (state->path) {
		free(state->path);
		state->path = NULL;
	}
    if (table != NULL) {
    	int i;
    	for (i = 0; i < table->entries; i++) {
            if (table->refs[i].size == 0) { /* ignore non-scalable fonts */
                if (table->refs[i].style == Style) {
                    int len = strlen(fontpath) + strlen("%macresource%#sfnt+") + 6;
                	state->path = malloc(len);
                    snprintf(state->path, len, "%%macresource%%%s#sfnt+%d", 
                        fontpath, table->refs[i].id);
                    break;
                }
            }
        }
    } else {
        /* regular font file */
        state->path = strdup(fontpath);
    }
    if (state->path == NULL) {
    	/* no matching font was found in the FOND resource table. this usually */
    	/* means an LWFN file, which we don't handle yet. */
    	/* we still specify these with a %macresource% path, but no res id */
    	/* TODO: check file type */
    	int len = strlen(fontpath) + strlen("%macresource%#POST") + 1;
    	state->path = malloc(len);
    	snprintf(state->path, len, "%%macresource%%%s#POST", fontpath);
    }
#ifdef DEBUG
    dlprintf2("fontenum: returning '%s' in '%s'\n", state->name, state->path);
#endif
    *fontname = state->name;
    *path = state->path;

	state->count += 1;
	return 1;
}
                                                                                
