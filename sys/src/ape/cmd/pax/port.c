/* $Source: /u/mark/src/pax/RCS/port.c,v $
 *
 * $Revision: 1.2 $
 *
 * port.c - These are routines not available in all environments. 
 *
 * DESCRIPTION
 *
 *	The routines contained in this file are provided for portability to
 *	other versions of UNIX or other operating systems (e.g. MSDOS).
 *	Not all systems have the same functions or the same semantics,
 *	these routines attempt to bridge the gap as much as possible.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *	John Gilmore (gnu@hoptoad)
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
 * $Log:	port.c,v $
 * Revision 1.2  89/02/12  10:05:35  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:29  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: port.c,v 1.2 89/02/12 10:05:35 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/*
 * Some computers are not so crass as to align themselves into the BSD or USG
 * camps.  If a system supplies all of the routines we fake here, add it to
 * the list in the #if !defined()'s below and it'll all be skipped. 
 */

#if !defined(mc300) && !defined(mc500) && !defined(mc700) && !defined(BSD) && !defined(_POSIX_SOURCE)

/* mkdir - make a directory
 *
 * DESCRIPTION
 *
 * 	Mkdir will make a directory of the name "dpath" with a mode of
 *	"dmode".  This is consistent with the BSD mkdir() function and the
 *	P1003.1 definitions of MKDIR.
 *
 * PARAMETERS
 *
 *	dpath		- name of directory to create
 *	dmode		- mode of the directory
 *
 * RETURNS
 *
 *	Returns 0 if the directory was successfully created, otherwise a
 *	non-zero return value will be passed back to the calling function
 *	and the value of errno should reflect the error.
 */

#ifdef __STDC__

int mkdir(char *dpath, int dmode)

#else
    
int mkdir(dpath, dmode)
char           *dpath;
int             dmode;

#endif
{
    int             cpid, status;
    Stat            statbuf;
    extern int      errno;

    if (STAT(dpath, &statbuf) == 0) {
	errno = EEXIST;		/* Stat worked, so it already exists */
	return (-1);
    }
    /* If stat fails for a reason other than non-existence, return error */
    if (errno != ENOENT)
	return (-1);

    switch (cpid = fork()) {

    case -1:			/* Error in fork() */
	return (-1);		/* Errno is set already */

    case 0:			/* Child process */

	status = umask(0);	/* Get current umask */
	status = umask(status | (0777 & ~dmode));	/* Set for mkdir */
	execl("/bin/mkdir", "mkdir", dpath, (char *) 0);
	_exit(-1);		/* Can't exec /bin/mkdir */

    default:			/* Parent process */
	while (cpid != wait(&status)) {
	    /* Wait for child to finish */
	}
    }

    if (TERM_SIGNAL(status) != 0 || TERM_VALUE(status) != 0) {
	errno = EIO;		/* We don't know why, but */
	return (-1);		/* /bin/mkdir failed */
    }
    return (0);
}


/* rmdir - remove a directory
 *
 * DESCRIPTION
 *
 *	Rmdir will remove the directory specified by "dpath".  It is
 *	consistent with the BSD and POSIX rmdir functions.
 *
 * PARAMETERS
 *
 *	dpath		- name of directory to remove
 *
 * RETURNS
 *
 *	Returns 0 if the directory was successfully deleted, otherwise a
 *	non-zero return value will be passed back to the calling function
 *	and the value of errno should reflect the error.
 */

#ifdef __STDC__

int rmdir(char *dpath)

#else
    
int rmdir(dpath)
char           *dpath;

#endif
{
    int             cpid, status;
    Stat            statbuf;
    extern int      errno;

    /* check to see if it exists */
    if (STAT(dpath, &statbuf) == -1) {
	return (-1);
    }
    switch (cpid = fork()) {

    case -1:			/* Error in fork() */
	return (-1);		/* Errno is set already */

    case 0:			/* Child process */
	execl("/bin/rmdir", "rmdir", dpath, (char *) 0);
	_exit(-1);		/* Can't exec /bin/rmdir */

    default:			/* Parent process */
	while (cpid != wait(&status)) {
	    /* Wait for child to finish */
	}
    }

    if (TERM_SIGNAL(status) != 0 || TERM_VALUE(status) != 0) {
	errno = EIO;		/* We don't know why, but */
	return (-1);		/* /bin/rmdir failed */
    }
    return (0);
}

#endif /* MASSCOMP, BSD */
