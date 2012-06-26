/* $Source: /u/mark/src/pax/RCS/create.c,v $
 *
 * $Revision: 1.3 $
 *
 * create.c - Create a tape archive. 
 *
 * DESCRIPTION
 *
 *	These functions are used to create/write and archive from an set of
 *	named files.
 *
 * AUTHOR
 *
 *     	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
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
 * $Log:	create.c,v $
 * Revision 1.3  89/02/12  10:29:37  mark
 * Fixed misspelling of Replstr
 * 
 * Revision 1.2  89/02/12  10:04:17  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:06  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: create.c,v 1.3 89/02/12 10:29:37 mark Exp Locker: mark $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__

static void writetar(char *, Stat *);
static void writecpio(char *, Stat *);
static char tartype(int);

#else /* !__STDC__ */

static void writetar();
static void writecpio();
static char tartype();

#endif /* __STDC__ */


/* create_archive - create a tar archive.
 *
 * DESCRIPTION
 *
 *	Create_archive is used as an entry point to both create and append
 *	archives.  Create archive goes through the files specified by the
 *	user and writes each one to the archive if it can.  Create_archive
 *	knows how to write both cpio and tar headers and the padding which
 *	is needed for each type of archive.
 *
 * RETURNS
 *
 *	Always returns 0
 */

#ifdef __STDC__

int create_archive(void)

#else

int create_archive()

#endif
{
    char            name[PATH_MAX + 1];
    Stat            sb;
    int             fd;

    while (name_next(name, &sb) != -1) {
	if ((fd = openin(name, &sb)) < 0) {
	    /* FIXME: pax wants to exit here??? */
	    continue;
	}

	if (rplhead != (Replstr *)NULL) {
	    rpl_name(name);
	    if (strlen(name) == 0) {
		continue;
	    }
	}
	if (get_disposition("add", name) || get_newname(name, sizeof(name))) {
	    /* skip file... */
	    if (fd) {
		close(fd);
	    }
	    continue;
	} 

	if (!f_link && sb.sb_nlink > 1) {
	    if (islink(name, &sb)) {
		sb.sb_size = 0;
	    }
	    linkto(name, &sb);
	}
	if (ar_format == TAR) {
	    writetar(name, &sb);
	} else {
	    writecpio(name, &sb);
	}
	if (fd) {
	    outdata(fd, name, sb.sb_size);
	}
	if (f_verbose) {
	    print_entry(name, &sb);
	}
    }

    write_eot();
    close_archive();
    return (0);
}


/* writetar - write a header block for a tar file
 *
 * DESCRIPTION
 *
 * 	Make a header block for the file name whose stat info is in st.  
 *	Return header pointer for success, NULL if the name is too long.
 *
 * 	The tar header block is structured as follows:
 *
 *		FIELD NAME	OFFSET		SIZE
 *      	-------------|---------------|------
 *		name		  0		100
 *		mode		100		  8
 *		uid		108		  8
 *		gid		116		  8
 *		size		124		 12
 *		mtime		136		 12
 *		chksum		148		  8
 *		typeflag	156		  1
 *		linkname	157		100
 *		magic		257		  6
 *		version		263		  2
 *		uname		265		 32
 *		gname		297		 32
 *		devmajor	329		  8
 *		devminor	337		  8
 *		prefix		345		155
 *
 * PARAMETERS
 *
 *	char	*name	- name of file to create a header block for
 *	Stat	*asb	- pointer to the stat structure for the named file
 *
 */

#ifdef __STDC__

static void writetar(char *name, Stat *asb)

#else
    
static void writetar(name, asb)
char           *name;
Stat           *asb;

#endif
{
    char	   *p;
    char           *prefix = (char *)NULL;
    int             i;
    int             sum;
    char            hdr[BLOCKSIZE];
    Link           *from;

    memset(hdr, 0, BLOCKSIZE);
    if (strlen(name) > 255) {
	warn(name, "name too long");
	return;
    }

    /* 
     * If the pathname is longer than TNAMLEN, but less than 255, then
     * we can split it up into the prefix and the filename.
     */
    if (strlen(name) > 100) {
	prefix = name;
	name += 155;
	while (name > prefix && *name != '/') {
	    name--;
	}

	/* no slash found....hmmm.... */
	if (name == prefix) {
	    warn(prefix, "Name too long");
	    return;
	}
	*name++ = '\0';
    }

#ifdef S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	strcpy(&hdr[157], asb->sb_link);
	asb->sb_size = 0;
    }
#endif
    strcpy(hdr, name);
    sprintf(&hdr[100], "%06o \0", asb->sb_mode & ~S_IFMT);
    sprintf(&hdr[108], "%06o \0", asb->sb_uid);
    sprintf(&hdr[116], "%06o \0", asb->sb_gid);
    sprintf(&hdr[124], "%011lo ", (long) asb->sb_size);
    sprintf(&hdr[136], "%011lo ", (long) asb->sb_mtime);
    strncpy(&hdr[148], "        ", 8);
    hdr[156] = tartype(asb->sb_mode);
    if (asb->sb_nlink > 1 && (from = linkfrom(name, asb)) != (Link *)NULL) {
	strcpy(&hdr[157], from->l_name);
	hdr[156] = LNKTYPE;
    }
    strcpy(&hdr[257], TMAGIC);
    strncpy(&hdr[263], TVERSION, 2);
    strcpy(&hdr[265], finduname((int) asb->sb_uid));
    strcpy(&hdr[297], findgname((int) asb->sb_gid));
    sprintf(&hdr[329], "%06o \0", major(asb->sb_rdev));
    sprintf(&hdr[337], "%06o \0", minor(asb->sb_rdev));
    if (prefix != (char *)NULL) {
	strncpy(&hdr[345], prefix, 155);
    }

    /* Calculate the checksum */

    sum = 0;
    p = hdr;
    for (i = 0; i < 500; i++) {
	sum += 0xFF & *p++;
    }

    /* Fill in the checksum field. */

    sprintf(&hdr[148], "%06o \0", sum);

    outwrite(hdr, BLOCKSIZE);
}


/* tartype - return tar file type from file mode
 *
 * DESCRIPTION
 *
 *	tartype returns the character which represents the type of file
 *	indicated by "mode". 
 *
 * PARAMETERS
 *
 *	int	mode	- file mode from a stat block
 *
 * RETURNS
 *
 *	The character which represents the particular file type in the 
 *	ustar standard headers.
 */

#ifdef __STDC__

static char tartype(int mode)

#else
    
static char tartype(mode)
int	    mode;

#endif
{
    switch (mode & S_IFMT) {

#ifdef S_IFCTG
    case S_IFCTG:
	return(CONTTYPE);
#endif

    case S_IFDIR:
	return (DIRTYPE);

#ifdef S_IFLNK
    case S_IFLNK:
	return (SYMTYPE);
#endif

#ifdef S_IFFIFO
    case S_IFIFO:
	return (FIFOTYPE);
#endif

#ifdef S_IFCHR
    case S_IFCHR:
	return (CHRTYPE);
#endif

#ifdef S_IFBLK
    case S_IFBLK:
	return (BLKTYPE);
#endif

    default:
	return (REGTYPE);
    }
}


/* writecpio - write a cpio archive header
 *
 * DESCRIPTION
 *
 *	Writes a new CPIO style archive header for the file specified.
 *
 * PARAMETERS
 *
 *	char	*name	- name of file to create a header block for
 *	Stat	*asb	- pointer to the stat structure for the named file
 */

#ifdef __STDC__

static void writecpio(char *name, Stat *asb)

#else
    
static void writecpio(name, asb)
char           *name;
Stat           *asb;

#endif
{
    uint            namelen;
    char            header[M_STRLEN + H_STRLEN + 1];

    namelen = (uint) strlen(name) + 1;
    strcpy(header, M_ASCII);
    sprintf(header + M_STRLEN, "%06o%06o%06o%06o%06o",
	    USH(asb->sb_dev), USH(asb->sb_ino), USH(asb->sb_mode), 
	    USH(asb->sb_uid), USH(asb->sb_gid));
    sprintf(header + M_STRLEN + 30, "%06o%06o%011lo%06o%011lo",
#ifdef _POSIX_SOURCE
	    USH(asb->sb_nlink), USH(0),
#else
	    USH(asb->sb_nlink), USH(asb->sb_rdev),
#endif
	    f_mtime ? asb->sb_mtime : time((time_t *) 0),
	    namelen, asb->sb_size);
    outwrite(header, M_STRLEN + H_STRLEN);
    outwrite(name, namelen);
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	outwrite(asb->sb_link, (uint) asb->sb_size);
    }
#endif	/* S_IFLNK */
}
