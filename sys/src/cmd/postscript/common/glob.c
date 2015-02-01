/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 * Global varibles - for PostScript translators.
 *
 */

#include <stdio.h>
#include "gen.h"

char	**argv;				/* global so everyone can use them */
int	argc;

int	x_stat = 0;			/* program exit status */
int	debug = OFF;			/* debug flag */
int	ignore = OFF;			/* what we do with FATAL errors */

long	lineno = 0;			/* line number */
long	position = 0;			/* byte position */
char	*prog_name = "";		/* and program name - for errors */
char	*temp_file = NULL;		/* temporary file - for some programs */
char	*fontencoding = NULL;		/* text font encoding scheme */

int	dobbox = FALSE;			/* enable BoundingBox stuff if TRUE */
double	pageheight = PAGEHEIGHT;	/* only for BoundingBox calculations! */
double	pagewidth = PAGEWIDTH;

int	reading = UTFENCODING;		/* input */
int	writing = WRITING;		/* and output encoding */

