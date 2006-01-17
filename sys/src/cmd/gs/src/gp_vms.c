/* Copyright (C) 1989-2003 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_vms.c,v 1.38 2004/04/08 16:18:25 giles Exp $ */
/* VAX/VMS specific routines for Ghostscript */

#include "string_.h"
#include "memory_.h"
#include "gx.h"
#include "gp.h"
#include "gpmisc.h"
#include "gsstruct.h"
#include <stat.h>
#include <stdlib.h>		/* for exit() with some compiler versions */
#include <errno.h>		/* for exit() with other compiler versions */
#include <unixio.h>

extern char *getenv(const char *);

/* Apparently gcc doesn't allow extra arguments for fopen: */
#ifdef VMS			/* DEC C */
#  define fopen_VMS fopen
#else /* gcc */
#  define fopen_VMS(name, mode, m1, m2) fopen(name, mode)
#endif


/* VMS string descriptor structure */
#define DSC$K_DTYPE_T 14
#define DSC$K_CLASS_S  1
struct dsc$descriptor_s {
    unsigned short dsc$w_length;
    unsigned char dsc$b_dtype;
    unsigned char dsc$b_class;
    char *dsc$a_pointer;
};
typedef struct dsc$descriptor_s descrip;

/* VMS RMS constants */
#define RMS_IS_ERROR_OR_NMF(rmsv) (((rmsv) & 1) == 0)
#define RMS$_NMF    99018
#define RMS$_NORMAL 65537
#define NAM$C_MAXRSS  255

struct file_enum_s {
    uint context, length;
    descrip pattern;
    gs_memory_t *memory;
};
gs_private_st_ptrs1(st_file_enum, struct file_enum_s, "file_enum",
	  file_enum_enum_ptrs, file_enum_reloc_ptrs, pattern.dsc$a_pointer);

extern uint
    LIB$FIND_FILE(descrip *, descrip *, uint *, descrip *, descrip *,
		  uint *, uint *),
    LIB$FIND_FILE_END(uint *),
    SYS$FILESCAN(descrip *, uint *, uint *),
    SYS$PUTMSG(uint *, int (*)(), descrip *, uint);

private uint
strlength(char *str, uint maxlen, char term)
{
    uint i = 0;

    while (i < maxlen && str[i] != term)
	i++;
    return i;
}

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
{				/* The program returns exit_status = 0 for OK, 1 for failure; */
    /* VMS has different conventions. */
    switch (exit_status) {
	case 0:
	    exit(exit_OK);
	case 1:
	    exit(exit_FAILED);
    }
    exit(exit_status);
}

/* ------ Date and time ------ */

/* Read the current time (in seconds since Jan. 1, 1980) */
/* and fraction (in nanoseconds). */
void
gp_get_realtime(long *pdt)
{
    struct {
	uint _l0, _l1;
    } binary_date, now, difference;
    long LIB$EDIV(), LIB$SUBX(), SYS$BINTIM(), SYS$GETTIM();
    long units_per_second = 10000000;
    char *jan_1_1980 = "1-JAN-1980 00:00:00.00";
    descrip str_desc;

    /* For those interested, Wednesday, November 17, 1858 is the base
       of the Modified Julian Day system adopted by the Smithsonian
       Astrophysical Observatory in 1957 for satellite tracking.  (The
       year 1858 preceded the oldest star catalog in use at the
       observatory.)  VMS uses quadword time stamps which are offsets
       in 100 nanosecond units from November 17, 1858.  With a 63-bit
       absolute time representation (sign bit must be clear), VMS will
       have no trouble with time until 31-JUL-31086 02:48:05.47. */

    /* Convert January 1, 1980 into a binary absolute time */
    str_desc.dsc$w_length = strlen(jan_1_1980);
    str_desc.dsc$a_pointer = jan_1_1980;
    (void)SYS$BINTIM(&str_desc, &binary_date);

    /* Compute number of 100 nanosecond units since January 1, 1980.  */
    (void)SYS$GETTIM(&now);
    (void)LIB$SUBX(&now, &binary_date, &difference);

    /* Convert to seconds and nanoseconds.  */
    (void)LIB$EDIV(&units_per_second, &difference, &pdt[0], &pdt[1]);
    pdt[1] *= 100;
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

/* ------ Screen management ------ */

/* Get the environment variable that specifies the display to use. */
const char *
gp_getenv_display(void)
{
    return getenv("DECW$DISPLAY");
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char fname[gp_file_name_sizeof], int binary_mode)
{
    if (strlen(fname) == 0)
	return 0;
    if (binary_mode) {		/*
				 * Printing must be done exactly byte to byte,
				 * using "passall".  However the standard VMS symbiont
				 * does not treat stream-LF files correctly in this respect,
				 * but throws away \n characters.  Giving the file
				 * the record type "undefined", but accessing it as a
				 * normal stream-LF file does the trick.
				 */
	return fopen_VMS(fname, "w", "rfm = udf", "ctx = stm");
    } else {			/* Open as a normal text stream file. */
	return fopen_VMS(fname, "w", "rfm = var", "rat = cr");
    }
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE * pfile, const char *fname)
{
    fclose(pfile);
}

/* ------ File naming and accessing ------ */

/* Define the character used for separating file names in a list. */
const char gp_file_name_list_separator = ',';

/* Define the default scratch file name prefix. */
const char gp_scratch_file_name_prefix[] = "_temp_";

/* Define the name of the null output file. */
const char gp_null_file_name[] = "NLA0:";

/* Define the name that designates the current directory. */
const char gp_current_directory_name[] = "[]";

/* Define the string to be concatenated with the file mode */
/* for opening files without end-of-line conversion. */
const char gp_fmode_binary_suffix[] = "";

/* Define the file modes for binary reading or writing. */
const char gp_fmode_rb[] = "r";
const char gp_fmode_wb[] = "w";

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *
gp_open_scratch_file(const char *prefix, char fname[gp_file_name_sizeof],
		     const char *mode)
{
    FILE *f;
    char tmpdir[gp_file_name_sizeof];
    int tdlen = gp_file_name_sizeof;
    int flen[1];

    if (!gp_file_name_is_absolute(prefix, strlen(prefix)) &&
	gp_gettmpdir(tmpdir, &tdlen) == 0) {
      flen[0] = gp_file_name_sizeof;
	if (gp_file_name_combine(tmpdir, tdlen, prefix, strlen(prefix),
			     false, fname, flen ) != gp_combine_success ) {
	    return NULL;
	}
       fname[ *flen ] = 0;
    } else {
	strcpy(fname, prefix);
    }
    if (strlen(fname) + 6 >= gp_file_name_sizeof)
	return 0;		/* file name too long */
    strcat(fname, "XXXXXX");
   mktemp(fname);
    f = fopen(fname, mode);
   
    if (f == NULL)
	eprintf1("**** Could not open temporary file %s\n", fname);
   return f;
}

/* Open a file with the given name, as a stream of uninterpreted bytes. */
/* We have to do something special if the file was FTP'ed in binary mode. */
/* Unfortunately, only DEC C supports the extra arguments to fopen. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
#ifdef __DECC
#define FAB$C_FIX 1
    stat_t buffer;

    if (stat((char *)fname, &buffer) == 0)
	if (buffer.st_fab_rfm == FAB$C_FIX)
	    return fopen(fname, mode, "rfm=stmlf", "ctx=stm");
#endif
    return fopen(fname, mode);
}

/* Set a file into binary or text mode. */
int
gp_setmode_binary(FILE * pfile, bool binary)
{
    return 0;			/* Noop under VMS */
}

/* ------ Wild card file search procedures ------ */

private void
gp_free_enumeration(file_enum * pfen)
{
    if (pfen) {
	LIB$FIND_FILE_END(&pfen->context);
	gs_free_object(pfen->memory, pfen->pattern.dsc$a_pointer,
		       "GP_ENUM(pattern)");
	gs_free_object(pfen->memory, pfen,
		       "GP_ENUM(file_enum)");
    }
}

/* Begin an enumeration.  See gp.h for details. */

file_enum *
gp_enumerate_files_init(const char *pat, uint patlen, gs_memory_t * mem)
{
    file_enum *pfen;
    uint i, len;
    char *c, *newpat;
    bool dot_in_filename = false;

    pfen = gs_alloc_struct(mem, file_enum, &st_file_enum,
			   "GP_ENUM(file_enum)");
    newpat = (char *)gs_alloc_bytes(mem, patlen + 2, "GP_ENUM(pattern)");
    if (pfen == 0 || newpat == 0) {
	gs_free_object(mem, newpat, "GP_ENUM(pattern)");
	gs_free_object(mem, pfen, "GP_ENUM(file_enum)");
	return (file_enum *) 0;
    }
    /*  Copy the pattern removing backslash quoting characters and
     *  transforming unquoted question marks, '?', to percent signs, '%'.
     *  (VAX/VMS uses the wildcard '%' to represent exactly one character
     *  and '*' to represent zero or more characters.  Any combination and
     *  number of interspersed wildcards is permitted.)
     *
     *  Since VMS requires "*.*" to actually return all files, we add a
     *  special check for a path ending in "*" and change it into "*.*"
     *  if a "." wasn't part of the file spec. Thus "[P.A.T.H]*" becomes
     *  "[P.A.T.H]*.*" but "[P.A.T.H]*.*" or "[P.A.T.H]*.X*" are unmodified.
     */
    c = newpat;
    for (i = 0; i < patlen; pat++, i++)
	switch (*pat) {
	    case '?':
		*c++ = '%';
		break;
	    case '\\':
		i++;
		if (i < patlen)
		    *c++ = *++pat;
		break;
	    case '.':
	    case ']':
		dot_in_filename = *pat == '.'; 
	    default:
		*c++ = *pat;
		break;
	}
    /* Check for trailing "*" and see if we need to add ".*" */
    if (pat[-1] == '*' && !dot_in_filename) {
	*c++ = '.';
	*c++ = '*';
    }
    len = c - newpat;

    /* Pattern may not exceed 255 characters */
    if (len > 255) {
	gs_free_object(mem, newpat, "GP_ENUM(pattern)");
	gs_free_object(mem, pfen, "GP_ENUM(file_enum)");
	return (file_enum *) 0;
    }
    pfen->context = 0;
    pfen->length = patlen;
    pfen->pattern.dsc$w_length = len;
    pfen->pattern.dsc$b_dtype = DSC$K_DTYPE_T;
    pfen->pattern.dsc$b_class = DSC$K_CLASS_S;
    pfen->pattern.dsc$a_pointer = newpat;
    pfen->memory = mem;

    return pfen;
}

/* Return the next file name in the enumeration.  The client passes in */
/* a scratch string and a max length.  If the name of the next file fits, */
/* the procedure returns the length.  If it doesn't fit, the procedure */
/* returns max length +1.  If there are no more files, the procedure */
/* returns -1. */

uint
gp_enumerate_files_next(file_enum * pfen, char *ptr, uint maxlen)
{
    char *c, filnam[NAM$C_MAXRSS];
    descrip result =
    {NAM$C_MAXRSS, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
    uint i, len;

    result.dsc$a_pointer = filnam;

    /* Find the next file which matches the pattern */
    i = LIB$FIND_FILE(&pfen->pattern, &result, &pfen->context,
		      (descrip *) 0, (descrip *) 0, (uint *) 0, (uint *) 0);

    /* Check the return status */
    if (RMS_IS_ERROR_OR_NMF(i)) {
	gp_free_enumeration(pfen);
	return (uint)(-1);
    } else if ((len = strlength(filnam, NAM$C_MAXRSS, ' ')) > maxlen)
	return maxlen + 1;

    /* Copy the returned filename over to the input string ptr */
    c = ptr;
    for (i = 0; i < len; i++)
	*c++ = filnam[i];

    return len;
}

/* Clean up a file enumeration.  This is only called to abandon */
/* an enumeration partway through: ...next should do it if there are */
/* no more files to enumerate.  This should deallocate the file_enum */
/* structure and any subsidiary structures, strings, buffers, etc. */

void
gp_enumerate_files_close(file_enum * pfen)
{
    gp_free_enumeration(pfen);
}

const char *
gp_strerror(int errnum)
{
    return NULL;
}

/* -------------- Helpers for gp_file_name_combine_generic ------------- */

uint gp_file_name_root(const char *fname, uint len)
{   
    /*
     *    The root for device:[root.][directory.subdirectory]filename.extension;version
     *	    is device:[root.][
     *    The root for device:[directory.subdirectory]filename.extension;version
     *	    is device:[
     *    The root for logical:filename.extension;version
     *	    is logical:
     */
    int i, j;

    if (len == 0)
	return 0;
    /* Search for ':' */
    for (i = 0; i < len; i++)
	if (fname[i] == ':')
	    break;
    if (i == len)
	return 0; /* No root. */
    if (fname[i] == ':')
	i++;
    if (i == len || fname[i] != '[')
	return i; 
    /* Search for ']' */
    i++;
    for (j = i; j < len; j++)
	if (fname[j] == ']')
	    break;
    if (j == len)
	return i; /* No ']'. Allowed as a Ghostscript specifics. */
    j++;
    if (j == len)
	return i; /* Appending "device:[directory.subdirectory]" with "filename.extension;version". */
    if (fname[j] != '[')
	return i; /* Can't append anything, but pass through for checking an absolute path. */
    return j + 1; /* device:[root.][ */
}

uint gs_file_name_check_separator(const char *fname, int len, const char *item)
{   
    if (len > 0) {
	/* 
	 * Ghostscript specifics : an extended syntax like Mac OS.
	 * We intentionally don't consider ':' and '[' as separators
	 * in forward search, see gp_file_name_combine. 
	 */
	if (fname[0] == ']')
	    return 1; /* It is a file separator. */
	if (fname[0] == '.')
	    return 1; /* It is a directory separator. */
	if (fname[0] == '-') {
	    if (fname == item + 1 && item[0] == '-')
		return 1; /* Two or more parents, cut the first one. */
	    return 1;
	}
    } else if (len < 0) {
	if (fname[-1] == '.' || fname[-1] == ':' || fname[-1] == '[')
	    return 1;
    }
    return 0;
}

bool gp_file_name_is_parent(const char *fname, uint len)
{   /* Ghostscript specifics : an extended syntax like Mac OS. */
    return len == 1 && fname[0] == '-';
}

bool gp_file_name_is_current(const char *fname, uint len)
{   /* Ghostscript specifics : an extended syntax like Mac OS. */
    return len == 0;
}

const char *gp_file_name_separator(void)
{   return "]";
}

const char *gp_file_name_directory_separator(void)
{   return ".";
}

const char *gp_file_name_parent(void)
{   return "-";
}

const char *gp_file_name_current(void)
{   return "";
}

bool gp_file_name_is_partent_allowed(void)
{   return false;
}

bool gp_file_name_is_empty_item_meanful(void)
{   return true;
}

gp_file_name_combine_result
gp_file_name_combine(const char *prefix, uint plen, const char *fname, uint flen, 
		    bool no_sibling, char *buffer, uint *blen)
{
    /*
     * Reduce it to the general case.
     *
     * Implementation restriction : fname must not contain a part of 
     * "device:[root.]["
     */
    uint rlen, flen1 = flen, plen1 = plen;
    const char *fname1 = fname;
  
   if ( plen > 0 && prefix[plen-1] == '\0' )
     plen--;
   
    if (plen == 0 && flen == 0) {
	/* Not sure that we need this case. */
	if (*blen == 0)
	    return gp_combine_small_buffer;
	buffer[0] = '.';
	*blen = 1;
    }
    rlen = gp_file_name_root(fname, flen);
    if (rlen > 0 || plen == 0 || flen == 0) {
	if (rlen == 0 && plen != 0) {
	    fname1 = prefix;
	    flen1 = plen;
	}
	if (flen1 + 1 > *blen)
	    return gp_combine_small_buffer;
	memcpy(buffer, fname1, flen1);
	buffer[flen1] = 0;
	*blen = flen1;
	return gp_combine_success;
    }
   
   if ( prefix[plen - 1] == ']' && fname[ 0 ] == '-' )
     {
	memcpy(buffer, prefix, plen - 1 );
	fname1 = fname + 1;
	flen1 = flen - 1;
	memcpy(buffer + plen - 1 , fname1, flen1);
	memcpy(buffer + plen + flen1 - 1 , "]" , 1 );
	buffer[plen + flen1] = 0;
	*blen = plen + flen1;
	return gp_combine_success;
     }

   if ( prefix[plen - 1] == ':' || (prefix[plen - 1] == ']' &&
				     memchr(fname, ']', flen) == 0) )
       {
	/* Just concatenate. */
	if (plen + flen + 1 > *blen)
	    return gp_combine_small_buffer;
	memcpy(buffer, prefix, plen);
	memcpy(buffer + plen, fname, flen);
	buffer[plen + flen] = 0;
	*blen = plen + flen;
	return gp_combine_success;
    }
   if ( memchr( prefix , '[' , plen ) == 0 &&
	memchr( prefix , '.' , plen ) == 0 )
     {
	char* tmp_prefix;
	int tmp_plen;
	
	if ( prefix[0] == '/' )
	  {
	     tmp_prefix = prefix + 1;
	     tmp_plen = plen - 1;
	  }
	else
	  {
	     tmp_prefix = prefix;
	     tmp_plen = plen;
	  }
	if ( tmp_plen + flen + 2 > *blen)
	    return gp_combine_small_buffer;
	memcpy(buffer, tmp_prefix, tmp_plen);
	memcpy(buffer + tmp_plen , ":" , 1 );
	memcpy(buffer + tmp_plen + 1, fname, flen);
	if ( memchr( fname , '.' , flen ) != 0 )
	  {
	     buffer[ tmp_plen + flen + 1] = 0;
	     *blen = tmp_plen + flen + 1;
	  }
	else
	  {
	     memcpy(buffer + tmp_plen + flen + 1 , "." , 1 );
	     buffer[ tmp_plen + flen + 2] = 0;
	     *blen = tmp_plen + flen + 2;
	  }
	return gp_combine_success;
     }
    if (prefix[plen - 1] != ']' && fname[0] == '[')
        return gp_combine_cant_handle;
    /* Unclose "][" :*/
    if (fname[0] == '[') {
	fname1 = fname + 1;
	flen1 = flen - 1;
    }
    if (prefix[plen - 1] == ']')
        plen1 = plen - 1;
    return gp_file_name_combine_generic(prefix, plen1, 
	    fname1, flen1, no_sibling, buffer, blen);
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

