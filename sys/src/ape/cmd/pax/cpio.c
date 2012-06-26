/* $Source: /u/mark/src/pax/RCS/cpio.c,v $
 *
 * $Revision: 1.2 $
 *
 * cpio.c - Cpio specific functions for archive handling
 *
 * DESCRIPTION
 *
 *	These function provide a cpio conformant interface to the pax
 *	program.
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
 * $Log:	cpio.c,v $
 * Revision 1.2  89/02/12  10:04:13  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:05  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: cpio.c,v 1.2 89/02/12 10:04:13 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__
static void 	usage(void);
#else /* !__STDC__ */
static void 	usage();
#endif /* __STDC__ */


/* do_cpio - handle cpio format archives
 *
 * DESCRIPTION
 *
 *	Do_cpio provides a standard CPIO interface to the PAX program.  All
 *	of the standard cpio flags are available, and the behavior of the
 *	program mimics traditonal cpio.
 *
 * PARAMETERS
 *
 *	int	argc	- command line argument count
 *	char	**argv	- pointer to command line arguments
 *
 * RETURNS
 *
 *	Nothing.
 */

#ifdef __STDC__

int do_cpio(int argc, char **argv)

#else

int do_cpio(argc, argv)
int             argc;
char          **argv;

#endif
{
    int             c;
    char           *dirname;
    Stat            st;

    /* default input/output file for CPIO is STDIN/STDOUT */
    ar_file = "-";
    names_from_stdin = 1;

    /* set up the flags to reflect the default CPIO inteface. */
    blocksize = BLOCKSIZE;
    ar_interface = CPIO;
    ar_format = CPIO;
    msgfile=stderr;

    while ((c = getopt(argc, argv, "D:Bacdfilmoprtuv")) != EOF) {
	switch (c) {
	case 'i':
	    f_extract = 1;
	    break;
	case 'o':
	    f_create = 1;
	    break;
	case 'p':
	    f_pass = 1;
	    dirname = argv[--argc];

	    /* check to make sure that the argument is a directory */
	    if (LSTAT(dirname, &st) < 0) {
		fatal(strerror());
	    }
	    if ((st.sb_mode & S_IFMT) != S_IFDIR) {
		fatal("Not a directory");
	    }
	    break;
	case 'B':
	    blocksize = BLOCK;
	    break;
	case 'a':
	    f_access_time = 1;
	    break;
	case 'c':
	    break;
	case 'D':
	    ar_file = optarg;
	    break;
	case 'd':
	    f_dir_create = 1;
	    break;
	case 'f':
	    f_reverse_match = 1;
	    break;
	case 'l':
	    f_link = 1;
	    break;
	case 'm':
	    f_mtime = 1;
	    break;
	case 'r':
	    f_interactive = 1;
	    break;
	case 't':
	    f_list = 1;
	    break;
	case 'u':
	    f_unconditional = 1;
	    break;
	case 'v':
	    f_verbose = 1;
	    break;
	default:
	    usage();
	}
    }

    if (f_create + f_pass + f_extract != 1) {
	usage();
    }
    if (!f_pass) {
	buf_allocate((OFFSET) blocksize);
    }
    if (f_extract) {
	open_archive(AR_READ);	/* Open for reading */
	read_archive();
    } else if (f_create) {
	open_archive(AR_WRITE);
	create_archive();
    } else if (f_pass) {
	pass(dirname);
    }

    /* print out the total block count transfered */
    fprintf(stderr, "%ld Blocks\n", ROUNDUP(total, BLOCKSIZE) / BLOCKSIZE);
    
    exit(0);
    /* NOTREACHED */
}


/* usage - print a helpful message and exit
 *
 * DESCRIPTION
 *
 *	Usage prints out the usage message for the CPIO interface and then
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
    fprintf(stderr, "Usage: %s -o[Bacv]\n", myname);
    fprintf(stderr, "       %s -i[Bcdmrtuvf] [pattern...]\n", myname);
    fprintf(stderr, "       %s -p[adlmruv] directory\n", myname);
    exit(1);
}
