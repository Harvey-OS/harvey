/* $Source: /u/mark/src/pax/RCS/replace.c,v $
 *
 * $Revision: 1.2 $
 *
 * replace.c - regular expression pattern replacement functions
 *
 * DESCRIPTION
 *
 *	These routines provide for regular expression file name replacement
 *	as required by pax.
 *
 * AUTHORS
 *
 *	Mark H. Colburn, NAPS International
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
 * $Log:	replace.c,v $
 * Revision 1.2  89/02/12  10:05:59  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:36  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: replace.c,v 1.2 89/02/12 10:05:59 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* not lint */

/* Headers */

#include "pax.h"


/* add_replstr - add a replacement string to the replacement string list
 *
 * DESCRIPTION
 *
 *	Add_replstr adds a replacement string to the replacement string
 *	list which is applied each time a file is about to be processed.
 *
 * PARAMETERS
 *
 *	char	*pattern	- A regular expression which is to be parsed
 */

#ifdef __STDC__

void add_replstr(char *pattern)

#else

void add_replstr(pattern)
char           *pattern;

#endif
{
    char           *p;
    char            sep;
    Replstr        *rptr;
    int             len;

    if ((len = strlen(pattern)) < 4) {
	warn("Replacement string not added",
		 "Malformed substitution syntax");
	return;
    }
    if ((rptr = (Replstr *) malloc(sizeof(Replstr))) == (Replstr *)NULL) {
	warn("Replacement string not added", "No space");
	return;
    }

    /* First character is the delimeter... */
    sep = *pattern;

    /* Get trailing g and/or p */
    p = pattern + len - 1;
    while (*p != sep) {
	if (*p == 'g') {
            rptr->global = 1;
	} else if (*p == 'p') {
	    rptr->print = 1;
	} else {
	    warn(p, "Invalid RE modifier");
	}
	p--;
    }

    if (*p != sep) {
	warn("Replacement string not added", "Bad delimeters");
	free(rptr);
	return;
    }
    /* strip off leading and trailing delimeter */
    *p = '\0';
    pattern++;

    /* find the separating '/' in the pattern */
    p = pattern;
    while (*p) {
	if (*p == sep) {
	    break;
	}
	if (*p == '\\' && *(p + 1) != '\0') {
	    p++;
	}
	p++;
    }
    if (*p != sep) {
	warn("Replacement string not added", "Bad delimeters");
	free(rptr);
	return;
    }
    *p++ = '\0';

    /*
     * Now pattern points to 'old' and p points to 'new' and both are '\0'
     * terminated 
     */
    if ((rptr->comp = regcomp(pattern)) == (regexp *)NULL) {
	warn("Replacement string not added", "Invalid RE");
	free(rptr);
	return;
    }
    rptr->replace = p;
    rptr->next = (Replstr *)NULL;
    if (rplhead == (Replstr *)NULL) {
	rplhead = rptr;
	rpltail = rptr;
    } else {
	rpltail->next = rptr;
	rpltail = rptr;
    }
}



/* rpl_name - possibly replace a name with a regular expression
 *
 * DESCRIPTION
 *
 *	The string name is searched for in the list of regular expression
 *	substituions.  If the string matches any of the regular expressions
 *	then the string is modified as specified by the user.
 *
 * PARAMETERS
 *
 *	char	*name	- name to search for and possibly modify
 */

#ifdef __STDC__

void rpl_name(char *name)

#else

void rpl_name(name)
char           *name;

#endif
{
    int             found = 0;
    int             ret;
    Replstr        *rptr;
    char            buff[PATH_MAX + 1];
    char            buff1[PATH_MAX + 1];
    char            buff2[PATH_MAX + 1];
    char           *p;
    char           *b;

    strcpy(buff, name);
    for (rptr = rplhead; !found && rptr != (Replstr *)NULL; rptr = rptr->next) {
	do {
	    if ((ret = regexec(rptr->comp, buff)) != 0) {
		p = buff;
		b = buff1;
		while (p < rptr->comp->startp[0]) {
		    *b++ = *p++;
		}
		p = rptr->replace;
		while (*p) {
		    *b++ = *p++;
		}
		strcpy(b, rptr->comp->endp[0]);
		found = 1;
		regsub(rptr->comp, buff1, buff2);
		strcpy(buff, buff2);
	    }
	} while (ret && rptr->global);
	if (found) {
	    if (rptr->print) {
		fprintf(stderr, "%s >> %s\n", name, buff);
	    }
	    strcpy(name, buff);
	}
    }
}


/* get_disposition - get a file disposition
 *
 * DESCRIPTION
 *
 *	Get a file disposition from the user.  If the user enters 'y'
 *	the the file is processed, anything else and the file is ignored.
 *	If the user enters EOF, then the PAX exits with a non-zero return
 *	status.
 *
 * PARAMETERS
 *
 *	char	*mode	- string signifying the action to be taken on file
 *	char	*name	- the name of the file
 *
 * RETURNS
 *
 *	Returns 1 if the file should be processed, 0 if it should not.
 */

#ifdef __STDC__

int get_disposition(char *mode, char *name)

#else

int get_disposition(mode, name)
char	*mode;
char	*name;

#endif
{
    char	ans[2];
    char	buf[PATH_MAX + 10];

    if (f_disposition) {
	sprintf(buf, "%s %s? ", mode, name);
	if (nextask(buf, ans, sizeof(ans)) == -1 || ans[0] == 'q') {
	    exit(0);
	}
	if (strlen(ans) == 0 || ans[0] != 'y') {
	    return(1);
	} 
    } 
    return(0);
}


/* get_newname - prompt the user for a new filename
 *
 * DESCRIPTION
 *
 *	The user is prompted with the name of the file which is currently
 *	being processed.  The user may choose to rename the file by
 *	entering the new file name after the prompt; the user may press
 *	carriage-return/newline, which will skip the file or the user may
 *	type an 'EOF' character, which will cause the program to stop.
 *
 * PARAMETERS
 *
 *	char	*name		- filename, possibly modified by user
 *	int	size		- size of allowable new filename
 *
 * RETURNS
 *
 *	Returns 0 if successfull, or -1 if an error occurred.
 *
 */

#ifdef __STDC__

int get_newname(char *name, int size)

#else

int get_newname(name, size)
char	*name;
int	size;

#endif
{
    char	buf[PATH_MAX + 10];

    if (f_interactive) {
	sprintf(buf, "rename %s? ", name);
	if (nextask(buf, name, size) == -1) {
	    exit(0);
	}
	if (strlen(name) == 0) {
	    return(1);
	}
    }
    return(0);
}
