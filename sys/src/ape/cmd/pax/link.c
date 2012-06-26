/* $Source: /u/mark/src/pax/RCS/link.c,v $
 *
 * $Revision: 1.2 $
 *
 * link.c - functions for handling multiple file links 
 *
 * DESCRIPTION
 *
 *	These function manage the link chains which are used to keep track
 *	of outstanding links during archive reading and writing.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
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
 * $Log:	link.c,v $
 * Revision 1.2  89/02/12  10:04:38  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:12  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: link.c,v 1.2 89/02/12 10:04:38 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Defines */

/*
 * Address link information base. 
 */
#define	LINKHASH(ino)	(linkbase + (ino) % NEL(linkbase))

/*
 * Number of array elements. 
 */
#define	NEL(a)		(sizeof(a) / sizeof(*(a)))



/* Internal Identifiers */

static Link    *linkbase[256];	/* Unresolved link information */


/* linkfrom - find a file to link from 
 *
 * DESCRIPTION
 *
 *	Linkfrom searches the link chain to see if there is a file in the
 *	link chain which has the same inode number as the file specified
 *	by the stat block pointed at by asb.  If a file is found, the
 *	name is returned to the caller, otherwise a NULL is returned.
 *
 * PARAMETERS
 *
 *	char    *name   - name of the file which we are attempting
 *                        to find a link for
 *	Stat	*asb	- stat structure of file to find a link to
 *
 * RETURNS
 *
 * 	Returns a pointer to a link structure, or NULL if unsuccessful. 
 *
 */

#ifdef __STDC__

Link *linkfrom(char *name, Stat *asb)

#else
    
Link *linkfrom(name, asb)
char           *name;
Stat           *asb;

#endif
{
    Link           *linkp;
    Link           *linknext;
    Path           *path;
    Path           *pathnext;
    Link          **abase;

    for (linkp = *(abase = LINKHASH(asb->sb_ino)); linkp; linkp = linknext) {
	if (linkp->l_nlink == 0) {
	    if (linkp->l_name) {
		free((char *) linkp->l_name);
	    }
	    if (linknext = linkp->l_forw) {
		linknext->l_back = linkp->l_back;
	    }
	    if (linkp->l_back) {
		linkp->l_back->l_forw = linkp->l_forw;
	    }
	    free((char *) linkp);
	    *abase = (Link *)NULL;
	} else if (linkp->l_ino == asb->sb_ino && linkp->l_dev == asb->sb_dev) {
	    /* 
	     * check to see if a file with the name "name" exists in the 
	     * chain of files which we have for this particular link
	     */
	    for (path = linkp->l_path; path; path = pathnext) {
		if (strcmp(path->p_name, name) == 0) {
		    --linkp->l_nlink;
		    if (path->p_name) {
			free(path->p_name);
		    }
		    if (pathnext = path->p_forw) {
			pathnext->p_back = path->p_back;
		    }
		    if (path->p_back) {
			path->p_back->p_forw = pathnext;
		    }
		    if (linkp->l_path == path) {
			linkp->l_path = pathnext;
		    }
		    free(path);
		    return (linkp);
		}
		pathnext = path->p_forw;
	    }
	    return((Link *)NULL);
	} else {
	    linknext = linkp->l_forw;
	}
    }
    return ((Link *)NULL);
}



/* islink - determine whether a given file really a link
 *
 * DESCRIPTION
 *
 *	Islink searches the link chain to see if there is a file in the
 *	link chain which has the same inode number as the file specified
 *	by the stat block pointed at by asb.  If a file is found, a
 *	non-zero value is returned to the caller, otherwise a 0 is
 *	returned.
 *
 * PARAMETERS
 *
 *	char    *name   - name of file to check to see if it is link.
 *	Stat	*asb	- stat structure of file to find a link to
 *
 * RETURNS
 *
 * 	Returns a pointer to a link structure, or NULL if unsuccessful. 
 *
 */

#ifdef __STDC__

Link *islink(char *name, Stat *asb)

#else
    
Link *islink(name, asb)
char           *name;
Stat           *asb;

#endif
{
    Link           *linkp;
    Link           *linknext;

    for (linkp = *(LINKHASH(asb->sb_ino)); linkp; linkp = linknext) {
	if (linkp->l_ino == asb->sb_ino && linkp->l_dev == asb->sb_dev) {
	    if (strcmp(name, linkp->l_name) == 0) {
		return ((Link *)NULL);
	    } 
	    return (linkp);
	} else {
	    linknext = linkp->l_forw;
	}
    }
    return ((Link *)NULL);
}


/* linkto  - remember a file with outstanding links 
 *
 * DESCRIPTION
 *
 *	Linkto adds the specified file to the link chain.  Any subsequent
 *	calls to linkfrom which have the same inode will match the file
 *	just entered.  If not enough space is available to make the link
 *	then the item is not added to the link chain, and a NULL is
 *	returned to the calling function.
 *
 * PARAMETERS
 *
 *	char	*name	- name of file to remember
 *	Stat	*asb	- pointer to stat structure of file to remember
 *
 * RETURNS
 *
 * 	Returns a pointer to the associated link structure, or NULL when 
 *	linking is not possible. 
 *
 */

#ifdef __STDC__

Link *linkto(char *name, Stat *asb)

#else
    
Link *linkto(name, asb)
char           *name;
Stat           *asb;

#endif
{
    Link           *linkp;
    Link           *linknext;
    Path           *path;
    Link          **abase;

    for (linkp = *(LINKHASH(asb->sb_ino)); linkp; linkp = linknext) {
	if (linkp->l_ino == asb->sb_ino && linkp->l_dev == asb->sb_dev) {
	    if ((path = (Path *) mem_get(sizeof(Path))) == (Path *)NULL || 
		(path->p_name = mem_str(name)) == (char *)NULL) {
		return((Link *)NULL);
	    }
	    if (path->p_forw = linkp->l_path) {
		if (linkp->l_path->p_forw) {
		    linkp->l_path->p_forw->p_back = path;
		}
	    } else {
		linkp->l_path = path;
	    }
	    path->p_back = (Path *)NULL;
	    return(linkp);
	} else {
	    linknext = linkp->l_forw;
	}
    }
    /*
     * This is a brand new link, for which there is no other information
     */

    if ((asb->sb_mode & S_IFMT) == S_IFDIR
	|| (linkp = (Link *) mem_get(sizeof(Link))) == (Link *)NULL
	|| (linkp->l_name = mem_str(name)) == (char *)NULL) {
	return ((Link *)NULL);
    }
    linkp->l_dev = asb->sb_dev;
    linkp->l_ino = asb->sb_ino;
    linkp->l_nlink = asb->sb_nlink - 1;
    linkp->l_size = asb->sb_size;
    linkp->l_path = (Path *)NULL;
    if (linkp->l_forw = *(abase = LINKHASH(asb->sb_ino))) {
	linkp->l_forw->l_back = linkp;
    } else {
	*abase = linkp;
    }
    linkp->l_back = (Link *)NULL;
    return (linkp);
}


/* linkleft - complain about files with unseen links 
 *
 * DESCRIPTION
 *
 *	Linksleft scans through the link chain to see if there were any
 *	files which have outstanding links that were not processed by the
 *	archive.  For each file in the link chain for which there was not
 *	a file,  and error message is printed.
 */

#ifdef __STDC__

void linkleft(void)

#else
    
void linkleft()

#endif
{
    Link           *lp;
    Link          **base;

    for (base = linkbase; base < linkbase + NEL(linkbase); ++base) {
	for (lp = *base; lp; lp = lp->l_forw) {
	    if (lp->l_nlink) {
		warn(lp->l_path->p_name, "Unseen link(s)");
	    }
	}
    }
}
