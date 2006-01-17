/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_ntfs.c,v 1.22 2003/12/10 23:07:10 ray Exp $ */
/* file system stuff for MS-Windows WIN32 and MS-Windows NT */
/* hacked from gp_dosfs.c by Russell Lang */

#include "stdio_.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include "memory_.h"
#include "string_.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsstruct.h"
#include "gp.h"
#include "gpmisc.h"
#include "gsutil.h"
#include "windows_.h"

/* ------ Printer accessing ------ */

/* Put a printer file (which might be stdout) into binary or text mode. */
/* This is not a standard gp procedure, */
/* but all MS-DOS configurations need it. */
private int
setmode_binary(int fno, bool binary)
{
    /* Use non-standard setmode that almost all NT compilers offer. */
#if defined(__STDC__) && !defined(__WATCOMC__)
    return _setmode(fno, binary ? _O_BINARY : _O_TEXT);
#else
    return setmode(fno, binary ? O_BINARY : O_TEXT);
#endif
}
void
gp_set_file_binary(int prnfno, int binary)
{
    DISCARD(setmode_binary(prnfno, binary != 0));
}

/* ------ File accessing -------- */

/* Set a file into binary or text mode. */
int
gp_setmode_binary(FILE * pfile, bool binary)
{
    /* Use non-standard fileno that almost all NT compilers offer. */
#if defined(__STDC__) && !defined(__WATCOMC__)
    int code = setmode_binary(_fileno(pfile), binary);
#else
    int code = setmode_binary(fileno(pfile), binary);
#endif

    return (code == -1 ? -1 : 0);
}

/* ------ File names ------ */

/* Define the character used for separating file names in a list. */
const char gp_file_name_list_separator = ';';

/* Define the string to be concatenated with the file mode */
/* for opening files without end-of-line conversion. */
const char gp_fmode_binary_suffix[] = "b";

/* Define the file modes for binary reading or writing. */
const char gp_fmode_rb[] = "rb";
const char gp_fmode_wb[] = "wb";

/* ------ File enumeration ------ */

struct file_enum_s {
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    char *pattern;		/* orig pattern + modified pattern */
    int patlen;			/* orig pattern length */
    int pat_size;		/* allocate space for pattern */
    int head_size;		/* pattern length through last */
    /* :, / or \ */
    int first_time;
    gs_memory_t *memory;
};
gs_private_st_ptrs1(st_file_enum, struct file_enum_s, "file_enum",
		    file_enum_enum_ptrs, file_enum_reloc_ptrs, pattern);

/* Initialize an enumeration.  Note that * and ? in a directory */
/* don't work with the OS call currently used. The '\' escape	*/
/* character is removed for the 'Find...File' function.		*/
file_enum *
gp_enumerate_files_init(const char *pat, uint patlen, gs_memory_t * mem)
{
    file_enum *pfen = gs_alloc_struct(mem, file_enum, &st_file_enum, "gp_enumerate_files");
    int pat_size = 2 * patlen + 1;
    char *pattern;
    int hsize = 0;
    int i, j;

    if (pfen == 0)
	return 0;
    /* pattern could be allocated as a string, */
    /* but it's simpler for GC and freeing to allocate it as bytes. */
    pattern = (char *)gs_alloc_bytes(mem, pat_size,
				     "gp_enumerate_files(pattern)");
    if (pattern == 0)
	return 0;
    /* translate the template into a pattern discarding the escape  */
    /* char '\' (not needed by the OS Find...File logic). Note that */
    /* a final '\' in the string is also discarded.		    */
    for (i = 0, j=0; i < patlen; i++) {
	if (pat[i] == '\\') {
	    i++;
	    if (i == patlen)
		break;		/* '\' at end ignored */
	}
	pattern[j++]=pat[i];
    }
    /* Scan for last path separator to determine 'head_size' (directory part) */
    for (i = 0; i < j; i++) {
	if(pattern[i] == '/' || pattern[i] == '\\' || pattern[i] == ':')
	hsize = i+1;
    }
    pattern[j] = 0;
    pfen->pattern = pattern;
    pfen->patlen = j;
    pfen->pat_size = pat_size;
    pfen->head_size = hsize;
    pfen->memory = mem;
    pfen->first_time = 1;
    memset(&pfen->find_data, 0, sizeof(pfen->find_data));
    pfen->find_handle = INVALID_HANDLE_VALUE;
    return pfen;
}

/* Enumerate the next file. */
uint
gp_enumerate_files_next(file_enum * pfen, char *ptr, uint maxlen)
{
    int code = 0;
    uint len;
    for(;;) 
      { if (pfen->first_time) 
          { pfen->find_handle = FindFirstFile(pfen->pattern, &(pfen->find_data));
	    if (pfen->find_handle == INVALID_HANDLE_VALUE)
	      { code = -1;
                break;
              }
	    pfen->first_time = 0;
          } 
        else 
          { if (!FindNextFile(pfen->find_handle, &(pfen->find_data)))
	      { code = -1;
                break;
              }
          }
        if ( strcmp(".",  pfen->find_data.cFileName)
          && strcmp("..", pfen->find_data.cFileName)
	  && (pfen->find_data.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY))
            break;
      } 
   
    if (code != 0) {		/* All done, clean up. */
	gp_enumerate_files_close(pfen);
	return ~(uint) 0;
    }
    len = strlen(pfen->find_data.cFileName);

    if (pfen->head_size + len < maxlen) {
	memcpy(ptr, pfen->pattern, pfen->head_size);
	strcpy(ptr + pfen->head_size, pfen->find_data.cFileName);
	return pfen->head_size + len;
    }
    if (pfen->head_size >= maxlen)
	return 0;		/* no hope at all */

    memcpy(ptr, pfen->pattern, pfen->head_size);
    strncpy(ptr + pfen->head_size, pfen->find_data.cFileName,
	    maxlen - pfen->head_size - 1);
    return maxlen;
}

/* Clean up the file enumeration. */
void
gp_enumerate_files_close(file_enum * pfen)
{
    gs_memory_t *mem = pfen->memory;

    if (pfen->find_handle != INVALID_HANDLE_VALUE)
	FindClose(pfen->find_handle);
    gs_free_object(mem, pfen->pattern,
		   "gp_enumerate_files_close(pattern)");
    gs_free_object(mem, pfen, "gp_enumerate_files_close");
}


/* -------------- Helpers for gp_file_name_combine_generic ------------- */

uint gp_file_name_root(const char *fname, uint len)
{   int i = 0;
    
    if (len == 0)
	return 0;
    if (len > 1 && fname[0] == '\\' && fname[1] == '\\') {
	/* A network path: "\\server\share\" */
	int k = 0;

	for (i = 2; i < len; i++)
	    if (fname[i] == '\\' || fname[i] == '/')
		if (k++) {
		    i++;
		    break;
		}
    } else if (fname[0] == '/' || fname[0] == '\\') {
	/* Absolute with no drive. */
	i = 1;
    } else if (len > 1 && fname[1] == ':') {
	/* Absolute with a drive. */
	i = (len > 2 && (fname[2] == '/' || fname[2] == '\\') ? 3 : 2);
    }
    return i;
}

uint gs_file_name_check_separator(const char *fname, int len, const char *item)
{   if (len > 0) {
	if (fname[0] == '/' || fname[0] == '\\')
	    return 1;
    } else if (len < 0) {
	if (fname[-1] == '/' || fname[-1] == '\\')
	    return 1;
    }
    return 0;
}

bool gp_file_name_is_parent(const char *fname, uint len)
{   return len == 2 && fname[0] == '.' && fname[1] == '.';
}

bool gp_file_name_is_current(const char *fname, uint len)
{   return len == 1 && fname[0] == '.';
}

const char *gp_file_name_separator(void)
{   return "/";
}

const char *gp_file_name_directory_separator(void)
{   return "/";
}

const char *gp_file_name_parent(void)
{   return "..";
}

const char *gp_file_name_current(void)
{   return ".";
}

bool gp_file_name_is_partent_allowed(void)
{   return true;
}

bool gp_file_name_is_empty_item_meanful(void)
{   return false;
}

gp_file_name_combine_result
gp_file_name_combine(const char *prefix, uint plen, const char *fname, uint flen, 
		    bool no_sibling, char *buffer, uint *blen)
{
    return gp_file_name_combine_generic(prefix, plen, 
	    fname, flen, no_sibling, buffer, blen);
}

