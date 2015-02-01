/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char *psinit(int, int);		/* second arg is debug flag; returns "" on success */
int image2psfile(int, Memimage*, int);
void psopt(char *, void *);

int paperlength;
int paperwidth;
