/* $Source: /u/mark/src/pax/RCS/names.c,v $
 *
 * $Revision: 1.2 $
 *
 * names.c - Look up user and/or group names. 
 *
 * DESCRIPTION
 *
 *	These functions support UID and GID name lookup.  The results are
 *	cached to improve performance.
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
 * $Log:	names.c,v $
 * Revision 1.2  89/02/12  10:05:05  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:19  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: names.c,v 1.2 89/02/12 10:05:05 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Defines */

#define myuid	( my_uid < 0? (my_uid = getuid()): my_uid )
#define	mygid	( my_gid < 0? (my_gid = getgid()): my_gid )


/* Internal Identifiers */

static int      saveuid = -993;
static char     saveuname[TUNMLEN];
static int      my_uid = -993;

static int      savegid = -993;
static char     savegname[TGNMLEN];
static int      my_gid = -993;


/* finduname - find a user or group name from a uid or gid
 *
 * DESCRIPTION
 *
 * 	Look up a user name from a uid/gid, maintaining a cache. 
 *
 * PARAMETERS
 *
 *	char	uname[]		- name (to be returned to user)
 *	int	uuid		- id of name to find
 *
 *
 * RETURNS
 *
 *	Returns a name which is associated with the user id given.  If there
 *	is not name which corresponds to the user-id given, then a pointer
 *	to a string of zero length is returned.
 *	
 * FIXME
 *
 * 	1. for now it's a one-entry cache. 
 *	2. The "-993" is to reduce the chance of a hit on the first lookup. 
 */

#ifdef __STDC__

char *finduname(int uuid)

#else
    
char *finduname(uuid)
int             uuid;

#endif
{
    struct passwd  *pw;

    if (uuid != saveuid) {
	saveuid = uuid;
	saveuname[0] = '\0';
	pw = getpwuid(uuid);
	if (pw) {
	    strncpy(saveuname, pw->pw_name, TUNMLEN);
	}
    }
    return(saveuname);
}


/* finduid - get the uid for a given user name
 *
 * DESCRIPTION
 *
 *	This does just the opposit of finduname.  Given a user name it
 *	finds the corresponding UID for that name.
 *
 * PARAMETERS
 *
 *	char	uname[]		- username to find a UID for
 *
 * RETURNS
 *
 *	The UID which corresponds to the uname given, if any.  If no UID
 *	could be found, then the UID which corrsponds the user running the
 *	program is returned.
 *
 */

#ifdef __STDC__

int finduid(char *uname)

#else
    
int finduid(uname)
char            *uname;

#endif
{
    struct passwd  *pw;
    extern struct passwd *getpwnam();

    if (uname[0] != saveuname[0]/* Quick test w/o proc call */
	||0 != strncmp(uname, saveuname, TUNMLEN)) {
	strncpy(saveuname, uname, TUNMLEN);
	pw = getpwnam(uname);
	if (pw) {
	    saveuid = pw->pw_uid;
	} else {
	    saveuid = myuid;
	}
    }
    return (saveuid);
}


/* findgname - look up a group name from a gid
 *
 * DESCRIPTION
 *
 * 	Look up a group name from a gid, maintaining a cache.
 *	
 *
 * PARAMETERS
 *
 *	int	ggid		- goupid of group to find
 *
 * RETURNS
 *
 *	A string which is associated with the group ID given.  If no name
 *	can be found, a string of zero length is returned.
 */

#ifdef __STDC__

char *findgname(int ggid)

#else
    
char *findgname(ggid)
int             ggid;

#endif
{
    struct group   *gr;

    if (ggid != savegid) {
	savegid = ggid;
	savegname[0] = '\0';
#ifndef _POSIX_SOURCE
	setgrent();
#endif
	gr = getgrgid(ggid);
	if (gr) {
	    strncpy(savegname, gr->gr_name, TGNMLEN);
	}
    }
    return(savegname);
}



/* findgid - get the gid for a given group name
 *
 * DESCRIPTION
 *
 *	This does just the opposit of finduname.  Given a group name it
 *	finds the corresponding GID for that name.
 *
 * PARAMETERS
 *
 *	char	uname[]		- groupname to find a GID for
 *
 * RETURNS
 *
 *	The GID which corresponds to the uname given, if any.  If no GID
 *	could be found, then the GID which corrsponds the group running the
 *	program is returned.
 *
 */

#ifdef __STDC__

int findgid(char *gname)

#else
    
int findgid(gname)
char           *gname;

#endif
{
    struct group   *gr;

    /* Quick test w/o proc call */
    if (gname[0] != savegname[0] || strncmp(gname, savegname, TUNMLEN) != 0) {
	strncpy(savegname, gname, TUNMLEN);
	gr = getgrnam(gname);
	if (gr) {
	    savegid = gr->gr_gid;
	} else {
	    savegid = mygid;
	}
    }
    return (savegid);
}
