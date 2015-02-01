/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * External varibles - most are in glob.c.
 */

extern char	**argv;			/* global so everyone can use them */
extern int	argc;

extern int	x_stat;			/* program exit status */
extern int	debug;			/* debug flag */
extern int	ignore;			/* what we do with FATAL errors */

extern long	lineno;			/* line number */
extern long	position;		/* byte position */
extern char	*prog_name;		/* and program name - for errors */
extern char	*temp_file;		/* temporary file - for some programs */
extern char	*fontencoding;		/* text font encoding scheme */

extern int	dobbox;			/* enable BoundingBox stuff if TRUE */
extern double	pageheight;		/* only for BoundingBox calculations! */
extern double	pagewidth;

extern int	reading;		/* input */
extern int	writing;		/* and output encoding */

extern char	*optarg;		/* for getopt() */
extern int	optind;

extern int	cat(char *);
extern void	error(int, char *, ...);
extern int	in_olist(int);
extern void	interrupt(int);
extern int	out_list(char *);
extern int	setencoding(char *);
extern char	*tempnam(char*,char*);
