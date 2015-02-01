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
 * An array of type Pages is used to keep track of the starting and ending byte
 * offsets for the pages we've been asked to print.
 *
 */

typedef struct {
	long	start;			/* page starts at this byte offset */
	long	stop;			/* and ends here */
	int	empty;			/* dummy page if TRUE */
} Pages;

/*
 *
 * Some of the non-integer functions in postreverse.c.
 *
 */

char	*copystdin();

