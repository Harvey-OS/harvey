/* $Source: /u/mark/src/pax/RCS/port.h,v $
 *
 * $Revision: 1.2 $
 *
 * port.h - defnitions for portability library
 *
 * DESCRIPTION
 *
 *	Header for maintaing portablilty across operating system and
 *	version boundries.  For the most part, this file contains
 *	definitions which map functions which have the same functionality
 *	but different names on different systems, to have the same name.
 *
 * AUTHORS
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
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Mark H. Colburn and sponsored by The USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _PAX_PORT_H
#define _PAX_PORT_H

/*
 * Everybody does wait() differently.  There seem to be no definitions for
 * this in V7 (e.g. you are supposed to shift and mask things out using
 * constant shifts and masks.)  In order to provide the functionality, here
 * are some non standard but portable macros.  Don't change to a "union wait" 
 * based approach -- the ordering of the elements of the struct depends on the 
 * byte-sex of the machine. 
 */

#define	TERM_SIGNAL(status)	((status) & 0x7F)
#define TERM_COREDUMP(status)	(((status) & 0x80) != 0)
#define TERM_VALUE(status)	((status) >> 8)

/*
 * String library emulation definitions for the different variants of UNIX
 */

#if defined(USG) 

#   include <string.h>
#ifndef _POSIX_SOURCE
#   include <memory.h>
#endif

#else /* USG */

/*
 * The following functions are defined here since func.h has no idea which
 * of the functions will actually be used.
 */
#  ifdef __STDC__
extern char *rindex(char *, char);
extern char *index(char *, char);
extern char *bcopy(char *, char *, unsigned int);
extern char *bzero(char *, unsigned int);
extern char *strcat(char *, char *);
extern char *strcpy(char *, char *);
#  else /* !__STDC__ */
extern char *rindex();
extern char *index();
extern char *bcopy();
extern char *bzero();
extern char *strcat();
extern char *strcpy();
#  endif /* __STDC__ */

/*
 * Map ANSI C compatible functions to V7 functions
 */

#   define memcpy(a,b,n)	bcopy((b),(a),(n))
#   define memset(a,b,n)	bzero((a),(n))
#   define strrchr(s,c)		rindex(s,c)
#   define strchr(s,c)		index(s,c)

#endif /* USG */
#endif /* _PAX_PORT_H */
