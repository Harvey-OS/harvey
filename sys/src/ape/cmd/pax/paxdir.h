/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
	<dirent.h> -- definitions for SVR3 directory access routines

	last edit:	25-Apr-1987	D A Gwyn

	Prerequisite:	<sys/types.h>
*/

#ifndef _PAX_DIRENT_H
#define _PAX_DIRENT_H

#include "config.h"
#ifdef USG
#define UFS
#else
#ifdef BSD
#define BFS
#endif
#endif

struct dirent { 			/* data from getdents()/readdir() */
    long		d_ino;		/* inode number of entry */
    off_t		d_off;		/* offset of disk directory entry */
    unsigned short	d_reclen;	/* length of this record */
    char		d_name[1];	/* name of file (non-POSIX) */
};

typedef struct {
    int		dd_fd;			/* file descriptor */
    int		dd_loc;			/* offset in block */
    int		dd_size;		/* amount of valid data */
    char	*dd_buf;		/* -> directory block */
} DIR;					/* stream data from opendir() */


/* 
 * The following nonportable ugliness could have been avoided by defining
 * DIRENTSIZ and DIRENTBASESIZ to also have (struct dirent *) arguments. 
 */
#define	DIRENTBASESIZ		(((struct dirent *)0)->d_name \
				- (char *)&((struct dirent *)0)->d_ino)
#define	DIRENTSIZ( namlen )	((DIRENTBASESIZ + sizeof(long) + (namlen)) \
				/ sizeof(long) * sizeof(long))

#define	MAXNAMLEN 512			/* maximum filename length */

#ifndef NAME_MAX
#define	NAME_MAX (MAXNAMLEN - 1)	/* DAG -- added for POSIX */
#endif

#define	DIRBUF	 8192			/* buffer size for fs-indep. dirs */
		/* must in general be larger than the filesystem buffer size */

extern DIR		*opendir();
extern struct dirent	*readdir();
extern OFFSET		telldir();
extern void		seekdir();
extern int		closedir();

#endif /* _PAX_DIRENT_H */
