/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* argmatch.c -- find a match for a string in an array
   Copyright (C) 1990, 1997 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <argmatch.h>

#include <sys/types.h>

#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

/* If ARG is an unambiguous match for an element of the
   null-terminated array OPTLIST, return the index in OPTLIST
   of the matched element, else -1 if it does not match any element
   or -2 if it is ambiguous (is a prefix of more than one element).  */

int
    argmatch(arg, optlist)
	const char *arg;
const char *const *optlist;
{
	int i;		   /* Temporary index in OPTLIST.  */
	size_t arglen;     /* Length of ARG.  */
	int matchind = -1; /* Index of first nonexact match.  */
	int ambiguous = 0; /* If nonzero, multiple nonexact match(es).  */

	arglen = strlen(arg);

	/* Test all elements for either exact match or abbreviated matches.  */
	for(i = 0; optlist[i]; i++) {
		if(!strncmp(optlist[i], arg, arglen)) {
			if(strlen(optlist[i]) == arglen)
				/* Exact match found.  */
				return i;
			else if(matchind == -1)
				/* First nonexact match found.  */
				matchind = i;
			else
				/* Second nonexact match found.  */
				ambiguous = 1;
		}
	}
	if(ambiguous)
		return -2;
	else
		return matchind;
}

/* Error reporting for argmatch.
   KIND is a description of the type of entity that was being matched.
   VALUE is the invalid value that was given.
   PROBLEM is the return value from argmatch.  */

void
    invalid_arg(kind, value, problem)
	const char *kind;
const char *value;
int problem;
{
	fprintf(stderr, "%s: ", program_name);
	if(problem == -1)
		fprintf(stderr, "invalid");
	else /* Assume -2.  */
		fprintf(stderr, "ambiguous");
	fprintf(stderr, " %s `%s'\n", kind, value);
}
