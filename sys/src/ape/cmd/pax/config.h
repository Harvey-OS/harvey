/* $Source: /u/mark/src/pax/RCS/config.h,v $
 *
 * $Revision: 1.2 $
 *
 * config.h - configuration options for PAX
 *
 * DESCRIPTION
 *
 *	This file contains a number of configurable parameters for the
 *	PAX software.  This files should be edited prior to makeing the
 *	package.
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

#ifndef _PAX_CONFIG_H
#define _PAX_CONFIG_H

/* Defines */

/* XENIX_286 (SCO ugh, Xenix system V(?) 286, USG with changes...
 * You will get a warning about DIRSIZ being redefined, ignore it,
 * complain to SCO about include files that are messed up or send 
 * mail to doug@lentni.UUCP, who can provide some patches to fix 
 * your include files.
 *
 * Defining XENIX_286 will automatically define USG.
 *
 */
/* #define XENIX_286	/* Running on a XENIX 286 system */

/*
 * USG - USG (Unix System V) specific modifications
 *
 * Define USG if you are running Unix System V or some similar variant
 */
#define USG 	/* Running on a USG System */

/*
 * BSD - BSD (Berkely) specific modifications
 *
 * Define BSD if you are running some version of BSD Unix
 */
/* #define BSD 	/* Running on a BSD System */

/*
 * DEF_AR_FILE - tar only (required)
 *
 * DEF_AR_FILE should contain the full pathname of your favorite archive
 * device.  Normally this would be a tape drive, but it may be a disk drive
 * on those systems that don't have tape drives.
 */
#define DEF_AR_FILE	"-"	/* The default archive on your system */

/*
 * TTY - device which interactive queries should be directed to (required)
 *
 * This is the device to which interactive queries will be sent to and
 * received from.  On most unix systems, this should be /dev/tty, however, on
 * some systems, such as MS-DOS, it my need to be different (e.g. "con:").
 */
/* #define	TTY	"/dev/tty"	/* for most versions of UNIX */
/* #define	TTY	"con:"		/* For MS-DOS */
#define	TTY	"/dev/cons"		/* for Plan 9 */

/*
 * PAXDIR - if you do not have directory access routines
 *
 * Define PAXDIR if you do not have Doug Gwyn's dirent package installed
 * as a system library or you wish to use the version supplied with PAX.  
 *
 * NOTE: DO NOT DEFINE THIS IF YOU HAVE BERKELEY DIRECTORY ACCESS ROUTINES.
 */
/* #define PAXDIR		/* use paxdir.h paxdir.c */

/*
 * DIRENT - directory access routines (required)
 *
 * If you have Doug Gwyn's dirent package installed, either as a system
 * library, or are using the paxdir.c and paxdir.h routines which come with 
 * PAX, then define dirent. 
 *
 * NOTE: DO NOT DEFINE THIS IF YOU HAVE BERKELEY DIRECTORY ACCESS ROUTINES.
 */
#define DIRENT		/* use POSIX compatible directory routines */

/*
 * OFFSET - compiler dependent offset type
 * 
 * OFFSET is the type which is returned by lseek().  It is different on
 * some systems.  Most define it to be off_t, but some define it to be long.
 */
#define OFFSET	off_t	/* for most BSD, USG and other systems */
/* #define OFFSET	long	/* for most of the rest of them... */

/*
 * VOID - compiler support for VOID types
 *
 * If your system does not support void, then this should be defined to
 * int, otherwise, it should be left undefined.
 *
 * For ANSI Systems this should always be blank.
 */
#ifndef __STDC__
/* #define void	int	/* for system which do support void */
#endif

/*
 * SIG_T - return type for the signal routine
 *
 * Some systems have signal defines to return an int *, other return a
 * void *.  Please choose the correct value for your system.
 */
#define SIG_T	void	/* signal defined as "void (*signal)()" */
/* #define SIG_T	int	/* signal defined as "int (*signal)()" */

/*
 * STRCSPN - use the strcspn function included with pax
 *
 * Some systems do not have the strcspn() function in their C libraries.
 * For those system define STRCSPN and the one provided in regexp.c will 
 * be used.
 */
/* #define STRCSPN	/* implementation does not have strcspn() */

/*
 * STRERROR - use the strerror function included with pax
 *
 * Non-Ansi systems do not have the strerror() function in their C libraries.
 * For those system define STRERROR and the one provided in misc.c will 
 * be used instead.
 */
/* #define STRERROR	/* implementation does not have strerror() */

/*

/*
 * END OF CONFIGURATION SECTION
 *
 * Nothing beyond this point should need to be changed
 */

#ifdef BSD
#ifdef USG
#include "You must first edit config.h and Makefile to configure pax."
#endif
#endif
/*
 * Do a little sanity checking
 */
#ifdef PAXDIR
#  ifndef DIRENT
#    define DIRENT
#  endif
#endif

#ifdef XENIX_286
#  define USG
#endif /* XENIX_286 */

#endif /* _PAX_CONFIG_H */

#ifndef __STDC__
#define __STDC__
#endif
