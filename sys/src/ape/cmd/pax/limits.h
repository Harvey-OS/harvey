/* $Source: /u/mark/src/pax/RCS/limits.h,v $
 *
 * $Revision: 1.2 $
 *
 * 	limits.h - POSIX compatible defnitions for some of <limits.h>
 *
 * DESCRIPTION
 *
 * 	We need to include <limits.h> if this system is being compiled with an 
 * 	ANSI standard C compiler, or if we are running on a POSIX confomrming 
 * 	system.  If the manifest constant _POSIX_SOURCE is not defined when 
 * 	<limits.h> is included, then none of the POSIX constants are defined 
 *	and we need to define them here.  It's a bit wierd, but it works.
 *
 * 	These values where taken from the IEEE P1003.1 standard, draft 12.
 * 	All of the values below are the MINIMUM values allowed by the standard.
 * 	Not all values are used by the PAX program, but they are included for
 * 	completeness, and for support of future enhancements.  Please see
 * 	section 2.9 of the draft standard for more information on the following
 * 	constants.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
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

#ifndef _PAX_LIMITS_H
#define _PAX_LIMITS_H

/* Headers */

#if defined(__STDC__) || defined(_POSIX_SOURCE)
#   include <limits.h>
#endif


/* Defines */

#ifndef _POSIX_SOURCE

#define MAX_INPUT	256	/* Max numbef of bytes in terminal input */
#define NGROUPS_MAX	1	/* Max number of suplemental group id's */
#define PASS_MAX	8	/* Max number of bytes in a password */
#define PID_MAX		30000	/* Max value for a process ID */
#define UID_MAX		32000	/* Max value for a user or group ID */
#define ARG_MAX		4096	/* Nax number of bytes passed to exec */
#define CHILD_MAX	6	/* Max number of simultaneous processes */
#define MAX_CANON	256	/* Max numbef of bytes in a cononical queue */
#define OPEN_MAX	16	/* Nax number of open files per process */
#define NAME_MAX	14	/* Max number of bytes in a file name */
#define PATH_MAX	255	/* Max number of bytes in pathname */
#define LINK_MAX	8	/* Max value of a file's link count */
#define PIPE_BUF	512	/* Max number of bytes for pipe reads */

#endif /* _POSIX_SOURCE */
#endif /* _PAX_LIMITS_H */
