/* $Source: /u/mark/src/pax/RCS/tar.c,v $
 *
 * $Revision: 1.2 $
 *
 * tar.c - tar specific functions for archive handling
 *
 * DESCRIPTION
 *
 *	These routines provide a tar conforming interface to the pax
 *	program.
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
 * software was developed by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	tar.c,v $
 * Revision 1.2  89/02/12  10:06:05  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:38  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: tar.c,v 1.2 89/02/12 10:06:05 mark Exp $";
static char *copyright ="Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.";
#endif /* not lint */

/* Headers */

#include "pax.h"


/* Defines */

#define DEF_BLOCKING	20	/* default blocking factor for extract */


/* Function Prototypes */

#ifdef __STDC__

static int taropt(int , char **, char *);
static void usage(void);

#else /* !__STDC__ */

static int taropt();
static void usage();

#endif /* __STDC__ */


/* do_tar - main routine for tar. 
 *
 * DESCRIPTION
 *
 *	Provides a tar interface to the PAX program.  All tar standard
 *	command line options are supported.
 *
 * PARAMETERS
 *
 *	int argc	- argument count (argc from main) 
 *	char **argv	- argument list (argv from main) 
 *
 * RETURNS
 *
 *	zero
 */

#ifdef __STDC__

int do_tar(int argc, char **argv)

#else

int do_tar(argc, argv)
int             argc;		/* argument count (argc from main) */
char          **argv;		/* argument list (argv from main) */

#endif
{
    int             c;		/* Option letter */

    /* Set default option values */
    names_from_stdin = 0;
    ar_file = getenv("TAPE");	/* From environment, or */
    if (ar_file == 0) {
	ar_file = DEF_AR_FILE;	/* From Makefile */
    }

    /*
     * set up the flags to reflect the default pax inteface.  Unfortunately
     * the pax interface has several options which are completely opposite
     * of the tar and/or cpio interfaces...
     */
    f_unconditional = 1;
    f_mtime = 1;
    f_dir_create = 1;
    blocking = 0;
    ar_interface = TAR;
    ar_format = TAR;
    msgfile=stderr;

    /* Parse options */
    while ((c = taropt(argc, argv, "b:cf:hlmortuvwx")) != EOF) {
	switch (c) {
	case 'b':		/* specify blocking factor */
	    /* 
	     * FIXME - we should use a conversion routine that does
	     * some kind of reasonable error checking, but...
	     */
	    blocking = atoi(optarg);
	    break;
	case 'c':		/* create a new archive */
	    f_create = 1;
	    break;
	case 'f':		/* specify input/output file */
	    ar_file = optarg;
	    break;
	case 'h':
	    f_follow_links = 1;	/* follow symbolic links */
	    break;
	case 'l':		/* report unresolved links */
	    f_linksleft = 1;
	    break;
	case 'm':		/* don't restore modification times */
	    f_modified = 1;
	    break;
	case 'o':		/* take on user's group rather than
				 * archives */
	    break;
	case 'r':		/* named files are appended to archive */
	    f_append = 1;
	    break;
	case 't':
	    f_list = 1;		/* list files in archive */
	    break;
	case 'u':		/* named files are added to archive */
	    f_newer = 1;
	    break;
	case 'v':		/* verbose mode */
	    f_verbose = 1;
	    break;
	case 'w':		/* user interactive mode */
	    f_disposition = 1;
	    break;
	case 'x':		/* named files are extracted from archive */
	    f_extract = 1;
	    break;
	case '?':
	    usage();
	    exit(EX_ARGSBAD);
	}
    }

    /* check command line argument sanity */
    if (f_create + f_extract + f_list + f_append + f_newer != 1) {
	(void) fprintf(stderr,
	   "%s: you must specify exactly one of the c, t, r, u or x options\n",
		       myname);
	usage();
	exit(EX_ARGSBAD);
    }

    /* set the blocking factor, if not set by the user */
    if (blocking == 0) {
#ifdef USG
	if (f_extract || f_list) {
	    blocking = DEF_BLOCKING;
	    fprintf(stderr, "Tar: blocksize = %d\n", blocking);
	} else {
	    blocking = 1;
	}
#else /* !USG */
	blocking = 20;
#endif /* USG */
    }
    blocksize = blocking * BLOCKSIZE;
    buf_allocate((OFFSET) blocksize);

    if (f_create) {
	open_archive(AR_WRITE);
	create_archive();	/* create the archive */
    } else if (f_extract) {
	open_archive(AR_READ);
	read_archive();		/* extract files from archive */
    } else if (f_list) {
	open_archive(AR_READ);
	read_archive();		/* read and list contents of archive */
    } else if (f_append) {
	open_archive(AR_APPEND);
	append_archive();	/* append files to archive */
    }
    
    if (f_linksleft) {		
	linkleft(); 		/* report any unresolved links */ 
    }
    
    return (0);
}


/* taropt -  tar specific getopt
 *
 * DESCRIPTION
 *
 * 	Plug-compatible replacement for getopt() for parsing tar-like
 * 	arguments.  If the first argument begins with "-", it uses getopt;
 * 	otherwise, it uses the old rules used by tar, dump, and ps.
 *
 * PARAMETERS
 *
 *	int argc	- argument count (argc from main) 
 *	char **argv	- argument list (argv from main) 
 *	char *optstring	- sring which describes allowable options
 *
 * RETURNS
 *
 *	Returns the next option character in the option string(s).  If the
 *	option requires an argument and an argument was given, the argument
 *	is pointed to by "optarg".  If no option character was found,
 *	returns an EOF.
 *
 */

#ifdef __STDC__

static int taropt(int argc, char **argv, char *optstring)

#else

static int taropt(argc, argv, optstring)
int             argc;
char          **argv;
char           *optstring;

#endif
{
    extern char    *optarg;	/* Points to next arg */
    extern int      optind;	/* Global argv index */
    static char    *key;	/* Points to next keyletter */
    static char     use_getopt;	/* !=0 if argv[1][0] was '-' */
    char            c;
    char           *place;

    optarg = (char *)NULL;

    if (key == (char *)NULL) {		/* First time */
	if (argc < 2)
	    return EOF;
	key = argv[1];
	if (*key == '-')
	    use_getopt++;
	else
	    optind = 2;
    }
    if (use_getopt) {
	return getopt(argc, argv, optstring);
    }

    c = *key++;
    if (c == '\0') {
	key--;
	return EOF;
    }
    place = strchr(optstring, c);

    if (place == (char *)NULL || c == ':') {
	fprintf(stderr, "%s: unknown option %c\n", argv[0], c);
	return ('?');
    }
    place++;
    if (*place == ':') {
	if (optind < argc) {
	    optarg = argv[optind];
	    optind++;
	} else {
	    fprintf(stderr, "%s: %c argument missing\n",
		    argv[0], c);
	    return ('?');
	}
    }
    return (c);
}


/* usage - print a helpful message and exit
 *
 * DESCRIPTION
 *
 *	Usage prints out the usage message for the TAR interface and then
 *	exits with a non-zero termination status.  This is used when a user
 *	has provided non-existant or incompatible command line arguments.
 *
 * RETURNS
 *
 *	Returns an exit status of 1 to the parent process.
 *
 */

#ifdef __STDC__

static void usage(void)

#else

static void usage()

#endif
{
    fprintf(stderr, "Usage: %s -c[bfvw] device block filename..\n", myname);
    fprintf(stderr, "       %s -r[bvw] device block [filename...]\n", myname);
    fprintf(stderr, "       %s -t[vf] device\n", myname);
    fprintf(stderr, "       %s -u[bvw] device block [filename...]\n", myname);
    fprintf(stderr, "       %s -x[flmovw] device [filename...]\n", myname);
    exit(1);
}
