/* $Source: /u/mark/src/pax/RCS/wildmat.c,v $
 *
 * $Revision: 1.2 $
 *
 * wildmat.c - simple regular expression pattern matching routines 
 *
 * DESCRIPTION 
 *
 * 	These routines provide simple UNIX style regular expression matching.  
 *	They were originally written by Rich Salz, the comp.sources.unix 
 *	moderator for inclusion in some of his software.  These routines 
 *	were released into the public domain and used by John Gilmore in 
 *	USTAR. 
 *
 * AUTHORS 
 *
 * 	Mark H. Colburn, NAPS International (mark@jhereg.mn.org) 
 * 	John Gilmore (gnu@hoptoad) 
 * 	Rich Salz (rs@uunet.uu.net) 
 *
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	wildmat.c,v $
 * Revision 1.2  89/02/12  10:06:20  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:41  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: wildmat.c,v 1.2 89/02/12 10:06:20 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__
static int star(char *, char *);
#else  /* !__STDC__ */
static int      star();
#endif /* __STDC__ */


/*
 * star - handle trailing * in a regular expression 
 *
 * DESCRIPTION
 *
 *	Star is used to match filename expansions containing a trailing
 *	asterisk ('*').  Star call wildmat() to determine if the substring
 *	passed to it is matches the regular expression.
 *
 * PARAMETERS 
 *
 * 	char *source 	- The source string which is to be compared to the 
 *			  regular expression pattern. 
 * 	char *pattern 	- The regular expression which we are supposed to 
 *			  match to. 
 *
 * RETURNS 
 *
 * 	Returns non-zero if the entire source string is completely matched by 
 *	the regular expression pattern, returns 0 otherwise. This is used to 
 *	see if *'s in a pattern matched the entire source string. 
 *
 */

#ifdef __STDC__

static int star(char *source, char *pattern)

#else

static int star(source, pattern)
char           *source;		/* source operand */
char           *pattern;	/* regular expression to match */

#endif
{
    while (!wildmat(pattern, source)) {
	if (*++source == '\0') {
	    return (0);
	}
    }
    return (1);
}


/*
 * wildmat - match a regular expression 
 *
 * DESCRIPTION
 *
 *	Wildmat attempts to match the string pointed to by source to the 
 *	regular expression pointed to by pattern.  The subset of regular 
 *	expression syntax which is supported is defined by POSIX P1003.2 
 *	FILENAME EXPANSION rules.
 *
 * PARAMETERS 
 *
 * 	char *pattern 	- The regular expression which we are supposed to 
 *			  match to. 
 * 	char *source 	- The source string which is to be compared to the 
 *			  regular expression pattern. 
 *
 * RETURNS 
 *
 * 	Returns non-zero if the source string matches the regular expression 
 *	pattern specified, returns 0 otherwise. 
 *
 */

#ifdef __STDC__

int wildmat(char *pattern, char *source)

#else

int wildmat(pattern, source)
char           *pattern;	/* regular expression to match */
char           *source;		/* source operand */

#endif
{
    int             last;	/* last character matched */
    int             matched;	/* !0 if a match occurred */
    int             reverse;	/* !0 if sense of match is reversed */

    for (; *pattern; source++, pattern++) {
	switch (*pattern) {
	case '\\':
	    /* Literal match with following character */
	    pattern++;
	    /* FALLTHRU */
	default:
	    if (*source != *pattern) {
		return (0);
	    }
	    continue;
	case '?':
	    /* Match anything. */
	    if (*source == '\0') {
		return (0);
	    }
	    continue;
	case '*':
	    /* Trailing star matches everything. */
	    return (*++pattern ? star(source, pattern) : 1);
	case '[':
	    /* [^....] means inverse character class. */
	    if (reverse = pattern[1] == '^') {
		pattern++;
	    }
	    for (last = 0400, matched = 0;
		 *++pattern && *pattern != ']'; last = *pattern) {
		/* This next line requires a good C compiler. */
		if (*pattern == '-'
		    ? *source <= *++pattern && *source >= last
		    : *source == *pattern) {
		    matched = 1;
		}
	    }
	    if (matched == reverse) {
		return (0);
	    }
	    continue;
	}
    }

    /*
     * For "tar" use, matches that end at a slash also work. --hoptoad!gnu 
     */
    return (*source == '\0' || *source == '/');
}
