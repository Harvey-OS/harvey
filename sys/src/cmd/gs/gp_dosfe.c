/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_dosfe.c */
/* MS-DOS file enumeration. */
#include "stdio_.h"
#include <fcntl.h>
#include "dos_.h"
#include "memory_.h"
#include "string_.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsstruct.h"
#include "gp.h"
#include "gsutil.h"

struct file_enum_s {
	ff_struct_t ffblk;
	char *pattern;			/* orig pattern + modified pattern */
	int patlen;			/* orig pattern length */
	int pat_size;			/* allocate space for pattern */
	int head_size;			/* pattern length through last */
					/* :, / or \ */
	int first_time;
	gs_memory_t *memory;
};
gs_private_st_ptrs1(st_file_enum, struct file_enum_s, "file_enum",
  file_enum_enum_ptrs, file_enum_reloc_ptrs, pattern);

/* Initialize an enumeration.  Note that * and ? in a directory */
/* don't work, and \ is taken literally unless a second \ follows. */
file_enum *
gp_enumerate_files_init(const char *pat, uint patlen, gs_memory_t *mem)
{	file_enum *pfen = gs_alloc_struct(mem, file_enum, &st_file_enum, "gp_enumerate_files");
	int pat_size = 2 * patlen + 1;
	char *pattern;
	char *p;
	int hsize = 0;
	int i;
	int dot = 0;
	if ( pfen == 0 )
		return 0;

	/* pattern could be allocated as a string, */
	/* but it's simpler for GC and freeing to allocate it as bytes. */

	pattern = (char *)gs_alloc_bytes(mem, pat_size,
					 "gp_enumerate_files(pattern)");
	if ( pattern == 0 )
		return 0;
	memcpy(pattern, pat, patlen);
	p = pattern + patlen;
	for ( i = 0; i < patlen; i++ )
	{	switch ( pat[i] )
		{
		case '*':
			/* Skip to . or end of string so DOS can do it. */
			*p++ = '*';
			while ( i < patlen && pat[i] != '.' ) i++;
			if ( i == patlen && !dot )
			{	/* DOS doesn't interpret * alone as */
				/* matching all files; we need *.*. */
				*p++ = '.';
				*p++ = '*';
			}
			i--;
			continue;
		case '.':
			dot = 1;
			break;
		case '\\':
			if ( i + 1 < patlen && pat[i + 1] == '\\' )
				i++;
			/* falls through */
		case ':':
		case '/':
			hsize = p + 1 - (pattern + patlen);
			dot = 0;
		}
		*p++ = pat[i];
	}
	*p = 0;
	pfen->pattern = pattern;
	pfen->patlen = patlen;
	pfen->pat_size = pat_size;
	pfen->head_size = hsize;
	pfen->memory = mem;
	pfen->first_time = 1;
	return pfen;
}

/* Enumerate the next file. */
private const string_match_params smp_file = { '*', '?', -1, true };
uint
gp_enumerate_files_next(file_enum *pfen, char *ptr, uint maxlen)
{	int code;
	char *p, *q;
	uint len;
	const char *fpat = pfen->pattern + pfen->patlen;
top:	if ( pfen->first_time )
	   {	code = dos_findfirst(fpat, &pfen->ffblk);
		pfen->first_time = 0;
	   }
	else
		code = dos_findnext(&pfen->ffblk);
	if ( code != 0 )
	   {	/* All done, clean up. */
		gp_enumerate_files_close(pfen);
		return ~(uint)0;
	   }
	if ( maxlen < 13 + pfen->head_size ) return maxlen + 1;	/* cop out! */
	memcpy(ptr, fpat, pfen->head_size);
	for ( p = &pfen->ffblk.ff_name[0], q = ptr + pfen->head_size; *p; p++ )
		if ( *p != ' ' ) *q++ = *p;
	len = q - ptr;
	/* Make sure this file really matches the pattern. */
	if ( !string_match(ptr, len, pfen->pattern, pfen->patlen, &smp_file) )
		goto top;
	return len;
}

/* Clean up the file enumeration. */
void
gp_enumerate_files_close(file_enum *pfen)
{	gs_memory_t *mem = pfen->memory;
	gs_free_object(mem, pfen->pattern,
		       "gp_enumerate_files_close(pattern)");
	gs_free_object(mem, pfen, "gp_enumerate_files_close");
}

