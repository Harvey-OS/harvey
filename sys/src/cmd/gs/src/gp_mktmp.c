/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_mktmp.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Replacement for missing mktemp */
#include "stat_.h"
#include "string_.h"

/* This procedure simulates mktemp on platforms that don't provide it. */
char *
mktemp(char *fname)
{
    struct stat fst;
    int len = strlen(fname);
    char *end = fname + len - 6;
    
    if (len < 6 || strcmp(end, "XXXXXX"))
	return (char *)0;	/* invalid  */
    strcpy(end, "AA.AAA");

    while (stat(fname, &fst) == 0) {
	char *inc = fname + len - 1;

	while (*inc == 'Z' || *inc == '.') {
	    if (inc == end)
		return (char *)0;	/* failure */
	    if (*inc == 'Z')
		*inc = 'A';
	    --inc;
	}
	++*inc;
    }
    return fname;
}
