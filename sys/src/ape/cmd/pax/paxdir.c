/*
	opendir -- open a directory stream
  
	last edit:	16-Jun-1987	D A Gwyn
*/

#include	<sys/errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"paxdir.h"

#ifdef BSD_SYSV
/*
	<sys/_dir.h> -- definitions for 4.2,4.3BSD directories
  
	last edit:	25-Apr-1987	D A Gwyn
  
	A directory consists of some number of blocks of DIRBLKSIZ bytes each,
	where DIRBLKSIZ is chosen such that it can be transferred to disk in a
	single atomic operation (e.g., 512 bytes on most machines).
  
	Each DIRBLKSIZ-byte block contains some number of directory entry
	structures, which are of variable length.  Each directory entry has the
	beginning of a (struct direct) at the front of it, containing its
	filesystem-unique ident number, the length of the entry, and the length
	of the name contained in the entry.  These are followed by the NUL-
	terminated name padded to a (long) boundary with 0 bytes.  The maximum
	length of a name in a directory is MAXNAMELEN.
  
	The macro DIRSIZ(dp) gives the amount of space required to represent a
	directory entry.  Free space in a directory is represented by entries
	that have dp->d_reclen > DIRSIZ(dp).  All DIRBLKSIZ bytes in a
	directory block are claimed by the directory entries; this usually
	results in the last entry in a directory having a large dp->d_reclen.
	When entries are deleted from a directory, the space is returned to the
	previous entry in the same directory block by increasing its
	dp->d_reclen.  If the first entry of a directory block is free, then
	its dp->d_fileno is set to 0; entries other than the first in a
	directory do not normally have 	dp->d_fileno set to 0.
  
	prerequisite:	<sys/types.h>
*/

#if defined(accel) || defined(sun) || defined(vax)
#define	DIRBLKSIZ	512	/* size of directory block */
#else
#ifdef alliant
#define	DIRBLKSIZ	4096	/* size of directory block */
#else
#ifdef gould
#define	DIRBLKSIZ	1024	/* size of directory block */
#else
#ifdef ns32000			/* Dynix System V */
#define	DIRBLKSIZ	2600	/* size of directory block */
#else				/* be conservative; multiple blocks are okay
				 * but fractions are not */
#define	DIRBLKSIZ	4096	/* size of directory block */
#endif
#endif
#endif
#endif

#define	MAXNAMELEN	255	/* maximum filename length */
/* NOTE:  not MAXNAMLEN, which has been preempted by SVR3 <dirent.h> */

struct direct {			/* data from read()/_getdirentries() */
    unsigned long   d_fileno;	/* unique ident of entry */
    unsigned short  d_reclen;	/* length of this record */
    unsigned short  d_namlen;	/* length of string in d_name */
    char            d_name[MAXNAMELEN + 1];	/* NUL-terminated filename */
};

/*
	The DIRSIZ macro gives the minimum record length which will hold the
	directory entry.  This requires the amount of space in a (struct
	direct) without the d_name field, plus enough space for the name with a
	terminating NUL character, rounded up to a (long) boundary.
  
	(Note that Berkeley didn't properly compensate for struct padding,
	but we nevertheless have to use the same size as the actual system.)
*/

#define	DIRSIZ( dp )	((sizeof(struct direct) - (MAXNAMELEN+1) \
			+ sizeof(long) + (dp)->d_namlen) \
			/ sizeof(long) * sizeof(long))

#else
#include	<sys/dir.h>
#ifdef SYSV3
#undef	MAXNAMLEN		/* avoid conflict with SVR3 */
#endif
 /* Good thing we don't need to use the DIRSIZ() macro! */
#ifdef d_ino			/* 4.3BSD/NFS using d_fileno */
#undef	d_ino			/* (not absolutely necessary) */
#else
#define	d_fileno	d_ino	/* (struct direct) member */
#endif
#endif
#ifdef UNK
#ifndef UFS
#include "***** ERROR ***** UNK applies only to UFS"
/* One could do something similar for getdirentries(), but I didn't bother. */
#endif
#include	<signal.h>
#endif

#if defined(UFS) + defined(BFS) + defined(NFS) != 1	/* sanity check */
#include "***** ERROR ***** exactly one of UFS, BFS, or NFS must be defined"
#endif

#ifdef UFS
#define	RecLen( dp )	(sizeof(struct direct))	/* fixed-length entries */
#else				/* BFS || NFS */
#define	RecLen( dp )	((dp)->d_reclen)	/* variable-length entries */
#endif

#ifdef NFS
#ifdef BSD_SYSV
#define	getdirentries	_getdirentries	/* package hides this system call */
#endif
extern int      getdirentries();
static long     dummy;		/* getdirentries() needs basep */
#define	GetBlock( fd, buf, n )	getdirentries( fd, buf, (unsigned)n, &dummy )
#else				/* UFS || BFS */
#ifdef BSD_SYSV
#define read	_read		/* avoid emulation overhead */
#endif
extern int      read();
#define	GetBlock( fd, buf, n )	read( fd, buf, (unsigned)n )
#endif

#ifdef UNK
extern int      _getdents();	/* actual system call */
#endif

extern char    *strncpy();
extern int      fstat();
extern OFFSET   lseek();

extern int      errno;

#ifndef DIRBLKSIZ
#define	DIRBLKSIZ	4096	/* directory file read buffer size */
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef SEEK_CUR
#define	SEEK_CUR	1
#endif

#ifndef S_ISDIR			/* macro to test for directory file */
#define	S_ISDIR( mode )		(((mode) & S_IFMT) == S_IFDIR)
#endif


#ifndef SEEK_CUR
#define	SEEK_CUR	1
#endif

#ifdef BSD_SYSV
#define open	_open		/* avoid emulation overhead */
#endif

extern int      getdents();	/* SVR3 system call, or emulation */

typedef char   *pointer;	/* (void *) if you have it */

extern void     free();
extern pointer  malloc();
extern int
open(), close(), fstat();

extern int      errno;
extern OFFSET   lseek();

#ifndef SEEK_SET
#define	SEEK_SET	0
#endif

typedef int     bool;		/* Boolean data type */
#define	false	0
#define	true	1


#ifndef NULL
#define	NULL	0
#endif

#ifndef O_RDONLY
#define	O_RDONLY	0
#endif

#ifndef S_ISDIR			/* macro to test for directory file */
#define	S_ISDIR( mode )		(((mode) & S_IFMT) == S_IFDIR)
#endif

#ifdef __STDC__

DIR *opendir(char *dirname)

#else
    
DIR *opendir(dirname)
char           *dirname;	/* name of directory */

#endif
{
    register DIR   *dirp;	/* -> malloc'ed storage */
    register int    fd;		/* file descriptor for read */
    struct stat     sbuf;	/* result of fstat() */

    if ((fd = open(dirname, O_RDONLY)) < 0)
	return ((DIR *)NULL);		/* errno set by open() */

    if (fstat(fd, &sbuf) != 0 || !S_ISDIR(sbuf.st_mode)) {
	close(fd);
	errno = ENOTDIR;
	return ((DIR *)NULL);		/* not a directory */
    }
    if ((dirp = (DIR *) malloc(sizeof(DIR))) == (DIR *)NULL
	|| (dirp->dd_buf = (char *) malloc((unsigned) DIRBUF)) == (char *)NULL
	) {
	register int    serrno = errno;
	/* errno set to ENOMEM by sbrk() */

	if (dirp != (DIR *)NULL)
	    free((pointer) dirp);

	close(fd);
	errno = serrno;
	return ((DIR *)NULL);		/* not enough memory */
    }
    dirp->dd_fd = fd;
    dirp->dd_loc = dirp->dd_size = 0;	/* refill needed */

    return dirp;
}


/*
 *	closedir -- close a directory stream
 *
 *	last edit:	11-Nov-1988	D A Gwyn
 */

#ifdef __STDC__

int closedir(register DIR *dirp)

#else
    
int closedir(dirp)
register DIR	*dirp;		/* stream from opendir() */

#endif
{
    register int	fd;

    if ( dirp == (DIR *)NULL || dirp->dd_buf == (char *)NULL ) {
	errno = EFAULT;
	return -1;			/* invalid pointer */
    }

    fd = dirp->dd_fd;			/* bug fix thanks to R. Salz */
    free( (pointer)dirp->dd_buf );
    free( (pointer)dirp );
    return close( fd );
}


/*
	readdir -- read next entry from a directory stream
  
	last edit:	25-Apr-1987	D A Gwyn
*/

#ifdef __STDC__

struct dirent  *readdir(register DIR *dirp)

#else
    
struct dirent  *readdir(dirp)
register DIR   *dirp;		/* stream from opendir() */

#endif
{
    register struct dirent *dp;	/* -> directory data */

    if (dirp == (DIR *)NULL || dirp->dd_buf == (char *)NULL) {
	errno = EFAULT;
	return (struct dirent *)NULL;		/* invalid pointer */
    }
    do {
	if (dirp->dd_loc >= dirp->dd_size)	/* empty or obsolete */
	    dirp->dd_loc = dirp->dd_size = 0;

	if (dirp->dd_size == 0	/* need to refill buffer */
	    && (dirp->dd_size =
		getdents(dirp->dd_fd, dirp->dd_buf, (unsigned) DIRBUF)
		) <= 0
	    )
	    return ((struct dirent *)NULL);	/* EOF or error */

	dp = (struct dirent *) & dirp->dd_buf[dirp->dd_loc];
	dirp->dd_loc += dp->d_reclen;
    }
    while (dp->d_ino == 0L);	/* don't rely on getdents() */

    return dp;
}


/*
	seekdir -- reposition a directory stream
  
	last edit:	24-May-1987	D A Gwyn
  
	An unsuccessful seekdir() will in general alter the current
	directory position; beware.
  
	NOTE:	4.nBSD directory compaction makes seekdir() & telldir()
		practically impossible to do right.  Avoid using them!
*/

#ifdef __STDC__

void seekdir(register DIR *dirp, register OFFSET loc)

#else
    
void seekdir(dirp, loc)
register DIR   *dirp;		/* stream from opendir() */
register OFFSET  loc;		/* position from telldir() */

#endif
{
    register bool   rewind;	/* "start over when stymied" flag */

    if (dirp == (DIR *)NULL || dirp->dd_buf == (char *)NULL) {
	errno = EFAULT;
	return;			/* invalid pointer */
    }
    /*
     * A (struct dirent)'s d_off is an invented quantity on 4.nBSD
     * NFS-supporting systems, so it is not safe to lseek() to it. 
     */

    /* Monotonicity of d_off is heavily exploited in the following. */

    /*
     * This algorithm is tuned for modest directory sizes.  For huge
     * directories, it might be more efficient to read blocks until the first
     * d_off is too large, then back up one block, or even to use binary
     * search on the directory blocks.  I doubt that the extra code for that
     * would be worthwhile. 
     */

    if (dirp->dd_loc >= dirp->dd_size	/* invalid index */
	|| ((struct dirent *) & dirp->dd_buf[dirp->dd_loc])->d_off > loc
    /* too far along in buffer */
	)
	dirp->dd_loc = 0;	/* reset to beginning of buffer */
    /* else save time by starting at current dirp->dd_loc */

    for (rewind = true;;) {
	register struct dirent *dp;

	/* See whether the matching entry is in the current buffer. */

	if ((dirp->dd_loc < dirp->dd_size	/* valid index */
	     || readdir(dirp) != (struct dirent *)NULL	/* next buffer read */
	     && (dirp->dd_loc = 0, true)	/* beginning of buffer set */
	     )
	    && (dp = (struct dirent *) & dirp->dd_buf[dirp->dd_loc])->d_off
	    <= loc		/* match possible in this buffer */
	    ) {
	    for ( /* dp initialized above */ ;
		 (char *) dp < &dirp->dd_buf[dirp->dd_size];
		 dp = (struct dirent *) ((char *) dp + dp->d_reclen)
		)
		if (dp->d_off == loc) {	/* found it! */
		    dirp->dd_loc =
			(char *) dp - dirp->dd_buf;
		    return;
		}
	    rewind = false;	/* no point in backing up later */
	    dirp->dd_loc = dirp->dd_size;	/* set end of buffer */
	} else
	 /* whole buffer past matching entry */ if (!rewind) {	/* no point in searching
								 * further */
	    errno = EINVAL;
	    return;		/* no entry at specified loc */
	} else {		/* rewind directory and start over */
	    rewind = false;	/* but only once! */

	    dirp->dd_loc = dirp->dd_size = 0;

	    if (lseek(dirp->dd_fd, (OFFSET) 0, SEEK_SET)
		!= 0
		)
		return;		/* errno already set (EBADF) */

	    if (loc == 0)
		return;		/* save time */
	}
    }
}


/* telldir - report directory stream position
 *
 * DESCRIPTION
 *
 *	Returns the offset of the next directory entry in the
 *	directory associated with dirp.
 *
 *	NOTE:	4.nBSD directory compaction makes seekdir() & telldir()
 *		practically impossible to do right.  Avoid using them!
 *
 * PARAMETERS
 *
 *	DIR	*dirp	- stream from opendir()
 *
 * RETURNS
 *
 * 	Return offset of next entry 
 */


#ifdef __STDC__

OFFSET telldir(DIR *dirp)

#else
    
OFFSET telldir(dirp)			
DIR            *dirp;		/* stream from opendir() */

#endif
{
    if (dirp == (DIR *)NULL || dirp->dd_buf == (char *)NULL) {
	errno = EFAULT;
	return -1;		/* invalid pointer */
    }
    if (dirp->dd_loc < dirp->dd_size)	/* valid index */
	return ((struct dirent *) & dirp->dd_buf[dirp->dd_loc])->d_off;
    else			/* beginning of next directory block */
	return lseek(dirp->dd_fd, (OFFSET) 0, SEEK_CUR);
}


#ifdef UFS

/*
	The following routine is necessary to handle DIRSIZ-long entry names.
	Thanks to Richard Todd for pointing this out.
*/


/* return # chars in embedded name */

#ifdef __STDC__

static int NameLen(char *name)

#else
    
static int NameLen(name)
char            *name;		/* -> name embedded in struct direct */

#endif
{
    register char  *s;		/* -> name[.] */
    register char  *stop = &name[DIRSIZ];	/* -> past end of name field */

    for (s = &name[1];		/* (empty names are impossible) */
	 *s != '\0'		/* not NUL terminator */
	 && ++s < stop;		/* < DIRSIZ characters scanned */
	);

    return s - name;		/* # valid characters in name */
}

#else				/* BFS || NFS */

extern int      strlen();

#define	NameLen( name )	strlen( name )	/* names are always NUL-terminated */

#endif

#ifdef UNK
static enum {
    maybe, no, yes
} state = maybe;


/* sig_catch - used to catch signals
 *
 * DESCRIPTION
 *
 *	Used to catch signals.
 */

/*ARGSUSED*/

#ifdef __STDC__

static void sig_catch(int sig)

#else
    
static void sig_catch(sig)
int             sig;		/* must be SIGSYS */

#endif
{
    state = no;			/* attempted _getdents() faulted */
}
#endif


/* getdents - get directory entries
 *
 * DESCRIPTION
 *
 *	Gets directory entries from the filesystem in an implemenation
 *	defined way.
 *
 * PARAMETERS
 *
 *	int             fildes	- directory file descriptor 
 *	char           *buf	- where to put the (struct dirent)s 
 *	unsigned	nbyte	- size of buf[] 
 *
 * RETURNS
 * 
 *	Returns number of bytes read; 0 on EOF, -1 on error 
 */

#ifdef __STDC__

int getdents(int fildes, char *buf, unsigned nbyte)

#else
    
int getdents(fildes, buf, nbyte)	
int             fildes;		/* directory file descriptor */
char           *buf;		/* where to put the (struct dirent)s */
unsigned        nbyte;		/* size of buf[] */

#endif
{
    int             serrno;	/* entry errno */
    OFFSET          offset;	/* initial directory file offset */
    struct stat     statb;	/* fstat() info */
    union {
	/* directory file block buffer */
#ifdef UFS
	char		dblk[DIRBLKSIZ + 1];
#else
	char            dblk[DIRBLKSIZ];
#endif
	struct direct   dummy;	/* just for alignment */
    } u;		/* (avoids having to malloc()) */
    register struct direct *dp;	/* -> u.dblk[.] */
    register struct dirent *bp;	/* -> buf[.] */

#ifdef UNK
    switch (state) {
	SIG_T         (*shdlr)();	/* entry SIGSYS handler */
	register int    retval;		/* return from _getdents() if any */

    case yes:			/* _getdents() is known to work */
	return _getdents(fildes, buf, nbyte);

    case maybe:		/* first time only */
	shdlr = signal(SIGSYS, sig_catch);
	retval = _getdents(fildes, buf, nbyte);	/* try it */
	signal(SIGSYS, shdlr);

	if (state == maybe) {	/* SIGSYS did not occur */
	    state = yes;	/* so _getdents() must have worked */
	    return retval;
	}
	/* else fall through into emulation */

/*	case no:	/* fall through into emulation */
    }
#endif

    if (buf == (char *)NULL
#ifdef ATT_SPEC
	|| (unsigned long) buf % sizeof(long) != 0	/* ugh */
#endif
	) {
	errno = EFAULT;		/* invalid pointer */
	return -1;
    }
    if (fstat(fildes, &statb) != 0) {
	return -1;		/* errno set by fstat() */
    }

    if (!S_ISDIR(statb.st_mode)) {
	errno = ENOTDIR;	/* not a directory */
	return -1;
    }
    if ((offset = lseek(fildes, (OFFSET) 0, SEEK_CUR)) < 0) {
	return -1;		/* errno set by lseek() */
    }

#ifdef BFS			/* no telling what remote hosts do */
    if ((unsigned long) offset % DIRBLKSIZ != 0) {
	errno = ENOENT;		/* file pointer probably misaligned */
	return -1;
    }
#endif

    serrno = errno;		/* save entry errno */

    for (bp = (struct dirent *) buf; bp == (struct dirent *) buf;) {	

    	/* convert next directory block */
	int             size;

	do {
	    size = GetBlock(fildes, u.dblk, DIRBLKSIZ);
	} while (size == -1 && errno == EINTR);

	if (size <= 0) {
	    return size;	/* EOF or error (EBADF) */
	}

	for (dp = (struct direct *) u.dblk;
	     (char *) dp < &u.dblk[size];
	     dp = (struct direct *) ((char *) dp + RecLen(dp))
	    ) {
#ifndef UFS
	    if (dp->d_reclen <= 0) {
		errno = EIO;	/* corrupted directory */
		return -1;
	    }
#endif

	    if (dp->d_fileno != 0) {	/* non-empty; copy to user buffer */
		register int    reclen =
		DIRENTSIZ(NameLen(dp->d_name));

		if ((char *) bp + reclen > &buf[nbyte]) {
		    errno = EINVAL;
		    return -1;	/* buf too small */
		}
		bp->d_ino = dp->d_fileno;
		bp->d_off = offset + ((char *) dp - u.dblk);
		bp->d_reclen = reclen;

		{
#ifdef UFS
		    /* Is the following kludge ugly?  You bet. */

		    register char   save = dp->d_name[DIRSIZ];
		    /* save original data */

		    dp->d_name[DIRSIZ] = '\0';
		    /* ensure NUL termination */
#endif
		    /* adds NUL padding */
		    strncpy(bp->d_name, dp->d_name, reclen - DIRENTBASESIZ);
#ifdef UFS
		    dp->d_name[DIRSIZ] = save;
		    /* restore original data */
#endif
		}

		bp = (struct dirent *) ((char *) bp + reclen);
	    }
	}

#ifndef BFS			/* 4.2BSD screwed up; fixed in 4.3BSD */
	if ((char *) dp > &u.dblk[size]) {
	    errno = EIO;	/* corrupted directory */
	    return -1;
	}
#endif
    }

    errno = serrno;		/* restore entry errno */
    return (char *) bp - buf;	/* return # bytes read */
}
