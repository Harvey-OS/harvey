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
 * A structure used to adjust pathnames in DWB C code. Pointers
 * set the address field, arrays use the value field and must
 * also set length to the number elements in the array. Pointers
 * are always reallocated and then reinitialized; arrays are only
 * reinitialized, if there's room.
 *
 */

typedef struct {
	char	**address;
	char	*value;
	int	length;
} dwbinit;

extern void	DWBinit(char *, dwbinit *);
extern char*	DWBhome(void);
extern void	DWBprefix(char *, char *, int);
