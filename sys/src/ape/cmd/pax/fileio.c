/* $Source: /u/mark/src/pax/RCS/fileio.c,v $
 *
 * $Revision: 1.2 $
 *
 * fileio.c - file I/O functions for all archive interfaces
 *
 * DESCRIPTION
 *
 *	These function all do I/O of some form or another.  They are
 *	grouped here mainly for convienence.
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
 * $Log:	fileio.c,v $
 * Revision 1.2  89/02/12  10:04:31  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:09  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: fileio.c,v 1.2 89/02/12 10:04:31 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* open_archive -  open an archive file.  
 *
 * DESCRIPTION
 *
 *	Open_archive will open an archive file for reading or writing,
 *	setting the proper file mode, depending on the "mode" passed to
 *	it.  All buffer pointers are reset according to the mode
 *	specified.
 *
 * PARAMETERS
 *
 * 	int	mode 	- specifies whether we are reading or writing.   
 *
 * RETURNS
 *
 *	Returns a zero if successfull, or -1 if an error occured during 
 *	the open.
 */

#ifdef __STDC__
    
int open_archive(int mode)

#else
    
int open_archive(mode)
int             mode;

#endif
{
    if (ar_file[0] == '-' && ar_file[1] == '\0') {
	if (mode == AR_READ) {
	    archivefd = STDIN;
	    bufend = bufidx = bufstart;
	} else {
	    archivefd = STDOUT;
	}
    } else if (mode == AR_READ) {
	archivefd = open(ar_file, O_RDONLY | O_BINARY);
	bufend = bufidx = bufstart;	/* set up for initial read */
    } else if (mode == AR_WRITE) {
	archivefd = open(ar_file, O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0666);
    } else if (mode == AR_APPEND) {
	archivefd = open(ar_file, O_RDWR | O_BINARY, 0666);
	bufend = bufidx = bufstart;	/* set up for initial read */
    }

    if (archivefd < 0) {
	warnarch(strerror(), (OFFSET) 0);
	return (-1);
    }
    ++arvolume;
    return (0);
}


/* close_archive - close the archive file
 *
 * DESCRIPTION
 *
 *	Closes the current archive and resets the archive end of file
 *	marker.
 */

#ifdef __STDC__

void close_archive(void)

#else
    
void close_archive()

#endif
{
    if (archivefd != STDIN && archivefd != STDOUT) {
	close(archivefd);
    }
    areof = 0;
}


/* openout - open an output file
 *
 * DESCRIPTION
 *
 *	Openo opens the named file for output.  The file mode and type are
 *	set based on the values stored in the stat structure for the file.
 *	If the file is a special file, then no data will be written, the
 *	file/directory/Fifo, etc., will just be created.  Appropriate
 *	permission may be required to create special files.
 *
 * PARAMETERS
 *
 *	char 	*name		- The name of the file to create
 *	Stat	*asb		- Stat structure for the file
 *	Link	*linkp;		- pointer to link chain for this file
 *	int	 ispass		- true if we are operating in "pass" mode
 *
 * RETURNS
 *
 * 	Returns the output file descriptor, 0 if no data is required or -1 
 *	if unsuccessful. Note that UNIX open() will never return 0 because 
 *	the standard input is in use. 
 */

#ifdef __STDC__

int openout(char *name, Stat *asb, Link *linkp, int ispass)

#else
    
int openout(name, asb, linkp, ispass)
char           *name;
Stat           *asb;
Link           *linkp;
int             ispass;

#endif
{
    int             exists;
    int             fd;
    ushort          perm;
    ushort          operm = 0;
    Stat            osb;
#ifdef	S_IFLNK
    int             ssize;
    char            sname[PATH_MAX + 1];
#endif	/* S_IFLNK */

    if (exists = (LSTAT(name, &osb) == 0)) {
	if (ispass && osb.sb_ino == asb->sb_ino && osb.sb_dev == asb->sb_dev) {
	    warn(name, "Same file");
	    return (-1);
	} else if ((osb.sb_mode & S_IFMT) == (asb->sb_mode & S_IFMT)) {
	    operm = osb.sb_mode & S_IPERM;
	} else if (REMOVE(name, &osb) < 0) {
	    warn(name, strerror());
	    return (-1);
	} else {
	    exists = 0;
	}
    }
    if (linkp) {
	if (exists) {
	    if (asb->sb_ino == osb.sb_ino && asb->sb_dev == osb.sb_dev) {
		return (0);
	    } else if (unlink(name) < 0) {
		warn(name, strerror());
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (link(linkp->l_name, name) != 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) != 0 ||
			    link(linkp->l_name, name) != 0) {
			    warn(name, strerror());
			return (-1);
		    }
		} else {
		    warn(name, 
			     "Directories are not being created (-d option)");
		}
		return(0);
	    } else if (errno != EXDEV) {
		warn(name, strerror());
		return (-1);
	    }
	} else {
	    return(0);
	} 
    }
    perm = asb->sb_mode & S_IPERM;
    switch (asb->sb_mode & S_IFMT) {
    case S_IFBLK:
    case S_IFCHR:
#ifdef _POSIX_SOURCE
	warn(name, "Can't create special files");
	return (-1);
#else
	fd = 0;
	if (exists) {
	    if (asb->sb_rdev == osb.sb_rdev) {
		if (perm != operm && chmod(name, (int) perm) < 0) {
		    warn(name, strerror());
		    return (-1);
		} else {
		    break;
		}
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror());
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (mknod(name, (int) asb->sb_mode, (int) asb->sb_rdev) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || mknod(name, (int) asb->sb_mode, 
			   (int) asb->sb_rdev) < 0) {
			warn(name, strerror());
			return (-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror());
		return (-1);
	    }
	}
	return(0);
#endif /* _POSIX_SOURCE */
	break;
    case S_IFDIR:
	if (exists) {
	    if (perm != operm && chmod(name, (int) perm) < 0) {
		warn(name, strerror());
		return (-1);
	    }
	} else if (f_dir_create) {
	    if (dirmake(name, asb) < 0 || dirneed(name) < 0) {
		warn(name, strerror());
		return (-1);
	    }
	} else {
	    warn(name, "Directories are not being created (-d option)");
	}
	return (0);
#ifndef _POSIX_SOURCE
#ifdef	S_IFIFO
    case S_IFIFO:
	fd = 0;
	if (exists) {
	    if (perm != operm && chmod(name, (int) perm) < 0) {
		warn(name, strerror());
		return (-1);
	    }
	} else if (mknod(name, (int) asb->sb_mode, 0) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0
		       || mknod(name, (int) asb->sb_mode, 0) < 0) {
			warn(name, strerror());
			return (-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror());
		return (-1);
	    }
	}
	return(0);
	break;
#endif				/* S_IFIFO */
#endif				/* _POSIX_SOURCE */
#ifdef	S_IFLNK
    case S_IFLNK:
	if (exists) {
	    if ((ssize = readlink(name, sname, sizeof(sname))) < 0) {
		warn(name, strerror());
		return (-1);
	    } else if (strncmp(sname, asb->sb_link, ssize) == 0) {
		return (0);
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror());
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (symlink(asb->sb_link, name) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || symlink(asb->sb_link, name) < 0) {
			warn(name, strerror());
			return (-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror());
		return (-1);
	    }
	}
	return (0);		/* Can't chown()/chmod() a symbolic link */
#endif				/* S_IFLNK */
    case S_IFREG:
	if (exists) {
	    if (!f_unconditional && osb.sb_mtime > asb->sb_mtime) {
		warn(name, "Newer file exists");
		return (-1);
	    } else if (unlink(name) < 0) {
		warn(name, strerror());
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if ((fd = creat(name, (int) perm)) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || 
			    (fd = creat(name, (int) perm)) < 0) {
			warn(name, strerror());
			return (-1);
		    }
		} else {
		    /* 
		     * the file requires a directory which does not exist
		     * and which the user does not want created, so skip
		     * the file...
		     */
		    warn(name, "Directories are not being created (-d option)");
		    return(0);
		}
	    } else {
		warn(name, strerror());
		return (-1);
	    }
	}
	break;
    default:
	warn(name, "Unknown filetype");
	return (-1);
    }
    if (f_owner) {
	if (!exists || asb->sb_uid != osb.sb_uid || asb->sb_gid != osb.sb_gid) {
	    chown(name, (int) asb->sb_uid, (int) asb->sb_gid);
	}
    }
    return (fd);
}


/* openin - open the next input file
 *
 * DESCRIPTION
 *
 *	Openi will attempt to open the next file for input.  If the file is
 *	a special file, such as a directory, FIFO, link, character- or
 *	block-special file, then the file size field of the stat structure
 *	is zeroed to make sure that no data is written out for the file.
 *	If the file is a special file, then a file descriptor of 0 is
 *	returned to the caller, which is handled specially.  If the file
 *	is a regular file, then the file is opened and a file descriptor
 *	to the open file is returned to the caller.
 *
 * PARAMETERS
 *
 *	char   *name	- pointer to the name of the file to open
 *	Stat   *asb	- pointer to the stat block for the file to open
 *
 * RETURNS
 *
 * 	Returns a file descriptor, 0 if no data exists, or -1 at EOF. This 
 *	kludge works because standard input is in use, preventing open() from 
 *	returning zero. 
 */

#ifdef __STDC__

int openin(char *name, Stat *asb)

#else
    
int openin(name, asb)
char           *name;		/* name of file to open */
Stat           *asb;		/* pointer to stat structure for file */

#endif
{
    int             fd;

    switch (asb->sb_mode & S_IFMT) {
    case S_IFDIR:
	asb->sb_nlink = 1;
	asb->sb_size = 0;
	return (0);
#ifdef	S_IFLNK
    case S_IFLNK:
	if ((asb->sb_size = readlink(name,
			     asb->sb_link, sizeof(asb->sb_link) - 1)) < 0) {
	    warn(name, strerror());
	    return(0);
	}
	asb->sb_link[asb->sb_size] = '\0';
	return (0);
#endif				/* S_IFLNK */
    case S_IFREG:
	if (asb->sb_size == 0) {
	    return (0);
	}
	if ((fd = open(name, O_RDONLY | O_BINARY)) < 0) {
	    warn(name, strerror());
	}
	return (fd);
    default:
	asb->sb_size = 0;
	return (0);
    }
}
